#include "Global.h"

int main()
{
	//prep app
	StartApp(E("FACEx v5 //by busy10"));

	//wait & attach to CS:GO
	if (csgoProc.Open(E("csgo.exe"))) {
		FU::Print(E("Attached to CS:GO. GLHF :))\n\n"));
	}

	else {
		FU::Print(E("Failed connect to CS:GO!!!\n\nClosing..."));
		FC3(kernel32, Sleep, 2000);
		FC3(kernel32, ExitProcess, 0);
	}

	//wait full game load & get modules info
	serverDll = csgoProc.GetModule(E("server.dll"));
	engineDll = csgoProc.GetModule(E("engine.dll"));
	clientDll = csgoProc.GetModule(E("client.dll"));
	csgoProc.GetModule(E("serverbrowser.dll"));
	FU::Print(E("Module's Found.\n\n"));

	//get offsets
	ParseOffsets();
	
	//init hooks
	HookMgr::Init();
	InitSkinChangerHook();
	InitCreateMove();
	
	//setup convars
	ConVar::SetVarI(Off.host_limitlocal, 1);
	ConVar::SetVarI(Off.engine_no_focus_sleep, 0);

	//init misc
	bctx.alloc_data();
	FThread aa(AutoAccept, THREAD_PRIORITY_LOWEST);
	FThread::BoostPriority(FC3(kernel32, GetCurrentThread), THREAD_PRIORITY_LOWEST);
	FU::Print(E("Cheat Runned.\n"));

	//main loop
	while (true)
	{
		//update LocalPlayer
		Gbl.LocalPlayer = Engine::LocalPlayer();
		
		//isConnect check
		if (Engine::Connected())
		{
			//run features
			if (!Gbl.isConnect) {
				Gbl.isConnect = true;
				FThread glow(Visuals, THREAD_PRIORITY_HIGHEST);
				FThread bt(PositionAdjustment, THREAD_PRIORITY_HIGHEST);
				FThread aim(Aimbot);
			}
		}

		else {
			//set disconnect status
			Gbl.isConnect = false;
		}

		//pSleep
		FC(kernel32, Sleep, 1);
	}
}