// ConsoleApplication8.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <windows.h>
#include <iostream>

#include <intrin.h>
#pragma intrinsic(_ReturnAddress)

struct WData {
	uint32_t PaintKit;
	uint16_t ItemIndex;
	uint16_t ItemIndexDst;
	uint8_t ModelNameCrt[24];
};

typedef void(__fastcall* __CallOrgFunc1)();

struct UnkVoidData
{
	uint32_t m_iFOV;
	uint32_t healt;
	uint32_t m_bIsScoped;
	uint32_t m_lifeState;
	uint32_t m_hMyWeapons;
	uint32_t dwEntityList;
	uint32_t m_hViewModel;
	uint32_t m_nModelIndex;
	uint32_t m_iDefaultFOV;
	uint32_t m_iItemIDHigh;
	uint32_t dwClientState;
	uint32_t dwModelPrecache;
	uint32_t m_flFallbackWear;
	uint32_t m_nFallbackPaintKit;
	uint32_t m_iItemDefinitionIndex;
	uint32_t dwClientState_GetLocalPlayer;

	uint32_t RetCheckPtr;
	__CallOrgFunc1 ForceUpdateFn;
	uint32_t UnknownVoidOrg;

	int replaceSrc;
	int replaceDst;

	int FOV;
	uint8_t NeedFU;
	WData Weapons[1];
};

template <typename StrType, typename StrType2>
__forceinline bool StrCmp(StrType Str, StrType2 InStr, bool Two) {
	wchar_t c1, c2; do {
		c1 = *Str++; c2 = *InStr++;
		if (!c1) return true;
	} while (c1 == c2);
	return false;
}

#define   SURF_HITBOX                   0x8000  
#define   SURF_TRANS                    0x0010

