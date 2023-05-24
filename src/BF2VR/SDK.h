// SDK.h - Reverse engineered classes and pointer offsets for Battlefront II
// as well as some utilities to access classes at runtime.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#pragma once
#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include "Types.h"
#include <iostream>

static const DWORD64 OFFSETGAMECONTEXT			  = 0x143DD7948;
static const DWORD64 OFFSETLOCALAIMER			  = 0x14406E610;
static const DWORD64 OFFSETGAMERENDERER		      = 0x143ffbe10;
static const DWORD64 OFFSETUISETTINGS			  = 0x143aebb80;
static const DWORD64 OFFSETWORLDRENDERSETTINGS	  = 0x143D7B068;

static const DWORD64 OFFSETCAMERA			      = 0x1410c7010;
static const DWORD64 OFFSETPOSE					  = 0x142150910;

static inline bool isValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr && !IsBadReadPtr(p, sizeof(PVOID)); }

// https://github.com/onra2/swbf2-internal/blob/master/swbf2%20onra2/Classes.h

struct typeInfoMemberResult {
	void* pVTable;
	const char* name;
	DWORD offset;
};

inline std::vector<typeInfoMemberResult> typeInfoMemberResults;

template <class T = void*>
T GetClassFromName(void* addr, const char* name, SIZE_T classSize = 0x2000) {

	for (typeInfoMemberResult& result : typeInfoMemberResults) {
		if (result.pVTable == addr) {
			if (result.name == name) {
				return *(T*)((DWORD64)addr + result.offset);
			}
		}
	}

	const byte INSTR_LEA = 0x48;
	const byte INSTR_RET = 0xc3;
	const byte INSTR_JMP = 0xe9;
	const DWORD64 BASE_ADDRESS = 0x140000000;
	const DWORD64 MAX_ADDRESS = 0x14fffffff;

	DWORD offset = 0;
	DWORD lastOffset = 0;
	while (offset < classSize) {
		offset += 8;
		if (!isValidPtr((void*)((DWORD64)addr + offset))) continue;
		void* czech = *(void**)((DWORD64)addr + offset);
		if (!isValidPtr(czech)) continue;
		if (!isValidPtr(*(void**)czech)) continue; // vtable
		if (!isValidPtr(**(void***)czech)) continue; // virtual 1;
		void* pGetType = **(DWORD64***)czech;
		if ((DWORD64)pGetType < BASE_ADDRESS || (DWORD64)pGetType > MAX_ADDRESS) continue;

		if (*(byte*)pGetType == INSTR_JMP || *(byte*)pGetType == INSTR_LEA) {
			void* pTypeInfo = nullptr;
			if (*(byte*)pGetType == INSTR_JMP) {
				pGetType = (void*)(*(int32_t*)((DWORD64)pGetType + 1) + (DWORD64)pGetType + 5);
			}
			if (*(byte*)pGetType == INSTR_LEA) {
				if (*(byte*)((DWORD64)pGetType + 7) != INSTR_RET) continue;
				pTypeInfo = (void*)(*(int32_t*)((DWORD64)pGetType + 3) + (DWORD64)pGetType + 7);
			}
			else continue;
			if (!isValidPtr(pTypeInfo)) continue;
			void* pMemberInfo = *(void**)pTypeInfo;
			if (!isValidPtr(pMemberInfo)) continue;
			char* m_name = *(char**)pMemberInfo;
			if (!isValidPtr(m_name)) continue;
			if ((DWORD64)pTypeInfo > BASE_ADDRESS && (DWORD64)pTypeInfo < MAX_ADDRESS) {
				if (strcmp(m_name, name) == 0) {
					typeInfoMemberResult result;
					result.name = name;
					result.offset = offset;
					result.pVTable = addr;
					typeInfoMemberResults.push_back(result);
					return *(T*)((DWORD64)addr + offset);
				}

				lastOffset = offset;
			}
		}
	}
	return nullptr;
}

///////////////////////
// Camera
///////////////////////

class WorldRenderSettings {
public:
	// TODO: Put stuff here

	static WorldRenderSettings* GetInstance(void) {
		return *(WorldRenderSettings**)OFFSETWORLDRENDERSETTINGS;
	}
};

