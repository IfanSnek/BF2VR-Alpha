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

		static inline int eyeWidth;
		static inline int eyeHeight;

		static inline bool onLeftEye = true;
		static inline bool isVRReady = false;

		static inline bool isFiring = false;
		static inline bool showUI = true;
		static inline bool isPressingMenu = false;

		static bool createXRInstanceWithExtensions();
		static bool beginXRSession(ID3D11Device* pDevice);
		static bool prepareActions();
		static bool beginFrameAndGetVectors(Vec3& outHeadLoc, Vec4& outHeadRot, Matrix4 outHeadMatrix);
		static bool submitFrame(ID3D11Texture2D* pTexture);
		static bool updateActions();
		static bool updatePoses();
		static bool endFrame();
		static void endXR();

	private:

		static inline float HALFPI = 1.570796f;
		static inline float fixRot(float x) { return (x > HALFPI) ? (-HALFPI + (x - HALFPI)) : ((x < HALFPI) ? (HALFPI + (x + HALFPI)) : x); }

		static inline float TP_OFFSET = .5f;

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
		static inline std::array<XrPath, 2> gripPaths;
		static inline XrPath menuPath;
		static inline XrPath walkPath;
		static inline XrPath rollPath;
		static inline XrPath jumpPath;
		static inline XrPath reloadPath;

		static inline XrAction poseAction;
		static inline XrAction triggerAction;
		static inline XrAction gripAction;
		static inline XrAction menuAction;
		static inline XrAction walkAction;
		static inline XrAction rollAction;
		static inline XrAction jumpAction;
		static inline XrAction reloadAction;

		static inline XrSpace poseActionSpaces[2];

		// Action values
		static inline XrSpaceLocation handLocations[2];
		static inline XrActionStateFloat grabValue[2];
		static inline XrActionStateFloat gripValue[2];
		static inline XrActionStateBoolean menuValue;
		static inline XrActionStateVector2f walkValue;
		static inline XrActionStateBoolean rollValue;
		static inline XrActionStateBoolean jumpValue;
		static inline XrActionStateBoolean reloadValue;

		// BeginFrameAndGetVectors
		static inline XrFrameState xrFrameState = {};
		static inline uint32_t xrProjectionViewCount = 0;
		static inline std::vector<XrCompositionLayerProjectionView> xrProjectionViews;
	};
}