void __fastcall hkTest(void* ecx, void* edx)
{
	UnkVoidData* hkData = *(UnkVoidData**)0xF1345719;

	bool pu = false;
	if ((DWORD)_ReturnAddress() == hkData->RetCheckPtr)
	{
		pu = true;

		//get localPlayer
		auto localId = *(uint32_t*)(hkData->dwClientState + hkData->dwClientState_GetLocalPlayer);
		auto localPlayer = *(uint32_t*)(hkData->dwEntityList + (localId * 0x10));
		if (!localPlayer) goto last;

		//check alive
		auto IsDead = ((*(uint8_t*)(localPlayer + hkData->m_lifeState) != 0) ||
			(*(int*)(localPlayer + hkData->healt) < 1));
		if (IsDead) goto last;

		if (hkData->NeedFU == 2) {
			hkData->ForceUpdateFn();
			hkData->RetCheckPtr = 0;
			goto last2;
		}

		//fov changer
		if (hkData->FOV) {
			if (!*(bool*)(localPlayer + hkData->m_bIsScoped)) {
				*(int*)(localPlayer + hkData->m_iFOV) = hkData->FOV;
				*(int*)(localPlayer + hkData->m_iDefaultFOV) = hkData->FOV;
			}

			else {
				*(int*)(localPlayer + hkData->m_iDefaultFOV) = 90;
			}
		}

		//enum weapons
		for (uint8_t i = 0; i < 8; ++i)
		{
			//get weapon handle
			auto currentWeapon = *(uint32_t*)(localPlayer + hkData->m_hMyWeapons + (i * 4));
			if (!currentWeapon || (currentWeapon == 0xFFFFFFFF/*INVALID_EHANDLE_INDEX*/))
				continue;

			//get weapon entity
			currentWeapon = *(uint32_t*)(hkData->dwEntityList + (((currentWeapon & 0xfff) - 1) * 0x10));
			if (!currentWeapon) continue;

			//apply skins
			auto idx = (uint16_t*)(currentWeapon + hkData->m_iItemDefinitionIndex);
			auto weaponCfg = &hkData->Weapons[0];
			while (weaponCfg->ItemIndex)
			{
				if (weaponCfg->ItemIndex == *idx)
				{
					//knife check
					if (weaponCfg->ItemIndexDst)
					{
						uint32_t newIndx = 0;// = getModelIndx();
						//auto getModelIndx = [&](const char* modelName)
						{
							auto nst = *(uint32_t*)(hkData->dwClientState + hkData->dwModelPrecache);
							//if (!nst) break;

							auto nsd = *(uint32_t*)(nst + 0x40);
							//if (!nsd) break;

							auto nsdi = *(uint32_t*)(nsd + 0xC);
							//if (!nsdi) break;

							for (int a = 0; a < 1024; ++a) {
								auto nsdi_i = *(const char**)(nsdi + (a * 0x34) + 0xC);
								//if (nsdi_i)
								{
									//decrt str
									char ModelName[24];
									uint8_t cc, b = 0;
									while ((cc = weaponCfg->ModelNameCrt[b])) {
										ModelName[b++] = cc ^ 0x41;
									} ModelName[b] = 0;

									//check modelname
									if (StrCmp(ModelName, &nsdi_i[17], false)) {
										newIndx = a;
										break;
									}
								}
							}
						}

						//no model
						if (!newIndx)
							break;

						//set apply knife model
						auto indx = (uint32_t*)(currentWeapon + hkData->m_nModelIndex);
						hkData->replaceSrc = *indx;
						hkData->replaceDst = newIndx;
						*indx = newIndx;
						*idx = weaponCfg->ItemIndexDst;
					}

					//apply weapon & knife paint
					auto paintKit = (uint32_t*)(currentWeapon + hkData->m_nFallbackPaintKit);
					if (*paintKit != weaponCfg->PaintKit) {
						*paintKit = weaponCfg->PaintKit;
						*(uint32_t*)(currentWeapon + hkData->m_flFallbackWear) = 0x38d1b717; //0.0001f
						*(int*)(currentWeapon + hkData->m_iItemIDHigh) = -1;
					}

					break;
				}

				//goto next weapon
				++weaponCfg;
			}
		}

		//get active model handle
		auto activeModel = *(uint32_t*)(localPlayer + hkData->m_hViewModel);
		if (!activeModel || (activeModel == 0xFFFFFFFF/*INVALID_EHANDLE_INDEX*/))
			goto last;

		//get active model entity
		activeModel = *(uint32_t*)(hkData->dwEntityList + (((activeModel & 0xfff) - 1) * 0x10));
		auto pCurModel = (uint32_t*)(activeModel + hkData->m_nModelIndex);
		if (!activeModel) goto last;

		//change knife model
		if (*pCurModel == hkData->replaceSrc) {
			*pCurModel = hkData->replaceDst;
		}

		//forceupdate
		if (hkData->NeedFU == 1) {
			hkData->ForceUpdateFn();
		}
	}

last:
	if (pu && (hkData->NeedFU == 1))
		hkData->NeedFU = 0;

last2:
	typedef void(__fastcall* __CallOrgFunc)(PVOID, PVOID);
	__CallOrgFunc OrgFunc = (__CallOrgFunc)hkData->UnknownVoidOrg;
	OrgFunc(ecx, edx);
}

void hkTestEnd() {
	return;
}

class Vector3 {
public:
	float x, y, z;
	__forceinline Vector3 operator+(const Vector3& v) const {
		Vector3 ret;
		ret.x = x + v.x;
		ret.y = y + v.y;
		ret.z = z + v.z;
		return ret;
	}

	__forceinline Vector3 operator*(float v) const {
		Vector3 ret;
		ret.x = x * v;
		ret.y = y * v;
		ret.z = z * v;
		return ret;
	}

};

class CUserCmd
{
public:
	virtual ~CUserCmd() { };
	int command_number;
	int tick_count;
	Vector3 viewangles;
	Vector3 aimdirection;
	float forwardmove;
	DWORD sidemove;
	float upmove;
	int buttons;
	unsigned char impulse;
	int weaponselect;
	int weaponsubtype;
	int random_seed;
	short mousedx;
	short mousedy;
	bool hasbeenpredicted;
	char pad_0x4C[0x18];
};

typedef bool(__stdcall* CreateMoveFn)(float, CUserCmd*);

class matrix3x4_t {
public:
	matrix3x4_t() {}
	matrix3x4_t(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23) {
		m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
	}