class RenderView {
public:
	Matrix4 transform; //0x0000
	char pad_0040[112]; //0x0040
	float FOV; //0x00B0
	char pad_00B4[20]; //0x00B4
	float nearPlane; //0x00C8
	float farPlane; //0x00CC
	float aspectRatio; //0x00D0
	float orthoWidth; //0x00D4
	float orthoHeight; //0x00D8

};

class CameraObject {
public:
	Matrix4 cameraTransform;
};

class GameRenderer {
public:
	char pad_0000[1304]; //0x0000
	class GameRenderSettings* gameRenderSettings; //0x0510
	char pad_0520[24]; //0x0520
	class RenderView* renderView; //0x0538
	char pad_0540[4872]; //0x0540

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

	static UISettings* GetInstance(void) {
		return *(UISettings**)OFFSETUISETTINGS;
	}
};

///////////////////////
// Aiming
///////////////////////

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


///////////////////////
// Client
///////////////////////

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

///////////////////////
// Vehicle
///////////////////////

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
			
		}

		return Vec3(0, 0, 0);
	}

}; //Size: 0x1058

///////////////////////
// Pose
///////////////////////

class QuatTransform
{
public:
	Vec4 Scale; //0x0000
	Vec4 Quat; //0x0010
	Vec4 Translation; //0x0020
}; //Size: 0x0030

class QuatTransformArray
{
public:
	class QuatTransform transform[98]; //0x0000
}; //Size: 0x1260

class UpdatePoseResultData
{
public:
	class QuatTransformArray* pArray; //0x0000
}; //Size: 0x0008

class ClientBoneCollisionComponent
{
public:
	char pad_0000[72]; //0x0000
	class AnimationSkeleton* skeleton; //0x0048
	class UpdatePoseResultData pose; //0x0050
	char pad_0058[8]; //0x0058
}; //Size: 0x0060

class AnimationSkeleton
{
public:
	char pad_0000[8]; //0x0000
	class SkeletonAsset* asset; //0x0008
	int32_t boneCount; //0x0010
}; //Size: 0x0014

class SkeletonAsset
{
public:
	char pad_0000[144]; //0x0000
	char* boneNames[98]; //0x0090
}; //Size: 0x03A0

struct Bone
{
	int id = 0;
	QuatTransform* transform = new QuatTransform;
	QuatTransform* originalTransfrom = new QuatTransform;
	std::vector<Bone*> children;
};

class BasicSkeleton
{
public:
	bool valid = false;
	int boneCount = 0;

	BasicSkeleton(ClientBoneCollisionComponent* clientComponent)
	{
		if (isValidPtr(clientComponent))
		{
			component = clientComponent;
			animationSkeleton = clientComponent->skeleton;
			if (isValidPtr(animationSkeleton))
			{
				boneCount = animationSkeleton->boneCount;
				asset = animationSkeleton->asset;
				if (asset != nullptr)
				{
					valid = true;

					if (!buildSkeleton())
						valid = false;

					return;
				}
			}
		}
	}

	int boneIdFromName(const char* name)
	{
		if (!updateValidity())
			return -1;

		int boneIndex = -1;
		for (int i = 0; i < animationSkeleton->boneCount; i++) {
			if (strcmp(asset->boneNames[i], name) == 0)
			{
				boneIndex = i;
			}
		}
		return boneIndex;
	}

	bool boneRelative(const char* name, QuatTransform** transfrom)
	{
		if (!updateValidity())
			return false;

		int boneId = boneIdFromName(name);
		if (boneId == -1)
			return false;

		*transfrom = &component->pose.pArray->transform[boneId];

		return true;
	}
	bool boneRelative(int boneId, QuatTransform** transfrom)
	{
		if (!updateValidity())
			return false;

		*transfrom = &component->pose.pArray->transform[boneId];

		return true;
	}

