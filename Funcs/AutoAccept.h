void AutoAccept()
{
	//autoaccept main
	while (!HookMgr::CloseEvent) 
	{
		if (!Gbl.isConnect)
		{
			//get match id
			auto CurQ = csgoProc.Read(Off.MmScheduler);
			auto CurQ_check = csgoProc.Read<int>(CurQ + 0xC);

			//found match
			if (CurQ && !CurQ_check)
			{
				//filter non mm mode
				FC(kernel32, Sleep, 4000);

				//match still exists
				if (csgoProc.Read(Off.MmScheduler))
				{
					//accept match
					csgoProc.Exec(Off.dwAcceptMatch);

					//write once flag
					csgoProc.Write(CurQ + 0xC, 1);
				}
			}
		}
		
		//pSleep
		FC(kernel32, Sleep, 1000);
	}
}