	void Init(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, const Vector3& vecOrigin) {
		m_flMatVal[0][0] = xAxis.x; m_flMatVal[0][1] = yAxis.x; m_flMatVal[0][2] = zAxis.x; m_flMatVal[0][3] = vecOrigin.x;
		m_flMatVal[1][0] = xAxis.y; m_flMatVal[1][1] = yAxis.y; m_flMatVal[1][2] = zAxis.y; m_flMatVal[1][3] = vecOrigin.y;
		m_flMatVal[2][0] = xAxis.z; m_flMatVal[2][1] = yAxis.z; m_flMatVal[2][2] = zAxis.z; m_flMatVal[2][3] = vecOrigin.z;
	}

	matrix3x4_t(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, const Vector3& vecOrigin) {
		Init(xAxis, yAxis, zAxis, vecOrigin);
	}

	inline void SetOrigin(Vector3 const& p) {
		m_flMatVal[0][3] = p.x;
		m_flMatVal[1][3] = p.y;
		m_flMatVal[2][3] = p.z;
	}

	inline void Invalidate(void) {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 4; j++)
				m_flMatVal[i][j] = std::numeric_limits<float>::infinity();;
	}

	float* operator[](int i) { return m_flMatVal[i]; }
	const float* operator[](int i) const { return m_flMatVal[i]; }
	float* Base() { return &m_flMatVal[0][0]; }
	const float* Base() const { return &m_flMatVal[0][0]; }
	float m_flMatVal[3][4];
};

template<typename T> T __forceinline CallVFunc(void* vfTable, int iIndex) {
	return (*(T**)vfTable)[iIndex];
}

struct PlayerCache
{
	DWORD Ptr;
	bool IsTeam;
	int HitBoxBoneId[4];
	Vector3 HitBoxPos[4];
	bool HitBoxVisible[4];
	Vector3 HitBoxBbMin[4];
	Vector3 HitBoxBbMax[4];
};

struct CrtMoveData
{
	DWORD m_fFlags;
	DWORD m_iHealth;
	DWORD m_iTeamNum;
	DWORD dwCHLClient;
	DWORD m_lifeState;
	DWORD m_vecOrigin;
	DWORD b_IsDormant;
	DWORD dwEntityList;
	DWORD m_pStudioHdr;
	DWORD TraceLineUtil;
	DWORD m_vecViewOffset;
	DWORD dwSmokeCheckUtil;
	DWORD m_bGunGameImmunity;
	DWORD GetLocalPlayerIdPtr;

	CreateMoveFn CrtMoveOrg;
	PlayerCache Players[64];

	matrix3x4_t outBones[128];
};

struct cplane_t {
	Vector3 normal;
	float dist;
	byte type;
	byte signbits;
	byte pad[2];
};

class CBaseTrace
{
public:
	Vector3 startpos;
	Vector3 endpos;
	cplane_t plane;

	float fraction;

	int contents;
	unsigned short dispFlags;

	bool allsolid;
	bool startsolid;

	CBaseTrace()
	{}
};

struct csurface_t {
	const char* name;
	short surfaceProps;
	unsigned short flags;
};

class CGameTrace : public CBaseTrace
{
public:
	bool DidHitWorld() const;
	bool DidHitNonWorldEntity() const;
	int GetEntityIndex() const;
	bool DidHit() const;
	bool IsVisible() const;

public:

	float fractionleftsolid;
	csurface_t surface;
	int hitgroup;
	short physicsbone;
	unsigned short worldSurfaceIndex;
	DWORD m_pEnt;
	int hitbox;

	CGameTrace()
	{ }

private:
	CGameTrace(const CGameTrace& vOther);
};

typedef CGameTrace trace_t;

__forceinline const Vector3 VectorTransform(const Vector3& in1, const matrix3x4_t& in2) {
	Vector3 out;
	out.x = in1.x * in2[0][0] + in1.y * in2[0][1] + in1.z * in2[0][2] + in2[0][3];
	out.y = in1.x * in2[1][0] + in1.y * in2[1][1] + in1.z * in2[1][2] + in2[1][3];
	out.z = in1.x * in2[2][0] + in1.y * in2[2][1] + in1.z * in2[2][2] + in2[2][3];
	return out;
}

