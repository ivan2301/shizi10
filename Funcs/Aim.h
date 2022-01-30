#include "AimHelper.h"
AimBotHelper ah;

//Aim logic
class AimBotNew
{
public:
	//aim sets
	float Fov;
	float Smooth;
	int8_t hitbox_list[5];
	
	//
	bool bt;
	int btTick;
	Vector3 btPos;

	//aim ent
	int ShotHitBox;
	PlayerCache* ShotEnt;

	//aim utils
	void fixSmoothPos(Vector3& pos, const PlayerCache* ent) 
	{
		//get velocity
		auto vel = ah.GetVelocity(ent);
		
		//z broken (VALVE FIX PLS)
		vel.z = 0.f;

		//comp velocity
		pos += (vel * 0.1f);
	}

	//weapon type
	bool RifleWeapon() {
		return (Smooth > 0.f);
	}
	
	bool NoAutoWeapon() {
		return (Smooth <= 0.f);
	}

	bool UserCmdSet() {
		return (Smooth == -1.f);
	}
	
	//aim funcs
	bool GetWeaponSettings()
	{
		//check weapon
		auto WeaponBase = ah.GetActiveWeaponBase();
		if (!WeaponBase)
			return false;

		//reload weapon check
		if (ah.inReload(WeaponBase))
			return false;

		//weapon switch
		auto weaponId = ah.WeaponID(WeaponBase);

		auto pushHitBox = [&](std::initializer_list<int> list) {
			for (size_t i = 0; i < list.size(); ++i) {
				hitbox_list[i] = list.begin()[i];
			} hitbox_list[list.size()] = -1;
		};

		bt = false;
		switch (weaponId)
		{
			case WEAPON_DEAGLE:
			case WEAPON_USP_SILENCER:
			case WEAPON_GLOCK:
			case WEAPON_FIVESEVEN:
			case WEAPON_HKP2000:
			case WEAPON_P250: {
				Fov = 2.f;
				Smooth = -1.f;
				bt = true;
				pushHitBox({ Hitbox_Head });
			} break;

			case WEAPON_AUG:
			case WEAPON_AK47:
			case WEAPON_M4A1:
			case WEAPON_M4A1_SILENCER:
			case WEAPON_MP5_SD:
			case WEAPON_MAC10:
			case WEAPON_GALILAR:
			case WEAPON_UMP45:
			case WEAPON_P90:
			case WEAPON_BIZON:
			case WEAPON_FAMAS:
			case WEAPON_MP7:
			case WEAPON_MP9:
			case WEAPON_SG556: {
				Fov = 5.0f;
				Smooth = 1.5f;
				pushHitBox({ Hitbox_Head, Hitbox_Chest, Hitbox_Stomach, Hitbox_Pelvis });
			} break;

			case WEAPON_AWP:
			case WEAPON_SSG08: {
				if (ah.IsScoped()) {
					Fov = 4.5f;
					Smooth = -1.f;
					bt = true;

					//MLG (rage mode)
					if (weaponId != WEAPON_SSG08) {
						pushHitBox({ Hitbox_Head, Hitbox_Chest, Hitbox_Stomach, Hitbox_Pelvis });
					}
					else {
						pushHitBox({ Hitbox_Head });
					}

					//ok!
					break;
				}
			}

			//no weapon in list
			default:
				return false;
		}

		return true;
	}

	bool FindTarget()
	{
		//get viewangles, camera pos
		const auto VA = Engine::ViewAngles();
		const auto RCS_VA = ah.RCS(VA, false);
		const auto LP_HeadPos = ah.GetLocalHeadPos(); 
		
		//find entity on fov
		ShotEnt = nullptr;
		auto bestFov = Fov + ((RCS_VA - VA).LengthXY() * 0.4f); //hehe
		for (uint8_t i = 0; i < Gbl.MaxPlayers; ++i)
		{
			//check valid
			auto EntTmp = &Players[i];
			if (EntTmp->Valid() && !EntTmp->IsTeam && EntTmp->IsVisible())
			{
				//scan hitbox (in list)
				for (const auto& h : hitbox_list)
				{
					//end list
					if (h == -1)
						break;

					if (!bt) 
					{
						//no visible
						if (!EntTmp->IsVisible(h))
							continue;

						float curFov;
						if (bestFov > (curFov = Math::GetFov(RCS_VA, LP_HeadPos, EntTmp->GetHibox(h)))) {
							bestFov = curFov;
							ShotEnt = EntTmp;
							ShotHitBox = h;
						}
					}

					else
					{
						//scan ticks
						for (int a = 0; a < 12; ++a)
						{
							auto record = bctx.records[i][a].Access();
							if (bctx.record_valid(record))
							{
								//check fov
								float curFov;
								auto bt_pos = record->GetHitBox(EntTmp, h);
								if (bestFov > (curFov = Math::GetFov(VA, LP_HeadPos, bt_pos))) {
									bestFov = curFov;
									btPos = bt_pos;
									btTick = bctx.to_tick(record->simtime);
									ShotEnt = EntTmp;
								}
							}
							record->Release();
						}
					}
				}
			}
		}

		//check found status
		return (bool)ShotEnt;
	}

	void Go2Target(Vector3& angle) 
	{
		//check valid aim target
		if (ShotEnt->Valid() && ShotEnt->IsVisible())
		{
			//last chance fix angles
			angle.ClampAngle();

			//silent angles + backtrack
			if (UserCmdSet()) {
				ah.SetSilentAngles(angle, bt ? btTick : 0);
			}

			//normal apply angles
			else {
				Engine::ViewAngles(angle);
			}
		}
	}
};

AimBotNew aim;

//aim main
void Aimbot()
{
	//aim vars
	FButton<VK_LBUTTON> LKM;

	//main loop
	while (Gbl.isConnect && !HookMgr::CloseEvent)
	{
		//check need aim
		if (!LKM.isDown(true) ||
			!aim.GetWeaponSettings() ||
			!aim.FindTarget()) {
			goto pSleep;
		}

		//check aim key
		if ((LKM.Click() && aim.NoAutoWeapon()) ||
			(LKM.Press() && aim.RifleWeapon()))
		{
			//get enemy pos
			auto entpos = aim.bt ? aim.btPos : ah.GetHitBox(aim.ShotEnt, aim.ShotHitBox);
			if (aim.RifleWeapon()) aim.fixSmoothPos(entpos, aim.ShotEnt);

			//calc aim angle
			Vector3 AimAngle = Math::CalcAngle(ah.GetLocalHeadPos(), entpos);

			//only rifles code
			if (aim.RifleWeapon())
			{
				//rcs
				AimAngle = ah.RCS(AimAngle.ClampAngle(), true);

				//smooth
				Math::SmoothAngle(Engine::ViewAngles(), AimAngle, aim.Smooth);
			}
			
			//set angles
			aim.Go2Target(AimAngle);
		}

		//pSleep
		pSleep:
		FC(kernel32, Sleep, 1); //need better
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}