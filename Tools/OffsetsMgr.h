//offets
struct SIG
{
	//offsets
	uint32_t dwInput;
	uint32_t MmScheduler;
	uint32_t dwCHLClient;
	uint32_t dwEntityList;
	uint32_t m_pStudioHdr;
	uint32_t dwGlobalVars;
	uint32_t ForceUpdateFn;
	uint32_t dwClientState;
	uint32_t dwIClientMode;
	uintptr_t dwAcceptMatch;
	uint32_t m_flNextCmdTime;
	uint32_t host_limitlocal;
	uint32_t dwModelPrecache;
	uint32_t dwInput_UserCmd;
	uint32_t dwTraceLineUtil;
	uint32_t dwSmokeCheckUtil;
	uint32_t LastOutGoingCommand;
	uint32_t dwClientState_State;
	uint32_t dwGlowObjectManager;
	uint32_t dwClientState_MaxPlayer;
	uint32_t dwClientState_ViewAngles;

	//netvars
	uint32_t m_iFOV;
	uint32_t m_iClip1;
	uint32_t m_iHealth;
	uint32_t m_iTeamNum;
	uint32_t m_lifeState;
	uint32_t m_bInReload;
	uint32_t m_clrRender;
	uint32_t m_vecOrigin;
	uint32_t m_bIsScoped;
	uint32_t m_hMyWeapons;
	uint32_t m_hViewModel;
	uint32_t m_iGlowIndex;
	uint32_t m_iItemIDHigh;
	uint32_t m_iDefaultFOV;
	uint32_t m_vecVelocity;
	uint32_t m_nModelIndex;
	uint32_t m_dwBoneMatrix;
	uint32_t m_hGroundEntity;
	uint32_t m_hActiveWeapon;
	uint32_t m_aimPunchAngle;
	uint32_t m_vecViewOffset;
	uint32_t m_flFallbackWear;
	uint32_t b_IsDormant = 0xED;
	uint32_t m_bGunGameImmunity;
	uint32_t m_flSimulationTime;
	uint32_t m_nFallbackPaintKit;
	uint32_t m_iItemDefinitionIndex;
	uint32_t dwClientState_GetLocalPlayer;

	//convars
	uint32_t viewmodel_fov;
	uint32_t r_modelAmbientMin;
	uint32_t viewmodel_offset_x;
	uint32_t engine_no_focus_sleep;

	//
	uint32_t sv_client_min_interp_ratio;
	uint32_t sv_client_max_interp_ratio;
	uint32_t sv_minupdaterate;
	uint32_t sv_maxupdaterate;
	uint32_t cl_interp_ratio;
	uint32_t cl_updaterate;
	uint32_t sv_maxunlag;
	uint32_t cl_interp;
} Off;

