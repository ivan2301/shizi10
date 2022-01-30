namespace ConVar
{
	float getVarF(uintptr_t convar) {
		auto ret = csgoProc.Read<uint32_t>(convar + 0x2C) ^ convar;
		return *(float*)&ret;
	}
	
	int getVarI(uintptr_t convar) {
		return (int)(csgoProc.Read<uint32_t>(convar + 0x30) ^ convar);
	}

	void SetVarF(uintptr_t convar, float val) {
		csgoProc.Write(convar + 0x2C, *(uint32_t*)&val ^ convar);
	}

	void SetVarI(uintptr_t convar, int val) {
		csgoProc.Write(convar + 0x30, *(uint32_t*)&val ^ convar);
	}

	uint32_t FindVar(FModule mod, const char* shit)
	{
		auto Buff = FMemory(mod.ModSize);
		csgoProc.ReadArr(mod.ModBase, Buff.Get(), mod.ModSize);

		auto Text = csgoProc.GetSection(mod, E(".text"));
		auto Data = csgoProc.GetSection(mod, E(".data"));
		auto rData = csgoProc.GetSection(mod, E(".rdata"));

		auto inSect = [&](uint32_t val, std::pair<uint32_t, uint32_t> sect) {
			return ((val > (mod.ModBase + sect.first)) &&
					(val < (mod.ModBase + sect.first + sect.second)));
		};

		for (uintptr_t i = 0; i < rData.second; i += 4) {
			auto test = *(uint32_t*)(Buff.Get() + rData.first + i);
			if (inSect(test, Text)) {
				bool ret = false;
				uint32_t Class = 0;
				for (size_t a = 0;; a++) 
				{
					auto test2 = Buff.Get() + (test - mod.ModBase);
					auto byte1 = *(uint8_t*)(test2 + a);

					if (byte1 == 0xB9) {
						auto Class1 = *(uint32_t*)(test2 + a + 1);
						if (inSect(Class1, Data)) {
							Class = Class1;
							a += 4;
							continue;
						}
					}

					if (byte1 == 0x68) {
						auto str = *(uint32_t*)(test2 + a + 1);
						if (inSect(str, rData)) {
							char testArr[64];
							auto str2 = Buff.Get() + (str - mod.ModBase);
							__movsb((uint8_t*)testArr, str2, 64);
							if (FU::StrCmp(shit, testArr, false)) {
								ret = true;
							}

							a += 4;
							continue;
						}
					}

					if ((byte1 == 0xC3) || (byte1 == 0xCC)) {
						if (ret) {
							return Class;
						}

						break;
					}
				}
			}
		}

		//error
		return 0;
	}
}