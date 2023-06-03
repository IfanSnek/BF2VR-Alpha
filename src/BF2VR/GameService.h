// GameService.h - Headers for the code that interacts with the reverse 
// engineered game memory structure. It also implements hooks into some
// game functions.
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
#include "SDK.h"

#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

namespace BF2VR {
	class GameService {
	public:
		static inline char* level = nullptr;

		static void updateCamera(Vec3 cameraLocation, Matrix4 cameraViewMatrix, float aimYaw, float aimPitch);

		static void updateBone(const char* boneName, Vec3 location = Vec3(0, 0, 0), Vec4 rotation = Vec4(0, 0, 0, 1));

		static bool applyBones();

		static bool enableHooks();

		static void setUIDrawState(bool enabled);

		static bool worldToScreen(Vec3 world, Vec3& screen);

	private:
		typedef struct boneState
		{
			const char* boneName;
			Vec3 location;
			Vec4 rotation;
			Vec3 scale;
		} boneState;

		static inline Matrix4 cameraTransfrom;
		static inline std::vector<boneState> boneStates;

		// Camera update hook
		typedef __int64(CameraUpdate)(class CameraObject*, class CameraObject*);
		static CameraUpdate cameraUpdateDetour;
		static inline CameraUpdate* cameraUpdateOriginal = nullptr;
		static inline void* pRenderView = nullptr;

		// Pose update hook
		typedef __int64(PoseUpdate)(int a1, int a2, int a3, int a4, __int64 a5);
		static PoseUpdate poseUpdateDetour;
		static inline PoseUpdate* poseUpdateOriginal = nullptr;
		static inline bool isPosing = false;
	};
}