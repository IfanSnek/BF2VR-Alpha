#pragma once
#include "SDK.h"

#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

namespace BF2VR {

	class GameService {
	public:
		static inline Matrix4x4 Transform;
		static inline Matrix4x4 Origin;
		static inline void* RenderView = NULL;
		static inline Vec4 CameraPosition = { 0, 0, 0, 0 };
		static inline std::string Level = "";

		static inline bool UpdateAim = true;
		static inline bool UpdateLook = true;

		static bool CaptureOrigin();

		static bool HookCamera();

		static void UpdateCamera(Vec3 location, Matrix4 matrix, float yaw, float pitch);

		static void Reposition();

	private:
		static inline float AimYaw = 0;
		static inline float AimPitch = 0;
		static inline float AimYawOffset = 0;
		static inline float AimPitchOffset = 0;

		typedef __int64(Update)(class CameraObject*, class CameraObject*);
		static Update UpdateDetour;
		static inline Update* UpdateOriginal = nullptr;
	};
}