void ParseNetVars()
{
	class CRecvTable
	{
	private:
		class CRecvProp {
		public:
			auto VarName() {
				return csgoProc.Read<std::array<char, 128>>(csgoProc.Read((uintptr_t)this));
			}

			auto Offset() {
				return csgoProc.Read((uintptr_t)this + 0x2C);
			}

			auto DataTable() {
				return (CRecvTable*)((uintptr_t)csgoProc.Read((uintptr_t)this + 0x28));
			}
		};

		int NumProps() {
			return csgoProc.Read<int>((uintptr_t)this + 4);
		}

		auto PropById(int id) {
			return (CRecvProp*)((uintptr_t)csgoProc.Read((uintptr_t)this) + 0x3C * id);
		}

	public:
		auto Name() {
			return csgoProc.Read<std::array<char, 128>>(csgoProc.Read((uintptr_t)this + 0xC));
		}

		 uint32_t Offset(const char* varName, DWORD dwLevel = 0)
		{
			for (int i = 0; i < this->NumProps(); ++i)
			{
				auto recvProp = this->PropById(i);
				if (!recvProp) {
					continue;
				}

				auto off = recvProp->Offset();

				if (FU::StrCmp(varName, recvProp->VarName().data(), true)) {
					return dwLevel + off;
				}

				auto child = recvProp->DataTable();
				if (!child) {
					continue;
				}

				auto ret = child->Offset(varName, dwLevel + off);
				if (ret) {
					return ret;
				}
			}

			return 0;
		};
	};
	class ClientClass
	{
	public:
		auto NextClass() {
			return (ClientClass*)((uintptr_t)csgoProc.Read((uintptr_t)this + 0x10));
		}

		auto Table() {
			return (CRecvTable*)((uintptr_t)csgoProc.Read((uintptr_t)this + 0xC));
		}
	};

	auto dwGetAllClasses = (uintptr_t)csgoProc.HazeDumper(clientDll, E("A1 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A1 ? ? ? ? B9"), { 1, 0 });
	auto NetVar = [&](const char* classname, const char* varname, uint32_t off = 0) -> uint32_t {
		for (auto pClass = (ClientClass*)dwGetAllClasses; pClass; pClass = pClass->NextClass()) {
			auto table = pClass->Table();
			if (FU::StrCmp(classname, table->Name().data(), true)) {
				auto ret = table->Offset(varname);
				if (ret) { ret += off; }
				return ret;
			}
		}

		return false;
	};

	auto csPlayer = xorstr("DT_CSPlayer");
	Off.m_iFOV = NetVar(csPlayer.crypt_get(), E("m_iFOV"));
	Off.m_bIsScoped = NetVar(csPlayer.get(), E("m_bIsScoped"));
	Off.m_lifeState = NetVar(csPlayer.get(), E("m_lifeState"));
	Off.m_iDefaultFOV = NetVar(csPlayer.get(), E("m_iDefaultFOV"));
	Off.m_vecVelocity = NetVar(csPlayer.get(), E("m_vecVelocity[0]"));
	Off.m_hGroundEntity = NetVar(csPlayer.get(), E("m_hGroundEntity"));
	Off.m_iGlowIndex = NetVar(csPlayer.get(), E("m_flFlashDuration"), 24);
	Off.m_vecViewOffset = NetVar(csPlayer.get(), E("m_vecViewOffset[0]"));
	Off.m_bGunGameImmunity = NetVar(csPlayer.get(), E("m_bGunGameImmunity"));
	Off.m_flSimulationTime = NetVar(csPlayer.get(), E("m_flSimulationTime"));
	
	auto basePlayer = xorstr("DT_BasePlayer");
	Off.m_iHealth = NetVar(basePlayer.crypt_get(), E("m_iHealth"));
	Off.m_iTeamNum = NetVar(basePlayer.get(), E("m_iTeamNum"));
	Off.m_vecOrigin = NetVar(basePlayer.get(), E("m_vecOrigin"));
	Off.m_hViewModel = NetVar(basePlayer.get(), E("m_hViewModel[0]"));
	Off.m_hActiveWeapon = NetVar(basePlayer.get(), E("m_hActiveWeapon"));
	Off.m_aimPunchAngle = NetVar(basePlayer.get(), E("m_aimPunchAngle"));

	auto weaponClass = xorstr("DT_BaseCombatWeapon");
	Off.m_iClip1 = NetVar(weaponClass.crypt_get(), E("m_iClip1"));
	Off.m_bInReload = NetVar(weaponClass.get(), E("m_flNextPrimaryAttack"), 109);
	Off.m_iItemDefinitionIndex = NetVar(weaponClass.get(), E("m_iItemDefinitionIndex"));

	auto attrClass = xorstr("DT_BaseAttributableItem");
	Off.m_iItemIDHigh = NetVar(attrClass.crypt_get(), E("m_iItemIDHigh"));
	Off.m_flFallbackWear = NetVar(attrClass.get(), E("m_flFallbackWear"));
	Off.m_nFallbackPaintKit = NetVar(attrClass.get(), E("m_nFallbackPaintKit"));

	Off.m_clrRender = NetVar(E("DT_BaseEntity"), E("m_clrRender"));
	Off.m_nModelIndex = NetVar(E("DT_BaseViewModel"), E("m_nModelIndex"));
	Off.m_hMyWeapons = NetVar(E("DT_BaseCombatCharacter"), E("m_hMyWeapons"));
	Off.m_dwBoneMatrix = NetVar(E("DT_BaseAnimating"), E("m_nForceBone"), 28);
}

