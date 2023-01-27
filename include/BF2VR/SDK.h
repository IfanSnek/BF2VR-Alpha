#pragma once
#include <windows.h>
#include <stdint.h>
#include "Matrices.h"

// Static offsets because EA abandoned the game :(
static const DWORD64 OffsetCamera = 0x1410c7010;
static const DWORD64 OffsetGameRenderer = 0x143ffbe10;
static const DWORD64 OffsetGameContext = 0x143DD7948;
static const DWORD64 OffsetSoldierWeapon = 0x1445ECF30;
static const DWORD64 OffsetUISettings = 0x143aebb80;
static const DWORD64 OffsetAim = 5436270096;

// All the basic types

struct Vec3 {
	float x;
	float y;
	float z;
};

struct Vec4 {
	float x;
	float y;
	float z;
	float w;

	Vec4 operator* (float value) {
		return { this->x * value, this->y * value, this->z * value, this->w * value };
	}
	Vec4 operator+ (Vec4 value) {
		return { this->x + value.x, this->y + value.y, this->z + value.z, this->w + value.w };
	}
	Vec4 operator- (Vec4 value) {
		return { this->x - value.x, this->y - value.y, this->z - value.z, this->w - value.w };
	}
};

// a Matrix4x4, just 4 vector4s
struct Matrix4x4 {
	Vec4 x;
	Vec4 y;
	Vec4 z;
	Vec4 o;
};

// Reversed classes, thanks to OpenGameCamera and a hack I won't link to.

class CameraObject {
public:
	Matrix4x4 cameraTransform;
};

class GameRenderer {
public:
	char pad_0000[1304]; //0x0000
	class GameRenderSettings* gameRenderSettings; //0x0510
	char pad_0520[24]; //0x0520
	class RenderView* renderView; //0x0538
	char pad_0540[4872]; //0x0540

	// static method to return the default instance
	static GameRenderer* GetInstance(void) {
		return *(GameRenderer**)OffsetGameRenderer;
	}
};

class GameRenderSettings {
public:
	char pad_0000[40]; //0x0000
	float resolutionScale; //0x0028
	char pad_002C[48]; //0x002C
	float forceFov; //0x005C
	char pad_0060[1000]; //0x0060
};

// RenderView structure, where we can read the camera transform
class RenderView {
public:
	Matrix4x4 transform;
};

class UISettings {
public:
	char pad_000[0x44];
	bool drawEnable;
	// static method to return the default instance, from an offset of GameTimeSettings's pointer
	static UISettings* GetInstance(void) {
		return *(UISettings**)OffsetUISettings;
	}
};

class VehicleEntityData
{
public:
	char pad_0000[568]; //0x0000
	char* VehicleName; //0x0238
	char pad_0240[520]; //0x0240

	char* GetName() {
		if (this != nullptr && this->VehicleName != nullptr) {
			return this->VehicleName;
		}
		return (char*)"\0";
	}
}; //Size: 0x0448

class AttachedControllable
{
public:
	char pad_0000[48]; //0x0000
	class VehicleEntityData* vehicleEntity; //0x0030
	char pad_0038[560]; //0x0038

	VehicleEntityData* GetVehicleEntityData() {
		if (this != nullptr && this->vehicleEntity != nullptr) {
			return this->vehicleEntity;
		}
	}

}; //Size: 0x1058

class SoldierBlueprint
{
public:
	char pad_0000[24]; //0x0000
	char* Name; //0x0018
	char pad_0020[40]; //0x0020
	char* GetName() {
		if (this != nullptr && this->Name != nullptr) {
			return this->Name;
		}
		return (char*)"\0";
	}
}; //Size: 0x0048

class ClientSoldierPrediction
{
public:
	char pad_0000[32]; //0x0000
	Vec3 Location; //0x0020
	char pad_002C[104]; //0x002C
}; //Size: 0x0094

class ClientAimEntity
{
public:
	char pad_0000[224]; //0x0000
	Vec4 CachedRayCast; //0x00E0
};

class ClientSoldierEntity
{
public:
	char pad_0000[712]; //0x0000
	class ClientSoldierHealthComponent* ClientSoldierHealthComponent; //0x02C8
	char pad_02D0[104]; //0x02D0
	class SoldierBlueprint* soldierBlueprint; //0x0338
	char pad_0340[404]; //0x0340
	float HeightOffset; //0x04D4
	char pad_04D8[156]; //0x04D8
	float Yaw; //0x0574
	char pad_0578[4]; //0x0578
	float Pitch; //0x057C
	char pad_0580[472]; //0x0580
	ClientSoldierPrediction* clientSoldierPrediction; //0x0758
	char pad_0760[704]; //0x0760??
	ClientAimEntity* pClientAimEntity; //0x0A20??

	SoldierBlueprint* GetSoldierBlueprint() {
		if (this != nullptr && this->soldierBlueprint != nullptr) {
			return this->soldierBlueprint;
		}
	}
}; //Size: 0x0840

