#pragma once
#include "SDK.h"

#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

namespace BF2VR {

	class GameService {
	public:
		static inline Matrix4x4 cameraTransfrom;
		static inline void* pRenderView = NULL;
		static inline Vec4 cameraPosition = { 0, 0, 0, 0 };
		static inline char* level = nullptr;

		static bool hookCamera();

		static void updateCamera(Vec3 cameraLocation, Matrix4 cameraViewMatrix, float aimYaw, float aimPitch, Vec3 gunPos, Vec4 gunRot);

		static void setUIDrawState(bool enabled);

	private:

		typedef __int64(CameraUpdate)(class CameraObject*, class CameraObject*);
		static CameraUpdate cameraUpdateDetour;
		static inline CameraUpdate* updateOriginal = nullptr;
	};
}