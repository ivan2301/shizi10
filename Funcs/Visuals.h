void Visuals()
{
	//visuals main
	while (Gbl.isConnect && !HookMgr::CloseEvent)
	{
		//get glow array
		auto GlowObj = csgoProc.Read(Off.dwGlowObjectManager);

		//process players
		for (uint8_t i = 0; i < Gbl.MaxPlayers; ++i)
		{
			//check valid entity
			const auto Ent = &Players[i];
			if (Ent->Valid())
			{
				if (!Ent->IsTeam)
				{
					//get data
					auto GlowIndx = csgoProc.Read(Ent->GetPtr(Off.m_iGlowIndex));
					if (!GlowIndx) continue;

					//visible
					if (Ent->IsVisible()) {
						csgoProc.Write(Ent->GetPtr(Off.m_clrRender), std::array<uint8_t, 3>{ 0, 255, 0 });
						csgoProc.Write((GlowObj + ((GlowIndx * 0x38) + 0x8)), std::array{ 0.5f, 1.f, 0.5f, 0.8f });
					}

					//in_visible
					else {
						csgoProc.Write(Ent->GetPtr(Off.m_clrRender), std::array<uint8_t, 3>{ 255, 0, 0 });
						csgoProc.Write((GlowObj + ((GlowIndx * 0x38) + 0x8)), std::array{ 1.f, 0.4f, 0.4f, 0.8f });
					}

					//enable glow
					csgoProc.Write((GlowObj + ((GlowIndx * 0x38) + 0x28)), true);
				}

				else {
					//reset chams
					csgoProc.Write(Ent->GetPtr(Off.m_clrRender), std::array<uint8_t, 3>{ 255, 255, 255 });
				}
			}
		}

		//set convars
		ConVar::SetVarF(Off.viewmodel_fov, 68.f);
		ConVar::SetVarF(Off.r_modelAmbientMin, 1.25f);
		ConVar::SetVarF(Off.viewmodel_offset_x, -2.5f);

		//pSleep
		FC(kernel32, Sleep, 2);
	}
}