	bool poseBoneRelative(const char* name, Vec3 location, Vec4 rotation, Vec3 scale)
	{

		QuatTransform* transfrom = nullptr;

		if (!boneRelative(name, &transfrom))
			return false;

		transfrom->Translation = Vec4(location.x, location.y, location.z, 0);
		if (rotation.x != 0 && rotation.y != 0 && rotation.z != 0 && rotation.w != 1)
			transfrom->Quat = rotation;
		transfrom->Scale = Vec4(scale.x, scale.y, scale.z, 0);
		
		return true;
	}
	bool poseBoneRelative(int boneId, Vec3 location, Vec4 rotation, Vec3 scale)
	{

		QuatTransform* transfrom = nullptr;

		if (!boneRelative(boneId, &transfrom))
			return false;

		transfrom->Translation = Vec4(location.x, location.y, location.z, 0);
		if (rotation.x != 0 && rotation.y != 0 && rotation.z != 0 && rotation.w != 1)
			transfrom->Quat = rotation;
		transfrom->Scale = Vec4(scale.x, scale.y, scale.z, 0);

		return true;
	}

	bool getBoneRelative(const char* name, Vec3 &location, Vec4 &rotation, Vec3 &scale)
	{
		QuatTransform* transfrom = nullptr;

		if (!boneRelative(name, &transfrom))
			return false;

		location = transfrom->Translation.dropW();
		rotation = transfrom->Quat;
		scale = transfrom->Scale.dropW();

		return true;
	}

	// Pose a bone in absolute local space.

	bool poseBone(const char* name, Vec3 location, Vec4 rotation, Vec3 scale)
	{
		if (!updateValidity())
			return false;

		int boneId = boneIdFromName(name);
		if (boneId == -1)
			return false;

		return poseBoneRelative(boneId, location, rotation, scale);
	}

private:
	bool addBone(const char* name, Bone* outBone, Bone* parent)
	{
		if (!updateValidity())
			return false;

		int boneId = boneIdFromName(name);
		if (boneId == -1)
			return false;

		outBone->id = boneId;
		if (!boneRelative(name, &outBone->transform))
			return false;
		if (!boneRelative(name, &outBone->originalTransfrom))
			return false;

		parent->children.push_back(outBone);

		return true;
	}
	bool buildSkeleton()
	{
		if (!updateValidity())
			return false;

		int rootId = boneIdFromName("AITrajectory");
		if (rootId == -1)
			return false;

		root->id = rootId;
		if (!boneRelative("AITrajectory", &root->transform))
			return false;
		if (!boneRelative("AITrajectory", &root->originalTransfrom))
			return false;

		// Fill in children

		Bone* hips = new Bone;
		if (!addBone("Hips", hips, root))
			return false;

		Bone* spine = new Bone;
		if (!addBone("Spine", spine, hips))
			return false;

		Bone* spine1 = new Bone;
		if (!addBone("Spine1", spine1, spine))
			return false;

		Bone* spine2 = new Bone;
		if (!addBone("Spine2", spine2, spine1))
			return false;

		Bone* leftShoulder = new Bone;
		if (!addBone("LeftShoulder", leftShoulder, spine2))
			return false;
		Bone* rightShoulder = new Bone;
		if (!addBone("RightShoulder", rightShoulder, spine2))
			return false;

		Bone* leftArm = new Bone;
		if (!addBone("LeftArm", leftArm, leftShoulder))
			return false;
		Bone* rightArm = new Bone;
		if (!addBone("RightArm", rightArm, rightShoulder))
			return false;

		Bone* leftForeArm = new Bone;
		if (!addBone("LeftForeArm", leftForeArm, leftArm))
			return false;
		Bone* rightForeArm = new Bone;
		if (!addBone("RightForeArm", rightForeArm, rightArm))
			return false;

		Bone* leftHand = new Bone;
		if (!addBone("LeftHand", leftHand, leftForeArm))
			return false;
		Bone* rightHand = new Bone;
		if (!addBone("RightHand", rightHand, rightForeArm))
			return false;

		Bone* weapon = new Bone;
		if (!addBone("Wep_Root", weapon, rightHand))
			return false;

		return true;
	}

	bool updateValidity()
	{
		if (isValidPtr(component))
		{
			if (isValidPtr(animationSkeleton))
			{
				if (isValidPtr(asset))
				{
					valid = true;
					return valid;
				}
			}

		}
		valid = false;
		return valid;
	}

	Bone* root = new Bone;

	ClientBoneCollisionComponent* component = nullptr;
	AnimationSkeleton* animationSkeleton = nullptr;
	SkeletonAsset* asset = nullptr;
};

///////////////////////
// Player
///////////////////////

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