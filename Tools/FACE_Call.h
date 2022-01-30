auto ntdll = xorstr("ntdll", false);
auto user32 = xorstr("user32", false);
auto win32u = xorstr("win32u", false);
auto kernel32 = xorstr("kernel32", false);
auto kernelbase = xorstr("kernelbase", false);

namespace FU
{
	//#ifdef DEBUG
	#define dp(a) FU::Print("%d: %d\n", __LINE__, a)
	#define hp(a) FU::Print("%d: 0x%p\n", __LINE__, a)
	#define sp(a) FU::Print("%d: %s\n", __LINE__, a)
	//#define ft2i(a) (int)(a), std::abs((int)(((a) - (int)(a)) * 100000000))
	#define fp(a) printf(E("FLOAT (%d): %f\n"), __LINE__, (a))
	//#define v3p(a) FU::Print(E("V3 (%d) x:%d.%d y:%d.%d z:%d.%d\n"), __LINE__, ft2i(a.x), ft2i(a.y), ft2i(a.z))
	//#endif
	
	//Static Macro
	#define SuperLoop(a) while (a) { MemoryBarrier(); }
	#define ConstStrLen(Str) ((sizeof(Str) - sizeof(Str[0])) / sizeof(Str[0]))
	#define ToLower(Char) ((Char >= 'A' && Char <= 'Z') ? (Char + 32) : Char)

	//StrCompare (with StrInStrI(Two = false))
	template <typename StrType, typename StrType2>
	bool StrCmp(StrType Str, StrType2 InStr, bool Two) {
		if (!Str || !InStr) return false;
		wchar_t c1, c2; do {
			c1 = *Str++; c2 = *InStr++;
			c1 = ToLower(c1); c2 = ToLower(c2);
			if (!c1 && (Two ? !c2 : 1)) return true;
		} while (c1 == c2); return false;
	}

	//CRC16 StrHash	
	template <typename StrType> __declspec(noinline) constexpr unsigned short HashStr(StrType Data, int Len) {
		unsigned short CRC = 0xFFFF; while (Len--) {
			auto CurChar = *Data++; if (!CurChar) break;
			CRC ^= ToLower(CurChar) << 8; for (int i = 0; i < 8; i++)
				CRC = CRC & 0x8000 ? (CRC << 1) ^ 0x8408 : CRC << 1;
		} return CRC;
	}
	#define ConstHashStr(Str) [](){ constexpr unsigned short CRC = FU::HashStr(Str, ConstStrLen(Str)); return CRC; }()

	//EncryptDecryptPointer
	template <typename PtrType>
	 PtrType EPtr(PtrType Ptr) {
		typedef union {
			struct {
				USHORT Key1; USHORT Key2;
				USHORT Key3; USHORT Key4;
			}; ULONG64 Key;
		} CryptData;
		CryptData Key{ ConstHashStr(__TIME__), ConstHashStr(__DATE__),
			ConstHashStr(__TIMESTAMP__), ConstHashStr(__TIMESTAMP__) };
		volatile LONG64 PtrData; volatile LONG64 VKey;
		InterlockedExchange64(&VKey, (ULONG64)Key.Key);
		InterlockedExchange64(&PtrData, (ULONG64)Ptr);
		PtrData ^= VKey; return (PtrType)PtrData;
	}
	#define EPtr(Ptr) FU::EPtr(Ptr)

