#pragma once
#include <windows.h>
#include <stdint.h>
#include "Matrices.h"
#include "Utils.h"

static const DWORD64 OffsetGameContext  = 0x143DD7948;
static const DWORD64 OffsetLocalAimer   = 0x14406E610;
static const DWORD64 OffsetGameRenderer = 0x143ffbe10;
static const DWORD64 OffsetUISettings   = 0x143aebb80;
static const DWORD64 OffsetCamera       = 0x1410c7010;

class RenderView {
public:
	Matrix4x4 transform;
};

// Created with ReClass.NET 1.2 by KN4CK3R

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

class UISettings {
public:
	char pad_000[0x44];
	bool drawEnable;
	// static method to return the default instance, from an offset of GameTimeSettings's pointer
	static UISettings* GetInstance(void) {
		return *(UISettings**)OffsetUISettings;
	}
};

class ViewAngles
{
public:
	unsigned char Signature[12]; //0x0000
	char pad_000C[156]; //0x000C
	float Yaw; //0x00A8
	float Pitch; //0x00AC
}; //Size: 0x00B8


class Alternator
{
public:
	char pad_0000[56]; //0x0000
	class ViewAngles* Primary; //0x0038
	char pad_0040[104]; //0x0040
	class ViewAngles* Secondary; //0x00A8
	char pad_00B0[216]; //0x00B0
}; //Size: 0x0188

class LocalAimer
{
public:
	char pad_0000[152]; //0x0000
	class Alternator* alternator; //0x0098
	char pad_00A0[224]; //0x00A0
	static LocalAimer* GetInstance() {
		return *(LocalAimer**)(OffsetLocalAimer);
	}
}; //Size: 0x0180

class GameContext
{
public:
	char pad_0000[56]; //0x0000
	class ClientLevel* level; //0x0038
	char pad_0040[24]; //0x0040
	class PlayerManager* playerManager; //0x0058
	char pad_0060[8216]; //0x0060
	static GameContext* GetInstance() {
		return *(GameContext**)(OffsetGameContext);
	}
}; //Size: 0x2078

class ClientLevel
{
public:
	char pad_0000[56]; //0x0000
	char* LevelName; //0x0038
	char pad_0040[72]; //0x0040
}; //Size: 0x0088

class PlayerManager
{
public:
	char pad_0000[1384]; //0x0000
	class ClientPlayer* LocalPlayer; //0x0568
	char pad_0570[3864]; //0x0570
}; //Size: 0x1488

class ClientPlayer
{
public:
	char pad_0000[24]; //0x0000
	char* Username; //0x0018
	char pad_0020[56]; //0x0020
	uint32_t Team; //0x0058
	char pad_005C[436]; //0x005C
	class ClientSoldierEntity* controlledControllable; //0x0210
	char pad_0218[1704]; //0x0218
}; //Size: 0x08C0

class ClientSoldierEntity
{
public:
	char pad_0000[824]; //0x0000
	class SoldierBlueprint* soldierBlueprint; //0x0338
	char pad_0340[404]; //0x0340
	float HeightOffset; //0x04D4
	char pad_04D8[640]; //0x04D8
	class ClientSoldierPrediction* clientSoldierPrediction; //0x0758
	char pad_0760[1276]; //0x0760
}; //Size: 0x0C5C

class SoldierBlueprint
{
public:
	char pad_0000[24]; //0x0000
	char* SoldierName; //0x0018
	char pad_0020[96]; //0x0020
}; //Size: 0x0080

class ClientSoldierPrediction
{
public:
	char pad_0000[32]; //0x0000
	Vec3 Location; //0x0020
	char pad_002C[4]; //0x002C
	Vec3 Velocity; //0x0030
	char pad_003C[2104]; //0x003C
}; //Size: 0x0874