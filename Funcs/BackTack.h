//backtack main
class backtrack_data_t
{
public:
	bool valid;
	float simtime;
	volatile LONG mutex;
	std::array<matrix3x4_t, 128> boneMatrix;

	auto GetHitBox(PlayerCache* ent, int hitbox)
	{
		auto bone = ent->GetBoneByHitBox(hitbox);

		Vector3 vMin, vMax;
		auto val = ent->GetBByHitBox(hitbox);
		Math::VectorTransform(val.first, boneMatrix[bone], vMin);
		Math::VectorTransform(val.second, boneMatrix[bone], vMax);

		return Vector3(vMin + vMax) * 0.5f;
	}

	auto Access() {
		SuperLoop(InterlockedCompareExchange(&mutex, true, false));
		return this;
	}

	void Release() {
		InterlockedExchange(&mutex, false);
	}
};

class BackTrackCtx
{
public:
	float LerpTime;
	float sv_maxunlag;
	float IntervalPerTick;
	backtrack_data_t** records;

	float to_time(int tick) {
		return IntervalPerTick * tick;
	}

	int to_tick(float time) {
		return (int)(0.5f + time / IntervalPerTick);
	}

	bool record_valid(backtrack_data_t* rec)
	{
		if (!rec->valid)
			return false;

		auto time = rec->simtime;
		//auto flServerTime = csgoProc.Read<float>(Off.dwGlobalVars + 0x10);
		auto flServerTime = to_time(csgoProc.Read(Gbl.LocalPlayer + 0x3440)); //tickBase

		bool IsDead = time < floorf(flServerTime - sv_maxunlag);
		if (IsDead) {
			rec->valid = false;
			return false;
		}

		float correct = 0.f;
		auto netChan = csgoProc.Read(Off.dwClientState + 0x9C);
		auto FLOW_OUTGOING = csgoProc.Read<float>(netChan + 0x4B4 + (0x1E2C * 0));
		auto FLOW_INCOMING = csgoProc.Read<float>(netChan + 0x4B4 + (0x1E2C * 1));

		//if (!Interfaces::Engine->NetChannelInfo() || !(IClientEntity*)Interfaces::EntityList->GetClientEntity(player))
		//	return false;

		correct += FLOW_OUTGOING;//Interfaces::Engine->NetChannelInfo()->get_latency(FLOW_OUTGOING);
		correct += FLOW_INCOMING;//Interfaces::Engine->NetChannelInfo()->get_latency(FLOW_INCOMING);
		correct += LerpTime/*BackTrack_GetLerpTime()*/;

		correct = std::clamp(correct, 0.f, sv_maxunlag);

		float deltaTime = correct - (flServerTime - time);
		IsDead = fabsf(deltaTime) > 0.2f;

		if (IsDead) {
			rec->valid = false;
			return false;
		}

		return true;
	}

	float lerp_time()
	{
		int ud_rate = ConVar::getVarI(Off.cl_updaterate);

		if (Off.sv_minupdaterate && Off.sv_maxupdaterate)
			ud_rate = ConVar::getVarI(Off.sv_maxupdaterate);

		float ratio = ConVar::getVarF(Off.cl_interp_ratio);

		if (ratio == 0)
			ratio = 1.f;

		float lerp = ConVar::getVarF(Off.cl_interp);

		if (Off.sv_client_min_interp_ratio && Off.sv_client_max_interp_ratio && ConVar::getVarF(Off.sv_client_min_interp_ratio) != 1.f)
			ratio = std::clamp(ratio, ConVar::getVarF(Off.sv_client_min_interp_ratio), ConVar::getVarF(Off.sv_client_max_interp_ratio));

		return max(lerp, (ratio / ud_rate));
	}

	void update() {
		LerpTime = lerp_time();
		sv_maxunlag = ConVar::getVarF(Off.sv_maxunlag);
		IntervalPerTick = csgoProc.Read<float>(Off.dwGlobalVars + 0x20);
	}

	void alloc_data() {
		auto b1 = FMemory(sizeof(uintptr_t) * 64, false).Get<backtrack_data_t**>();
		for (uint8_t i = 0; i < 64; ++i) {
			b1[i] = FMemory(sizeof(backtrack_data_t) * 12, false).Get<backtrack_data_t*>();
		} records = b1;
	}
};

BackTrackCtx bctx;

void PositionAdjustment()
{
	while (Gbl.isConnect && !HookMgr::CloseEvent)
	{
		//
		bctx.update();

		//process all players
		for (uint8_t i = 0; i < Gbl.MaxPlayers; ++i)
		{
			//get, check valid
			auto EntTmp = &Players[i];
			if (EntTmp->Valid() && !EntTmp->IsTeam)
			{
				//wait access tick
				auto tick = csgoProc.Read<int>(Off.dwGlobalVars + 0x1C) % 13;
				auto rec = bctx.records[i][tick].Access();
				
				//build cache tick
				auto boneMatrixPtr = csgoProc.Read(EntTmp->GetPtr() + Off.m_dwBoneMatrix);
				csgoProc.ReadArr(boneMatrixPtr, &rec->boneMatrix, sizeof(matrix3x4_t[128]));
				rec->simtime = csgoProc.Read<float>(EntTmp->GetPtr() + Off.m_flSimulationTime);
				rec->valid = true;

				//unlock tick
				rec->Release();
			}
		}

		//pSleep
		FC(kernel32, Sleep, 1);
	}
}