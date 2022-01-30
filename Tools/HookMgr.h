namespace HookMgr
{
	template <typename T>
	class Hook
	{
	private:
		uintptr_t ClassPtr;
		uintptr_t Old_VMT;
		uintptr_t New_VMT;
		int VmtEntryCount;

	public:
		Hook() {
			New_VMT =
			ClassPtr =
			VmtEntryCount = 0;
		}

		bool Setup(uintptr_t Class)
		{
			//FIXME
			//auto clDllExec = csgoProc.GetSection(clientDll, E(".text"));
			////auto enDllExec = csgoProc.GetSection(engineDll, E(".text"));
			//auto exec = [&](uint32_t addr) {
			//	return (addr > (clientDll.ModBase + clDllExec.first)) && (addr < (clientDll.ModBase + clDllExec.first + clDllExec.second));// ||
			//			//((addr > (engineDll.ModBase + enDllExec.first)) && (addr < (engineDll.ModBase + enDllExec.first + enDllExec.second))));
			//};

			//get vmt
			auto VmtPtr = csgoProc.Read<T>(Class);
			if (!VmtPtr) return false;

			//get vmt entry count
			for (int i = 0;; ++i) {
				auto vmtEntryPtr = csgoProc.Read<T>(VmtPtr + (i * sizeof(T)));

				//if (!exec(vmtEntryPtr)){
				//hp(vmtEntryPtr);
				//}

				//FIXME
				if (!vmtEntryPtr) {
					if (i) {
						//dp(i);
						this->VmtEntryCount = i;
						break;
					}

					//error
					return false;
				}
			}

			//init ok!
			this->ClassPtr = Class;
			this->Old_VMT = VmtPtr;
			return true;
		}

		T GetFunc(int Index) {
			if (this->ClassPtr && (Index < this->VmtEntryCount)) {
				return csgoProc.Read<T>(this->Old_VMT + (Index * sizeof(T)));
			} return 0;
		}

		bool SetupHook(int Index, uintptr_t Func)
		{
			if (this->ClassPtr && (Index < this->VmtEntryCount))
			{
				//create new vmt
				if (!this->New_VMT) {
					const auto allocSize = this->VmtEntryCount * sizeof(T);
					this->New_VMT = csgoProc.Alloc(allocSize);
					auto Buff = FMemory(allocSize);
					csgoProc.ReadArr(this->Old_VMT, Buff.Get<void*>(), allocSize);
					csgoProc.WriteArr(this->New_VMT, Buff.Get(), allocSize);
					csgoProc.Write(this->ClassPtr, (T)this->New_VMT);
				}

				//hook
				csgoProc.Write(this->New_VMT + (Index * sizeof(T)), (T)Func);
				return true;
			}

			//error
			return false;
		}

		void UnHook(int PostDelay, int PreDelay = 0) {
			if (this->New_VMT) {
				FC3(kernel32, Sleep, PreDelay);
				csgoProc.Write(this->ClassPtr, (T)this->Old_VMT);
				FC3(kernel32, Sleep, PostDelay);
				csgoProc.Free(this->New_VMT);
				this->New_VMT = 0;
			}
		}
	};
	template <typename T>
	class HookData
	{
	private:
		uintptr_t CodePtr;
		uintptr_t DataPtr;

	public:
		auto Alloc(void* shell, uint32_t shellSize, void* shellData, uint32_t shellDataSize, uint32_t shellDataOff, size_t dataSz)
		{
			//alloc shared data
			auto hShared = FC3(kernel32, CreateFileMappingA, INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, dataSz, nullptr);
			this->DataPtr = (uintptr_t)FC3(kernelbase, MapViewOfFileNuma2, hShared, csgoProc.Process, 0, 0, dataSz, 0, PAGE_READWRITE, NUMA_NO_PREFERRED_NODE);
			auto LocalPtr = (uintptr_t)FC3(kernel32, MapViewOfFile, hShared, SECTION_MAP_WRITE, 0, 0, dataSz);
			FC3(kernel32, CloseHandle, hShared);

			//alloc code
			this->CodePtr = csgoProc.Alloc(((shellSize + 0xFFF) & 0xFFFFFFFFFFFFF000), PAGE_EXECUTE_READ);
			csgoProc.WriteProtArr(this->CodePtr, shell, shellSize);
			csgoProc.WriteProt(this->CodePtr + shellDataOff, (T)(this->DataPtr));
			
			//copy shell data
			__movsb((uint8_t*)LocalPtr, (const uint8_t*)shellData, shellDataSize);

			//local buff addr (F), remote buff addr (S)
			return std::pair{ LocalPtr, this->CodePtr };
		}

		void Free() {
			csgoProc.Free(this->CodePtr);
			FC3(kernelbase, UnmapViewOfFile2, csgoProc.Process, (void*)this->DataPtr, 0);
		}
	};
	
	//CreateMove hook
	Hook<uint32_t> ClientMode;
	Hook<uint32_t> LpHook;
	HookData<uint32_t> CreateMove;

	//SkinChanger hook
	Hook<uint32_t> SCH;
	HookData<uint32_t> MagicHook;

	//Unload
	bool CloseEvent = false;
	BOOL CloseHandler(DWORD state) 
	{
		if ((state == CTRL_BREAK_EVENT) ||
			(state == CTRL_CLOSE_EVENT))
		{
			//set close flag
			CloseEvent = true;

			//unhook createmove
			const int delay = 400;
			ClientMode.UnHook(200);
			CreateMove.Free();

			//unhook skinchanger
			bool needFU = Engine::NeedForceUpdate();
			if (needFU) { *Gbl.FU = 2; }
			SCH.UnHook(delay, needFU ? delay : 0);
			MagicHook.Free();

			//reset convars
			ConVar::SetVarI(Off.host_limitlocal, 0);
			ConVar::SetVarF(Off.viewmodel_fov, 60.f);
			ConVar::SetVarF(Off.r_modelAmbientMin, 0.f);
			ConVar::SetVarF(Off.viewmodel_offset_x, 0.f);
			ConVar::SetVarI(Off.engine_no_focus_sleep, 50);
		}

		//ok
		return true;
	}
	
	//Init
	void Init() {
		FC3(kernel32, SetConsoleCtrlHandler, CloseHandler, true);
	}
}
