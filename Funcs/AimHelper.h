class AimBotHelper
{
public:
	auto GetLocalHeadPos() {
		return csgoProc.Read<Vector3>(Gbl.LocalPlayer + Off.m_vecOrigin) + 
			   csgoProc.Read<Vector3>(Gbl.LocalPlayer + Off.m_vecViewOffset);
	}

	uintptr_t GetActiveWeaponBase()
	{
		//get weapon handle
		auto Weapon = csgoProc.Read(Gbl.LocalPlayer + Off.m_hActiveWeapon);
		if (!Weapon || (Weapon == -1/*INVALID_EHANDLE_INDEX*/)) 
			return 0;

		//get weapon entity
		return csgoProc.Read(Off.dwEntityList + ((Weapon & 0xFFF) - 1) * 0x10);
	}

	auto WeaponID(uintptr_t weapon) {
		return csgoProc.Read<short>(weapon + Off.m_iItemDefinitionIndex);
	}

	auto RCS(Vector3 ang, bool remove) {
		auto Punch = csgoProc.Read<Vector3>(Gbl.LocalPlayer + Off.m_aimPunchAngle) * 2.f;
		remove ? ang -= Punch : ang += Punch;
		return ang.ClampAngle();
	}
	
	void SetSilentAngles(const Vector3& aimAngle, int tick)
	{
		//calc vars
		const int butt = Off.m_hGroundEntity ? 0x400001/*IN_ATTACK | IN_BULLRUSH*/ : 1/*IN_ATTACK*/;
		
		//get usercmd
		UserCmd cmd1;
		cmd1.Access();

		//apply bt
		if (tick)
			cmd1.tick_count = tick;

		//access usercmd
		cmd1.viewangles = aimAngle;
		cmd1.buttons = butt;

		//write usercmd
		cmd1.Write();
	}

	__forceinline auto GetHitBox(PlayerCache* ent, int hitbox)
	{
		auto boneMatrixPtr = csgoProc.Read(ent->GetPtr() + Off.m_dwBoneMatrix);
		auto bm = csgoProc.Read<matrix3x4_t>(boneMatrixPtr + (ent->GetBoneByHitBox(hitbox) * sizeof(matrix3x4_t)));

		Vector3 vMin, vMax;
		auto val = ent->GetBByHitBox(hitbox);
		Math::VectorTransform(val.first, bm, vMin);
		Math::VectorTransform(val.second, bm, vMax);

		return Vector3(vMin + vMax) * 0.5f;
	}

	Vector3 GetVelocity(const PlayerCache* ent) {
		return csgoProc.Read<Vector3>(ent->GetPtr() + Off.m_vecVelocity);
	}

	bool inReload(uintptr_t weapon)
	{
		bool reload =
			(csgoProc.Read<bool>(weapon + Off.m_bInReload) ||
			(csgoProc.Read<int>(weapon + Off.m_iClip1) < 1));
		return reload;
	}

	bool IsScoped() {
		return csgoProc.Read<bool>(Gbl.LocalPlayer + Off.m_bIsScoped);
	}
};