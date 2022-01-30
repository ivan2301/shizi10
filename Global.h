//fix project
extern "C" int _fltused = 0;
#pragma comment(linker,"/MERGE:.rdata=.text")

//main defines
#include <windows.h>
#include <algorithm>
#include <tlhelp32.h>
#include <winternl.h>
#include <intrin.h>
#include <psapi.h>
#include <array>

//base utils
#include "Tools/EncStr.h"
#include "Tools/FACE_Call.h"
#include "SDK/FButton.h"
#include "Tools/Utils.h"
#include "SDK/Math.h"

//csgo ctx
FModule serverDll;
FModule engineDll;
FModule clientDll;
FProcess<uint32_t> csgoProc;
struct GLOBAL {
	uint8_t* FU;
	bool isConnect = 0;
	int MaxPlayers = 32;
	uintptr_t LocalPlayer = 0;
} Gbl;

//tools
#include "Tools/ConVar.h"
#include "Tools/OffsetsMgr.h"
#include "SDK/Engine.h"

//cheat helper
#include "Tools/HookMgr.h"
#include "SDK/UserCmd.h"

//shellcodes
#include "Helpers/CreateMove.h"
#include "Helpers/SkinChangerHk.h"

//external funcs
#include "Funcs/AutoAccept.h"
#include "Funcs/Visuals.h"
#include "Funcs/BackTack.h"
#include "Funcs/Aim.h"