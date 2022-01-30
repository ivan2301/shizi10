//Prep app
void StartApp(const char* title)
{
	//fix str
	ntdll.crypt_get();
	win32u.crypt_get();
	user32.crypt_get();
	kernel32.crypt_get();
	kernelbase.crypt_get();

	//fix console
	FC3(kernel32, SetConsoleTitleA, title);
	auto hConsoleOut = FC3(kernel32, GetStdHandle, STD_OUTPUT_HANDLE);
	FC3(kernel32, SetConsoleTextAttribute, hConsoleOut, 10);
	CONSOLE_CURSOR_INFO structCursorInfo;
	FC3(kernel32, GetConsoleCursorInfo, hConsoleOut, &structCursorInfo);
	structCursorInfo.bVisible = false;
	FC3(kernel32, SetConsoleCursorInfo, hConsoleOut, &structCursorInfo);
	auto hConsoleIn = FC3(kernel32, GetStdHandle, STD_INPUT_HANDLE);
	FC3(kernel32, SetConsoleMode, hConsoleIn, ENABLE_EXTENDED_FLAGS);

	//fix sleep
	ULONG minRes, maxRes, curRes;
	FC2(ntdll, NtQueryTimerResolution, &maxRes, &minRes, &curRes);
	FC2(ntdll, NtSetTimerResolution, minRes, true, &curRes);

	//load user32
	FC3(kernel32, LoadLibraryA, user32.get());
}

//Thread mgr
class FThread 
{
public:
	static void BoostPriority(HANDLE thread, int level = THREAD_PRIORITY_TIME_CRITICAL) {
		FC3(kernel32, SetThreadPriority, thread, level);
	}

	template<typename argT = void*>
	FThread(void* func, int level = THREAD_PRIORITY_TIME_CRITICAL, argT arg = nullptr) {
		auto Thread = FC3(kernel32, CreateThread, nullptr, 0, (LPTHREAD_START_ROUTINE)func, (void*)arg, 0, nullptr);
		BoostPriority(Thread, level);
		FC3(kernel32, CloseHandle, Thread);
	}
};

//Memory mgr
class FMemory
{
private:
	void* Ptr;
	bool PostFree;

public:
	FMemory(size_t size, bool destruct = true) {
		PostFree = destruct;
		Ptr = FC3(kernel32, VirtualAlloc, nullptr, size, MEM_COMMIT, PAGE_READWRITE);
	}
	
	~FMemory() {
		if (PostFree) {
			FC3(kernel32, VirtualFree, (void*)Ptr, (size_t)0, MEM_RELEASE);
		}
	}

	template <typename T = uint8_t*>
	T Get() {
		return (T)this->Ptr;
	}
};

//Module entry (GetModBase)
struct FModule {
	uint32_t ModSize;
	uintptr_t ModBase;
};

//Process mgr
template<typename T>
class FProcess
{
private:
	int PID;
	
	static void WaitClose(void* proc) {
		FC3(kernel32, WaitForSingleObject, proc, INFINITE);
		FC3(kernel32, ExitProcess, 0);
	}