	//GetModuleBase
	__declspec(noinline) PBYTE GetModuleBase_Wrapper(const char* ModuleName) {
		PPEB_LDR_DATA Ldr = ((PTEB)__readgsqword(FIELD_OFFSET(NT_TIB, Self)))->ProcessEnvironmentBlock->Ldr;
		for (PLIST_ENTRY CurEnt = Ldr->InMemoryOrderModuleList.Flink; CurEnt != &Ldr->InMemoryOrderModuleList; CurEnt = CurEnt->Flink) {
			LDR_DATA_TABLE_ENTRY* pEntry = CONTAINING_RECORD(CurEnt, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
			PUNICODE_STRING BaseDllName = (PUNICODE_STRING)&pEntry->Reserved4[0];
			if (!ModuleName || StrCmp(ModuleName, BaseDllName->Buffer, false))
				return (PBYTE)pEntry->DllBase;
		}

		return nullptr;
	}
	#define GetModuleBase FC::GetModuleBase_Wrapper

	//FACE Call
	__declspec(noinline) PVOID GetExportAddress(PBYTE hDll, const char* Name)
	{
		//process image data
		PIMAGE_NT_HEADERS NT_Head = (PIMAGE_NT_HEADERS)(hDll + ((PIMAGE_DOS_HEADER)hDll)->e_lfanew);
		PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)(hDll + NT_Head->OptionalHeader.DataDirectory[0].VirtualAddress);

		//process list
		for (DWORD i = 0; i < ExportDir->NumberOfNames; i++)
		{
			//get ordinal & name
			USHORT Ordinal = ((USHORT*)(hDll + ExportDir->AddressOfNameOrdinals))[i];
			const char* ExpName = (const char*)hDll + ((DWORD*)(hDll + ExportDir->AddressOfNames))[i];
			if (StrCmp(Name, ExpName, true))
				return (PVOID)(hDll + ((DWORD*)(hDll + ExportDir->AddressOfFunctions))[Ordinal]);
		}

		return nullptr;
	}
	
	#define FC(Mod, Name, ...) [&]() __attribute__((always_inline)) { \
		static PVOID FAddr = nullptr; \
		if (!FAddr) FAddr = EPtr(FU::GetExportAddress(FU::GetModuleBase_Wrapper(Mod.get()), E(#Name))); \
		using Ret = decltype(Name(__VA_ARGS__)); \
		typedef Ret(*CallFnTest)(...); \
		return (CallFnTest(EPtr(FAddr)))(__VA_ARGS__); \
	}()

	#define FC2(Mod, Name, ...) [&]() __attribute__((always_inline)) { \
		typedef __int64(*CallFnTest)(...); \
		return (CallFnTest(FU::GetExportAddress(FU::GetModuleBase_Wrapper(Mod.get()), E(#Name))))(__VA_ARGS__); \
	}()

	#define FC3(Mod, Name, ...) [&]() __attribute__((always_inline)) { \
		using Ret = decltype(Name(__VA_ARGS__)); \
		typedef Ret(*CallFnTest)(...); \
		return (CallFnTest(FU::GetExportAddress(FU::GetModuleBase_Wrapper(Mod.get()), E(#Name))))(__VA_ARGS__); \
	}()

	//FACE Print
	template<typename... Args>
	void Print(const char* Format, Args... args) {
		char Buff[128];
		auto StrLengthOut = FC3(ntdll, sprintf, Buff, Format, args...);
		HANDLE Out = FC3(kernel32, GetStdHandle, STD_OUTPUT_HANDLE);
		FC3(kernel32, WriteConsoleA, Out, Buff, StrLengthOut, NULL, NULL);
	}

	template<class T1, class T2, class T3, class T4, class T5 = uint64_t, class T6 = uint64_t>
	__forceinline void SysCall(int id, T1 _1, T2 _2, T3 _3, T4 _4, T5 _5 = 0, T6 _6 = 0)
	{
		register auto a1 asm("r10") = _1;
		register auto a3 asm("r8") = _3;
		register auto a4 asm("r9") = _4;

		int32_t ret;
		asm("sub $64, %%rsp\n"
			"movq %[a5], 40(%%rsp)\n"
			"movq %[a6], 48(%%rsp)\n"
			"syscall\n"
			"add $64, %%rsp"
			:
			"=a"(ret),
			"=r"(a1),
			"=d"(_2), 
			"=r"(a3), 
			"=r"(a4)  
			:
			"a"(id),
			"r"(a1),
			"d"(_2),
			"r"(a3),
			"r"(a4),
			[a5] "re"(_5),
			[a6] "re"(_6)
			:
			"xmm0", "xmm1", 
			"xmm2", "xmm3", 
			"xmm4", "xmm5", 
			"rcx", "r11",
			"memory"
		);
	}
}