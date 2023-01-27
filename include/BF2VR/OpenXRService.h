#pragma once

#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_PLATFORM_WIN32

#include <d3d11.h>
#include <map>
#include <vector>
#include <array>

#include "../../third-party/OpenXR/openxr.h"
#include "../../third-party/OpenXR/openxr_platform.h"
#include "../../third-party/OpenXR/xrmath.h"

#include "SDK.h"

namespace BF2VR {
	class OpenXRService {
	public:
		static inline XrInstance xrInstance;
		static inline XrSession xrSession;
		static inline std::vector<XrSwapchain> xrSwapchains;
		static inline std::map<uint32_t, std::vector<ID3D11RenderTargetView*>> xrRTVs;

		static inline int EyeWidth;
		static inline int EyeHeight;

		static inline bool LeftEye = true;
		static inline bool VRReady = false;

		static inline bool Firing = false;
		static inline bool LeftHanded = false;
		static inline bool Menu = true;
		static inline bool pressingMenu = false;
		static inline bool pressingW = false;
		static inline bool pressingA = false;
		static inline bool pressingS = false;
		static inline bool pressingD = false;

		static bool CreateXRInstanceWithExtensions();
		static bool BeginXRSession(ID3D11Device* device);
		static bool PrepareActions();
		static bool BeginFrameAndGetVectors(Vec3& Loc, Vec4& Rot, Matrix4 outMatrix);
		static bool SubmitFrame(ID3D11Texture2D* texture);
		static bool UpdateActions();
		static bool UpdatePoses();
		static bool EndFrame();
		static void EndXR();

	private:

		// CreateXRInstanceWithExtensions
		static inline XrEnvironmentBlendMode xrBlend = {};

		// BeginXRSession
		static inline XrSystemId xrSystemId = XR_NULL_SYSTEM_ID;
		static inline XrSpace xrAppSpace = {};
		static inline uint32_t xrViewCount = 0;
		static inline std::vector<XrViewConfigurationView> xrConfigViews;
		static inline std::vector<XrView> xrViews;

		// PrepareActions
		static inline XrActionSet actionSet;

		static inline std::array<XrPath, 2> handPaths;
		static inline std::array<XrPath, 2> triggerPaths;
		static inline XrPath menuPath;
		static inline XrPath walkPath;

		static inline XrAction poseAction;
		static inline XrAction triggerAction;
		static inline XrAction menuAction;
		static inline XrAction walkAction;

		static inline XrSpace pose_action_spaces[2];

		// Action values
		static inline XrSpaceLocation hand_locations[2];
		static inline XrActionStateFloat grab_value[2];
		static inline XrActionStateBoolean menu_value;
		static inline XrActionStateVector2f walk_value;

		// BeginFrameAndGetVectors
		static inline XrFrameState xrFrameState = {};
		static inline uint32_t xrProjectionViewCount = 0;
		static inline std::vector<XrCompositionLayerProjectionView> xrProjectionViews;
	};
}