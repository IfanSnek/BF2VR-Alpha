#pragma once
#include "SDK.h"

#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

namespace BF2VR {

	class GameService {
	public:
		static inline Matrix4 cameraTransfrom;

		static inline void* pRenderView = NULL;
		static inline Vec4 cameraPosition = { 0, 0, 0, 0 };
		static inline char* level = nullptr;

		static inline Vec3 aimLoc = Vec3(0,0,0);
		static inline Vec4 aimQuat = Vec4(0,0,0,0);

		static void updateCamera(Vec3 cameraLocation, Matrix4 cameraViewMatrix, float aimYaw, float aimPitch);

		static void updateBone(const char* boneName, Vec3 location = Vec3(0, 0, 0), Vec4 rotation = Vec4(0, 0, 0, 1));

		static bool enableHooks();

		static void setUIDrawState(bool enabled);

	private:

		static inline bool isPosing = false;

		typedef __int64(CameraUpdate)(class CameraObject*, class CameraObject*);
		static CameraUpdate cameraUpdateDetour;
		static inline CameraUpdate* cameraUpdateOriginal = nullptr;

		typedef struct boneState
		{
			const char* boneName;
			Vec3 location;
			Vec4 rotation;
			Vec3 scale;
		} boneState;

		static inline std::vector<boneState> boneStates;

		typedef __int64(PoseUpdate)(int a1, int a2, int a3, int a4, __int64 a5);
		static PoseUpdate poseUpdateDetour;
		static inline PoseUpdate* poseUpdateOriginal = nullptr;
	};
}