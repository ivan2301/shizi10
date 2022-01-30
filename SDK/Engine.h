namespace Engine
{
	auto ViewAngles() {
		return csgoProc.Read<Vector3>(Off.dwClientState + Off.dwClientState_ViewAngles, 8);
	}

	void ViewAngles(const Vector3& aimAngle) {
		csgoProc.Write(Off.dwClientState + Off.dwClientState_ViewAngles, aimAngle, 8);
	}

	bool Connected() {
		auto isConected = 
			(((csgoProc.Read<int>(Off.dwClientState + Off.dwClientState_State) == 6) &&
			 (csgoProc.Read<int>(Off.dwClientState + Off.dwClientState_MaxPlayer) > 0)));
		return isConected;
	}

	uintptr_t LocalPlayer() {
		auto LocalPlayerId = csgoProc.Read(Off.dwClientState + Off.dwClientState_GetLocalPlayer);
		return csgoProc.Read(Off.dwEntityList + (LocalPlayerId * 0x10));
	}

	int DeltaTicks() {
		return csgoProc.Read<int>(Off.dwClientState + 0x174);
	}

	bool NeedForceUpdate() {
		return DeltaTicks() != -1;
	}
}