#define	FL_ONGROUND		(1 << 0)
#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)
#define IN_BACK			(1 << 4)
#define IN_USE			(1 << 5)
#define IN_CANCEL		(1 << 6)
#define IN_LEFT			(1 << 7)
#define IN_RIGHT		(1 << 8)
#define IN_MOVELEFT		(1 << 9)
#define IN_MOVERIGHT	(1 << 10)
#define IN_ATTACK2		(1 << 11)
#define IN_RUN			(1 << 12)
#define IN_RELOAD		(1 << 13)
#define IN_ALT1			(1 << 14)
#define IN_ALT2			(1 << 15)
#define IN_SCORE		(1 << 16)
#define IN_SPEED		(1 << 17)
#define IN_WALK			(1 << 18)
#define IN_ZOOM			(1 << 19)
#define IN_WEAPON1		(1 << 20)
#define IN_WEAPON2		(1 << 21)
#define IN_BULLRUSH		(1 << 22)
#define IN_GRENADE1		(1 << 23)
#define IN_GRENADE2		(1 << 24)
#define IN_LOOKSPIN		(1 << 25)
#define MULTIPLAYER_BACKUP 150

__forceinline bool is_visible(const DWORD Func, Vector3 Start, const Vector3 End, const DWORD Need, DWORD Skip)
{
	trace_t tr;
	uint8_t i = 0; //SHIT

newTrace:
	typedef int(__fastcall* UTIL_TraceLine_t)(const Vector3&, const Vector3&, unsigned int, const DWORD, int, trace_t*);
	((UTIL_TraceLine_t)Func)(Start, End, 0x46004009, Skip, 6, &tr);
	__asm add esp, 0x10

	bool visible = (tr.m_pEnt == Need && !tr.allsolid);
	if (!visible & (i < 2)) {
		bool trans = (tr.surface.flags & SURF_TRANS);
		bool hitbox = (tr.surface.flags & SURF_HITBOX);
		++i; if (trans) ++i;
		if (trans || hitbox) {
			Start = tr.endpos;
			Skip = tr.m_pEnt;
			goto newTrace;
		}
	}

	return visible;
};