class MatchInfo
{
public:
	char pad_0000[8]; //0x0000
	char* GameMode; //0x0008
	char pad_0010[1080]; //0x0010
	char* GetMatchMode() {
		if (this != nullptr && this->GameMode != nullptr) {
			return this->GameMode;
		}
		return (char*)"Menu";
	}
}; //Size: 0x0448

class ClientLevel
{
public:
	char pad_0000[48]; //0x0000
	class MatchInfo* matchInfo; //0x0030
	char* LevelName; //0x0038
	char pad_0040[1032]; //0x0040
	char* GetLevelName() {
		if (this != nullptr && this->LevelName != nullptr) {
			return this->LevelName;
		}
		return (char*)"\0";
	}
	MatchInfo* GetMatchInfo()
	{
		if (this != nullptr && this->matchInfo != nullptr)
		{
			return this->matchInfo;
		}
	}
}; //Size: 0x0148

class ClientPlayer
{
public:
	char pad_0000[24]; //0x0000
	char* Name; //0x0018
	char pad_0020[56]; //0x0020
	uint32_t Team; //0x0058
	char pad_005C[420]; //0x005C
	class AttachedControllable* attachedControllable; //0x0200
	char pad_0208[8]; //0x0208
	class ClientSoldierEntity* controlledControllable; //0x0210
	char pad_0218[4676]; //0x0218
	int32_t Score; //0x145C
	char pad_1460[4]; //0x1460
	int32_t Kills; //0x1464
	int32_t Assists; //0x1468
	int32_t Deaths; //0x146C
	char pad_1470[3028]; //0x1470

	ClientSoldierEntity* GetClientSoldier() {
		if (this != nullptr && this->attachedControllable == nullptr && this->controlledControllable != nullptr) {
			return this->controlledControllable;
		}
	}

	AttachedControllable* GetAttachedControllable() {
		if (this != nullptr && this->attachedControllable != nullptr) {
			return this->attachedControllable;
		}
	}

	uint32_t GetTeam() {
		if (this != nullptr && &this->Team != nullptr) {
			return this->Team;
		}
	}

	char* GetName() {
		if (this != nullptr && this->Name != nullptr) {
			return this->Name;
		}
		return (char*)"\0";
	}

	bool IsValid() {
		if (this != nullptr &&
			this->attachedControllable == nullptr &&
			this->controlledControllable != nullptr)
			return true;
		return false;
	}

	char* GetPlayerClassName() {
		if (IsValid()) {
			return this->GetClientSoldier()->GetSoldierBlueprint()->GetName();
		}
		//if (strcmp(outchar, "\0")) return outchar;
		return this->GetAttachedControllable()->GetVehicleEntityData()->GetName();

	}

}; //Size: 0x1040

class MppPlayer
{
public:
	class ClientPlayer* PlayerArray[64]; //0x0000
	char pad_01E0[64]; //0x01E0

	ClientPlayer* GetPlayerByID(int ID) {
		if (this != nullptr && this->PlayerArray[ID] != nullptr && this->PlayerArray[ID]->IsValid()) {
			return this->PlayerArray[ID];
		}
		return nullptr;
	}
}; //Size: 0x0220

class PlayerManager
{
public:
	char pad_0000[1384]; //0x0000
	class ClientPlayer* LocalPlayer; //0x0568
	char pad_0570[504]; //0x0570
	class MppPlayer* mppPlayer; //0x0768
	char pad_0770[6888]; //0x0770

	ClientPlayer* GetLocalPlayer() {
		if (this != nullptr && this->LocalPlayer != nullptr) {
			return this->LocalPlayer;
		}
	}
	MppPlayer* GetmPPlayer() {
		if (this != nullptr && this->mppPlayer != nullptr) {
			return this->mppPlayer;
		}
	}

}; //Size: 0x2258

class GameContext
{
public:
	char pad_0000[56]; //0x0000
	class ClientLevel* clientLevel; //0x0038
	char pad_0040[24]; //0x0040
	class PlayerManager* playerManager; //0x0058
	char pad_0060[232]; //0x0060

	ClientLevel* GetClientLevel() {
		if (this->clientLevel != nullptr) {
			return this->clientLevel;
		}
	}

	PlayerManager* GetPlayerManager() {
		if (this != nullptr && this->playerManager != nullptr) {
			return this->playerManager;
		}
	}

	static GameContext* GetInstance(void) {
		return *(GameContext**)(OffsetGameContext);
	}
}; //Size: 0x0148

// These are from the cheat. 

class UnknownPtr2
{
public:
	char pad_0000[168]; //0x0000
	float yaw; //0x00A8
	float pitch; //0x00AC
	char pad_00B0[12]; //0x00B0
}; //Size: 0x00BC

class UnknownPtr1
{
public:
	char pad_0000[56]; //0x0000
	class UnknownPtr2* UnknownPtr2; //0x0038
	char pad_0040[8]; //0x0040
}; //Size: 0x0048

class LocalAimer
{
public:
	char pad_0000[152]; //0x0000
	class UnknownPtr1* UnknownPtr1; //0x0098
	char pad_00A0[8]; //0x00A0

	static LocalAimer* Instance() {
		return *(LocalAimer**)(OffsetAim);
	}
}; //Size: 0x00A8