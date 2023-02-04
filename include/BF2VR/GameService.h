#pragma once
#include "SDK.h"

#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

namespace BF2VR {

	class GameService {
	public:
		static inline Matrix4x4 Transform;
		static inline void* RenderView = NULL;
		static inline Vec4 CameraPosition = { 0, 0, 0, 0 };
		static inline char* Level = nullptr;

		static bool HookCamera();

		static void UpdateCamera(Vec3 location, Matrix4 matrix, float yaw, float pitch, Vec3 gunPos, Vec4 gunRot);

		static void SetMenu(bool enabled);

	private:

		typedef __int64(Update)(class CameraObject*, class CameraObject*);
		static Update UpdateDetour;
		static inline Update* UpdateOriginal = nullptr;
	};
}