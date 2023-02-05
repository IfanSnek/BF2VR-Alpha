#pragma once
#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "Matrices.h"
#include "Types.h"

inline bool isValidRange(DWORD64 p) { return (p >= 0x10000) && (p < 0x000F000000000000); }


static const DWORD64 OFFSETGAMECONTEXT  = 0x143DD7948;
static const DWORD64 OFFSETLOCALAIMER   = 0x14406E610;
static const DWORD64 OFFSETGAMERENDERER = 0x143ffbe10;
static const DWORD64 OFFSETUISETTINGS   = 0x143aebb80;
static const DWORD64 OFFSETCAMERA       = 0x1410c7010;

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
		return *(GameRenderer**)OFFSETGAMERENDERER;
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
		return *(UISettings**)OFFSETUISETTINGS;
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
		return *(LocalAimer**)(OFFSETLOCALAIMER);
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
		return *(GameContext**)(OFFSETGAMECONTEXT);
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

class VehicleEntityData
{
public:
	char pad_0000[0x120]; //0x0000
	Vec3 interactionOffset; //0x0120
	Vec3 m_VictimOffsetOverride; //0x0130
	char* VehicleName; //0x0238
	char pad_0240[520]; //0x0240

	char* GetName() {
		if (this != nullptr && this->VehicleName != nullptr) {
			return this->VehicleName;
		}
		return (char*)"\0";
	}
};

class VehicleLocation
{
public:
	DWORD64 classcheck; //0x0000
	char pad_0008[88]; //0x0008
	Vec3 Velocity; //0x0060  <-don't use
	char pad_006C[4]; //0x006C
	Vec3 Location; //0x0070
}; //Size: 0x0168


class AttachedControllable
{
public:
	char pad_0000[48]; //0x0000
	class VehicleEntityData* vehicleEntity; //0x0030
	char pad_0038[1592]; //0x0038
	DWORD64 loc1; //0x0670
	char pad_0678[24]; //0x0678
	DWORD64 loc2; //0x0690
	char pad_0698[24]; //0x0698
	DWORD64 loc3; //0x06B0
	char pad_06B8[24]; //0x06B8
	DWORD64 loc4; //0x06D0
	char pad_06D8[24]; //0x06D8
	DWORD64 loc5; //0x06F0

	VehicleEntityData* GetVehicleEntityData() {
		if (this != nullptr && this->vehicleEntity != nullptr) {
			return this->vehicleEntity;
		}
	}

	Vec3 GetVehicleLocation()
	{
		DWORD64 sig = 5416674584;

		if (this != nullptr && this->vehicleEntity != nullptr) {

			VehicleLocation* location = (VehicleLocation*)this->loc1;
			if (location != nullptr)
			{
				if (location->classcheck == sig)
				{
					return location->Location;
				}
			}

			
			location = (VehicleLocation*)this->loc2;
			if (location != nullptr)
			{
				if (location->classcheck == sig)
				{
					return location->Location;
				}
			}
			

			location = (VehicleLocation*)this->loc3;
			if (location != nullptr)
			{
				if (location->classcheck == sig)
				{
					return location->Location;
				}
			}
			

			location = (VehicleLocation*)this->loc4;
			if (location != nullptr)
			{
				if (location->classcheck == sig)
				{
					return location->Location;
				}
			}
			

			location = (VehicleLocation*)this->loc5;
			if (location != nullptr)
			{
				if (location->classcheck == sig)
				{
					return location->Location;
				}
			}
			

			return Vec3(0, 0, 0);
		}
	}

}; //Size: 0x1058


class ClientPlayer
{
public:
	char pad_0000[24]; //0x0000
	char* Username; //0x0018
	char pad_0020[56]; //0x0020
	uint32_t Team; //0x0058
	char pad_005C[420]; //0x005C
	class AttachedControllable* attachedControllable; //0x0200
	char pad_0208[8]; //0x0208
	class ClientSoldierEntity* controlledControllable; //0x0210
	char pad_0218[1704]; //0x0218
}; //Size: 0x08C0

class UpdatePoseResultData
{
public:
	class QuatTransform
	{
	public:
		Vec4 m_TransAndScale; //0x0000 
		Vec4 m_Rotation; //0x0010 
	};//Size=0x0020

	QuatTransform* m_LocalTransforms; //0x0000 
	QuatTransform* m_WorldTransforms; //0x0008 
	QuatTransform* m_Unk; //0x0010 
	QuatTransform* m_Unk1; //0x0018 
	QuatTransform* m_Unk2; //0x0020 
	QuatTransform* m_ActiveWorldTransforms; //0x0028 
	QuatTransform* m_ActiveLocalTransforms; //0x0030 
	__int32 m_Slot; //0x0038 
	__int32 m_ReaderIndex; //0x003C 
	unsigned char m_ValidTransforms; //0x0040 
	unsigned char m_PoseUpdateNeeded; //0x0041 
	unsigned char m_PoseNeeded; //0x0042 
};

enum HumanBones
{
	Head = 48,
	Neck = 46,
	Spine = 5,
	Spine1 = 6,
	Spine2 = 7,
	LeftShoulder = 8,
	LeftElbowRoll = 13,
	RightShoulder = 144,
	RightElbowRoll = 149,
	LeftHand = 17,
	RightHand = 153,
	RightKneeRoll = 235,
	LeftKneeRoll = 223,
	RightFoot = 228,
	LeftFoot = 216
};

class SkeletonAsset
{
public:
	char pad_0000[24]; //0x0000
	char* CharacterType; //0x0018
	char** BoneNames; //0x0020
};

class AnimationSkeleton
{
public:
	char pad_0000[8]; //0x0000
	SkeletonAsset* skeletonAsset; //0x0008
	__int32 m_BoneCount; //0x0010
};

class ClientBoneCollisionComponent
{
public:
	UpdatePoseResultData m_ragdollTransforms; //0x0000
	char pad_0008[64]; //0x0008
	AnimationSkeleton* animationSkeleton; //0x0048
};

class WSClientSoldierEntity
{
public:
};

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