	int FindPID(const char* procName, bool wait)
	{
		do
		{
			auto procSnap = FC3(kernel32, CreateToolhelp32Snapshot, TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 procInfo; procInfo.dwSize = sizeof(PROCESSENTRY32);

			while (FC3(kernel32, Process32Next, procSnap, &procInfo)) {
				if (FU::StrCmp(procName, procInfo.szExeFile, true)) {
					FC3(kernel32, CloseHandle, procSnap);
					return procInfo.th32ProcessID;
				}
			}

			FC3(kernel32, CloseHandle, procSnap);
			FC3(kernel32, Sleep, 100);
		} while (wait);

		//no process
		return -1;
	}

public:
	HANDLE Process;

	//open process
	bool Open(const char* procName)
	{
		//get pid
		this->PID = this->FindPID(procName, true);
		if (PID == -1) return false;

		//open process
		this->Process = FC3(kernel32, OpenProcess, PROCESS_ALL_ACCESS, false, this->PID);

		//monitor close process
		if (this->Process) {
			FThread watchTh(FProcess::WaitClose, THREAD_PRIORITY_LOWEST, this->Process);
		}

		return bool(this->Process != nullptr);
	}

	//get module base, size
	FModule GetModule(const char* modName, bool wait = true)
	{
		do
		{
			auto modSnap = FC3(kernel32, CreateToolhelp32Snapshot, TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->PID);
			MODULEENTRY32 modInfo; modInfo.dwSize = sizeof(MODULEENTRY32);

			while (FC3(kernel32, Module32Next, modSnap, &modInfo)) {
				if (FU::StrCmp(modName, modInfo.szModule, true)) {
					FC3(kernel32, CloseHandle, modSnap);
					return FModule{ modInfo.modBaseSize, (uintptr_t)modInfo.modBaseAddr };
				}
			}

			FC3(kernel32, CloseHandle, modSnap);
			FC3(kernel32, Sleep, 200);
		} while (wait);

		//not found
		return FModule{};
	}

	//read memory
	__forceinline void ReadArr(uintptr_t addr, void* arr, size_t size) {
		if (addr > 0x10000) {
			FU::SysCall(63, this->Process, addr, arr, size);
		}
	}
	
	template<typename ReadType = T>
	__forceinline ReadType Read(uintptr_t addr, size_t forceSize = 0) {
		ReadType ReadData{};
		ReadArr(addr, &ReadData, forceSize ? forceSize : sizeof(ReadType));
		return ReadData;
	}

	//write memory
	__forceinline void WriteArr(uintptr_t addr, void* arr, size_t size) {
		if (addr > 0x10000) {
			FU::SysCall(58, this->Process, addr, arr, size);
		}
	}

	template<typename WriteType>
	__forceinline void Write(uintptr_t addr, const WriteType& data, size_t forceSize = 0) {
		WriteArr(addr, (void*)&data, forceSize ? forceSize : sizeof(WriteType));
	}

	void WriteProtArr(uintptr_t addr, void* arr, size_t size) {
		int oldProt = 0; auto addr1 = addr; auto size1 = size;
		FU::SysCall(80, this->Process, &addr1, &size1, PAGE_EXECUTE_READWRITE, &oldProt);
		WriteArr(addr, arr, size);
		FU::SysCall(80, this->Process, &addr1, &size1, oldProt, &oldProt);
	}

	template<typename WriteType>
	void WriteProt(uintptr_t addr, const WriteType& data) {
		WriteProtArr(addr, (void*)&data, sizeof(WriteType));
	}

	//allocate, free memory
	uintptr_t Alloc(size_t size, uint32_t Prot = PAGE_READWRITE) {
		uintptr_t addr = 0;
		FU::SysCall(24, this->Process, &addr, 0ull, &size, MEM_COMMIT, Prot);
		return addr;
	}

	void Free(uintptr_t addr, size_t size = 0) {
		FU::SysCall(30, this->Process, &addr, &size, size ? MEM_DECOMMIT : MEM_RELEASE);
	}

	void Exec(uintptr_t addr) {
		HANDLE hThread = FC(kernel32, CreateRemoteThread, Process, nullptr, 0, (LPTHREAD_START_ROUTINE)addr, nullptr, 0, nullptr);
		FC(kernel32, WaitForSingleObject, hThread, INFINITE);
		FC(kernel32, CloseHandle, hThread);
	}

	//find pattern
	T FindPattern(FModule mod, const char* pat)
	{
		//read module to local memory
		FMemory mem(mod.ModSize);
		this->ReadArr(mod.ModBase, mem.Get(), mod.ModSize);

		//find pattern utils
		#define InRange(x, a, b) (x >= a && x <= b) 
		#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
		#define GetByte(x) ((uint8_t)(GetBits(x[0]) << 4 | GetBits(x[1])))

		//get module range
		auto ModuleStart = mem.Get(), ModuleEnd = mem.Get() + mod.ModSize;
		
		//scan pattern main
		PBYTE FirstMatch = nullptr;
		const char* CurPatt = pat;
		for (; ModuleStart < ModuleEnd; ++ModuleStart)
		{
			bool SkipByte = (*CurPatt == '\?');
			if (SkipByte || *ModuleStart == GetByte(CurPatt)) {
				if (!FirstMatch) FirstMatch = ModuleStart;
				SkipByte ? CurPatt += 2 : CurPatt += 3;
				if (CurPatt[-1] == 0) {
					return (T)(mod.ModBase + (FirstMatch - mem.Get()));
				}
					
			}

			else if (FirstMatch) {
				ModuleStart = FirstMatch;
				FirstMatch = nullptr;
				CurPatt = pat;
			}
		}

		return 0;
	}

	T HazeDumper(FModule mod, const char* sig, std::initializer_list<uint32_t> offsets, uint32_t extra = 0)
	{
		auto pAddr = FindPattern(mod, sig);
		if (!pAddr) {
			return 0;
		}

		for (const auto& i : offsets) {
			pAddr = Read<T>(pAddr + i);
			if (!pAddr) {
				return 0;
			}
		}

		return pAddr + extra;
	}
	
	auto GetSection(FModule mod, const char* name)
	{
		//read module headers
		FMemory mem(0x1000);
		ReadArr(mod.ModBase, mem.Get(), 0x1000);

		//get headers
		uint32_t V_Addr = 0, V_Size = 0;
		auto NtHeaders = (PIMAGE_NT_HEADERS)(mem.Get() + ((PIMAGE_DOS_HEADER)mem.Get())->e_lfanew);
		PIMAGE_SECTION_HEADER FirstSect = IMAGE_FIRST_SECTION(NtHeaders);
		for (auto CurSect = FirstSect; CurSect < FirstSect + NtHeaders->FileHeader.NumberOfSections; CurSect++) {
			if (FU::StrCmp(name, CurSect->Name, false)) {
				V_Addr = CurSect->VirtualAddress;
				V_Size = CurSect->Misc.VirtualSize;
				break;
			}
		}

		//ret
		return std::make_pair(V_Addr, V_Size);
	}
};