void ParseOffsets()
{
	//client
	Off.dwInput_UserCmd = csgoProc.HazeDumper(clientDll, E("03 87 ? ? ? ? 5F 39"), { 2 });
	Off.dwCHLClient = csgoProc.HazeDumper(clientDll, E("A1 ? ? ? ? 35 ? ? ? ? A3"), { 1 });
	Off.MmScheduler = csgoProc.HazeDumper(clientDll, E("83 3D ? ? ? ? ? 74 0C 51"), { 2 });
	Off.dwGlobalVars = csgoProc.HazeDumper(clientDll, E("A1 ? ? ? ? F3 0F 10 40 10"), { 1, 0 });
	Off.dwTraceLineUtil = csgoProc.FindPattern(clientDll, E("55 8B EC 83 E4 F0 83 EC 7C 56 52"));
	Off.dwInput = csgoProc.HazeDumper(clientDll, E("B9 ? ? ? ? F3 0F 11 04 24 FF 50 10"), { 1 });
	Off.dwGlowObjectManager = csgoProc.HazeDumper(clientDll, E("0F 11 05 ? ? ? ? 83 C8 01"), { 3 });
	Off.dwAcceptMatch = csgoProc.FindPattern(clientDll, E("55 8B EC 51 56 8B 35 ? ? ? ? 57 83 BE"));
	Off.dwSmokeCheckUtil = csgoProc.FindPattern(clientDll, E("55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0"));
	Off.dwEntityList = csgoProc.HazeDumper(clientDll, E("BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8"), { 1 });
	Off.dwIClientMode = csgoProc.HazeDumper(clientDll, E("8B 0D ? ? ? ? FF 75 08 8B 01 FF 50 64"), { 2, 0 });
	Off.m_pStudioHdr = csgoProc.HazeDumper(clientDll, E("8B B6 ? ? ? ? 85 F6 74 05 83 3E 00 75 02 33 F6 F3 0F 10 44 24"), { 2 });

	//engine
	Off.m_flNextCmdTime = csgoProc.HazeDumper(engineDll, E("F2 0F 10 87 ? ? ? ? 66 0F 2F"), { 4 });
	Off.dwClientState_State = csgoProc.HazeDumper(engineDll, E("83 B8 ? ? ? ? ? 0F 94 C0 C3"), { 2 });
	Off.dwClientState_GetLocalPlayer = csgoProc.HazeDumper(engineDll, E("8B 80 ? ? ? ? 40 C3"), { 2 });
	Off.LastOutGoingCommand = csgoProc.HazeDumper(engineDll, E("8B 8F ? ? ? ? 8B 87 ? ? ? ? 41"), { 2 });
	Off.ForceUpdateFn = csgoProc.FindPattern(engineDll, E("A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85"));
	Off.dwClientState = csgoProc.HazeDumper(engineDll, E("A1 ? ? ? ? 33 D2 6A 00 6A 00 33 C9 89 B0"), { 1, 0 });
	Off.dwModelPrecache = csgoProc.HazeDumper(engineDll, E("0C 3B 81 ? ? ? ? 75 11 8B 45 10 83 F8 01 7C 09 50 83"), { 3 });
	Off.dwClientState_ViewAngles = csgoProc.HazeDumper(engineDll, E("F3 0F 11 86 ? ? ? ? F3 0F 10 44 24 ? F3 0F 11 86"), { 4 });
	Off.dwClientState_MaxPlayer = csgoProc.HazeDumper(engineDll, E("A1 ? ? ? ? 8B 80 ? ? ? ? C3 CC CC CC CC 55 8B EC 8A 45 08"), { 7 });
	
	//convars
	Off.viewmodel_fov = ConVar::FindVar(clientDll, E("viewmodel_fov"));
	Off.host_limitlocal = ConVar::FindVar(engineDll, E("host_limitlocal"));
	Off.r_modelAmbientMin = ConVar::FindVar(engineDll, E("r_modelAmbientMin"));
	Off.viewmodel_offset_x = ConVar::FindVar(clientDll, E("viewmodel_offset_x"));
	Off.engine_no_focus_sleep = ConVar::FindVar(engineDll, E("engine_no_focus_sleep"));

	//
	Off.sv_client_min_interp_ratio = ConVar::FindVar(engineDll, E("sv_client_min_interp_ratio"));
	Off.sv_client_max_interp_ratio = ConVar::FindVar(engineDll, E("sv_client_max_interp_ratio"));
	Off.sv_minupdaterate = ConVar::FindVar(engineDll, E("sv_minupdaterate"));
	Off.sv_maxupdaterate = ConVar::FindVar(engineDll, E("sv_maxupdaterate"));
	Off.cl_interp_ratio = ConVar::FindVar(clientDll, E("cl_interp_ratio"));
	Off.cl_updaterate = ConVar::FindVar(engineDll, E("cl_updaterate"));
	Off.sv_maxunlag = ConVar::FindVar(serverDll, E("sv_maxunlag"));
	Off.cl_interp = ConVar::FindVar(clientDll, E("cl_interp"));

	//netvar parse
	ParseNetVars();
}