bool __stdcall CreateMove(float flInputSampleTime, CUserCmd* Cmd)
{
	CrtMoveData* hkData = *(CrtMoveData**)0xF1345719;

	//valid tick
	if (Cmd && (Cmd->command_number > 0))
	{
		//get lp & lp headpos
		Vector3 localEyePosition;
		auto localId = *(uint32_t*)hkData->GetLocalPlayerIdPtr;
		auto localPlayer = *(uint32_t*)(hkData->dwEntityList + (localId * 0x10));

		//show ranks
		if (localPlayer && (Cmd->buttons & IN_SCORE)) {
			using DispatchUserMessage_t = bool* (__thiscall*)(void*, int, int, int, void*);
			volatile int indx2 = 50;
			CallVFunc<DispatchUserMessage_t>((void*)hkData->dwCHLClient, 38)((void*)hkData->dwCHLClient, indx2/*CS_UM_ServerRankRevealAll*/, 0, 0, nullptr);
		}

		auto entValid = [&](DWORD ptr, bool lp = false) [[msvc::forceinline]] {
			bool alive =
				ptr &&
				(!*(bool*)(ptr + hkData->b_IsDormant)) &&
				(*(int*)(ptr + hkData->m_iHealth) > 0) &&
				(*(int*)(ptr + hkData->m_lifeState) == 0) &&
				(lp || (!*(bool*)(ptr + hkData->m_bGunGameImmunity)));
			return alive;
		};

		auto lpValid = entValid(localPlayer, 1);
		if (lpValid)
		{
			//lp head pos
			localEyePosition = *(Vector3*)(localPlayer + hkData->m_vecOrigin) +
				*(Vector3*)(localPlayer + hkData->m_vecViewOffset);

			//check need misc
			if (hkData->m_fFlags)
			{
				//bhop
				if (Cmd->buttons & IN_JUMP) {
					if (!(*(uint32_t*)(localPlayer + hkData->m_fFlags) & FL_ONGROUND)) {
						Cmd->buttons &= ~IN_JUMP;

						//autostrafe
						if (Cmd->mousedx < 0)
							Cmd->sidemove = 0xc3e10000/*-450*/;
						else if (Cmd->mousedx > 0)
							Cmd->sidemove = 0x43e10000/*450*/;
					}
				}

				//infinity dick
				Cmd->buttons |= IN_BULLRUSH;
			}
		}

		//enum entities
		for (uint8_t i = 0; i < 64; ++i)
		{
			//get & save entity
			auto pEntity = *(uint32_t*)(hkData->dwEntityList + (i * 0x10));

			//check valid
			if (entValid(pEntity))
			{
				if (lpValid)
				{
					//skip team-mates!
					if (hkData->m_iTeamNum) {
						auto IsTeam = bool(*(int*)(pEntity + hkData->m_iTeamNum) == *(int*)(localPlayer + hkData->m_iTeamNum));
						hkData->Players[i].IsTeam = IsTeam;
						if (IsTeam) {
							goto invBones;
						}
					}

					//get hitbox ptr
					auto studio_hdr = *(uint32_t*)(pEntity + hkData->m_pStudioHdr);
					if (!studio_hdr) goto invBones;
					studio_hdr = *(uint32_t*)(studio_hdr);
					if (!studio_hdr) goto invBones;
					const auto hitbox_set_index = *(int32_t*)(studio_hdr + 0xB0);
					if (!hitbox_set_index) goto invBones;
					const auto studio_hitbox_set = studio_hdr + hitbox_set_index;
					auto iHitboxIdx = *(int32_t*)(studio_hitbox_set + 0x8);
					if (!iHitboxIdx) goto invBones;

					//get bones array
					typedef bool(__thiscall* SBones)(void*, matrix3x4_t*, int, int, float);
					if (!CallVFunc<SBones>((PVOID)(pEntity + 4), 13)((PVOID)(pEntity + 4), hkData->outBones, 128, 0x100, 0.f))
						goto invBones;

					//5 (Head, Neck, Chest, Pelvis)
					for (uint8_t a = 0, b = 0; a < 6; ++a)
					{
						//get bb bone				
						auto bonePtr = studio_hitbox_set + iHitboxIdx + (a * 0x44);

						//build hitbox pos
						auto boneId = *(int*)(bonePtr);
						volatile float f; *(DWORD*)&f = 0x3f000000; /*0.5*/
						auto HitBoxBbMin = *(Vector3*)(bonePtr + 0x8);
						hkData->Players[i].HitBoxBbMin[b] = HitBoxBbMin;
						auto HitBoxBbMax = *(Vector3*)(bonePtr + 0x14);
						hkData->Players[i].HitBoxBbMax[b] = HitBoxBbMax;
						auto End = Vector3(
							VectorTransform(HitBoxBbMin, hkData->outBones[boneId]) +
							VectorTransform(HitBoxBbMax, hkData->outBones[boneId])) * f;

						//smoke check
						using LineGoesThroughSmokeFn = bool(*)(Vector3, Vector3, int16_t);
						auto smoke = hkData->dwSmokeCheckUtil ? ((LineGoesThroughSmokeFn)hkData->dwSmokeCheckUtil)(localEyePosition, End, 1) : false;

						//pVisibleCheck
						hkData->Players[i].HitBoxVisible[b] = !smoke && is_visible(hkData->TraceLineUtil, localEyePosition, End, pEntity, localPlayer);
						hkData->Players[i].HitBoxBoneId[b] = boneId;
						hkData->Players[i].HitBoxPos[b++] = End;

						//goto chest
						if (a == 2)
							a = 4;
					}
				}

				else {
					//invalid bones
					hkData->Players[i].IsTeam = false;
				invBones:
					*(int*)&hkData->Players[i].HitBoxVisible[0] = 0;
				}

				//valid player
				hkData->Players[i].Ptr = pEntity;
			}

			else {
				//in valid player
				hkData->Players[i].Ptr = 0;
			}
		}
	}

	//call original
	return hkData->CrtMoveOrg(flInputSampleTime, Cmd);
}

void hkTestEnd2() {
	return;
}
#include <string>
#include <array>
#include <utility>
#include <cstdarg>
#include <iostream> 

#include <memory>


int main()
{

	HANDLE hFile = CreateFileA(("FSN.bin"),    // создаваемый файл
		GENERIC_WRITE,         // открывается для записи
		0,                     // совместно не используется
		NULL,                  // защита по умолчанию
		CREATE_ALWAYS,         // переписывает существующий
		FILE_ATTRIBUTE_NORMAL | // обычный файл
		0,  // асинхронный ввод/вывод I/O
		NULL);                 // атрибутов шаблона нет

	WriteFile(hFile, &hkTest, (DWORD)hkTestEnd - (DWORD)hkTest, 0, 0);
	CloseHandle(hFile);
}