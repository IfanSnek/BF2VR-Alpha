#include "OpenXRService.h"
#include "InputService.h"

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

#include "DirectXService.h"
#include "GameService.h"
#include "Utils.h"
#include "SDKOLD.h"

namespace BF2VR {
    bool OpenXRService::CreateXRInstanceWithExtensions() {

        // Enable extensions
        std::vector<const char*> enabledExtensions;
        const char* requiredExtensions[] = {
            XR_KHR_D3D11_ENABLE_EXTENSION_NAME
        };

        uint32_t extensionCount = 0;
        XrResult xr = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
        if (xr != XR_SUCCESS) {
            error("Could not query OpenXR extension count.");
            return false;
        }

        std::vector<XrExtensionProperties> allExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
        xr = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, allExtensions.data());

        if (xr != XR_SUCCESS) {
            error("Could not query OpenXR extensions.");
            return false;
        }

        for (auto& allExtension : allExtensions) {
            for (auto& requiredExtension : requiredExtensions) {
                if (strcmp(requiredExtension, allExtension.extensionName) == 0) {
                    enabledExtensions.push_back(requiredExtension);
                    break;
                }
            }
        }

        if (std::find(enabledExtensions.begin(), enabledExtensions.end(), XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == enabledExtensions.end())  {
            error("The active OpenXR runtime does not support D3D11.");
            return false;
        }

        XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        createInfo.enabledExtensionNames = enabledExtensions.data();
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strcpy_s(createInfo.applicationInfo.applicationName, "Star Wars Battlefront II VR");
        xr = xrCreateInstance(&createInfo, &xrInstance);
        if (xr != XR_SUCCESS && xr != XR_ERROR_RUNTIME_FAILURE) {
            error("Failed to create OpenXR xrInstance. Error: " + std::to_string(xr));
            return false;

        }
        else if (xr == XR_ERROR_RUNTIME_FAILURE)
        {
            error("Failed to create OpenXR xrInstance. Is your runtime (eg. SteamVR) running first?");
            return false;
        }

        // Load D3D11 extension

        PFN_xrGetD3D11GraphicsRequirementsKHR xrExtGetD3D11GraphicsRequirements = nullptr;
        xr = xrGetInstanceProcAddr(xrInstance, "xrGetD3D11GraphicsRequirementsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&xrExtGetD3D11GraphicsRequirements));
        if (xr != XR_SUCCESS) {
            error("Could not get address for xrGetD3D11GraphicsRequirementsKHR");
            return false;

        }

        XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        xr = xrGetSystem(xrInstance, &systemInfo, &xrSystemId);
        if (xr != XR_SUCCESS) {
            error("Could not get XR system. Error: " + std::to_string(xr));
            return false;

        }

        uint32_t blendCount = 0;
        xr = xrEnumerateEnvironmentBlendModes(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 1, &blendCount, &xrBlend);
        if (xr != XR_SUCCESS) {
            error("Could not get XR blend modes. Error: " + std::to_string(xr));
            return false;

        }

        XrGraphicsRequirementsD3D11KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
        xr = xrExtGetD3D11GraphicsRequirements(xrInstance, xrSystemId, &graphicsRequirements);

        if (xr != XR_SUCCESS) {
            error("Could not get XR graphic requirements. Error: " + std::to_string(xr));
            return false;

        }

        return true;
    }

    bool OpenXRService::BeginXRSession(ID3D11Device* device) {

        XrGraphicsBindingD3D11KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
        graphicsBinding.device = device;
        XrSessionCreateInfo xrSessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
        xrSessionInfo.next = &graphicsBinding;
        xrSessionInfo.systemId = xrSystemId;
        XrResult xr = xrCreateSession(xrInstance, &xrSessionInfo, &xrSession);
        if (xr != XR_SUCCESS) {
            error("Failed to create OpenXR xrSession. Error: " + std::to_string(xr));
            return false;
        }

        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.poseInReferenceSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        xr = xrCreateReferenceSpace(xrSession, &spaceInfo, &xrAppSpace);
        if (xr != XR_SUCCESS) {
            error("Could not create OpenXR reference space. Error: " + std::to_string(xr));
            return false;
        }

        // Set eye resolution

        xr = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &xrViewCount, nullptr);
        if (xr != XR_SUCCESS) {
            error("Could not get OpenXR view configuration view count. Error: " + std::to_string(xr));
            return false;
        }

        xrConfigViews.resize(xrViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xrViews.resize(xrViewCount, { XR_TYPE_VIEW });

        xr = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, xrViewCount, &xrViewCount, xrConfigViews.data());
        if (xr != XR_SUCCESS) {
            error("Could not enumerate OpenXR view configuration views. Error: " + std::to_string(xr));
            return false;
        }

        // Create XR Swapchains

        for (uint32_t i = 0; i < xrViewCount; i++) {

            XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            XrSwapchain swapchain;
            swapchainInfo.arraySize = 1;
            swapchainInfo.mipCount = 1;
            swapchainInfo.faceCount = 1;
            swapchainInfo.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            swapchainInfo.width = EyeWidth;
            swapchainInfo.height = EyeHeight;
            swapchainInfo.sampleCount = 1;
            swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

            XrResult result = xrCreateSwapchain(xrSession, &swapchainInfo, &swapchain);
            if (xr != XR_SUCCESS) {
                error("Could not create XR swapchain. Error: " + std::to_string(xr));
                return false;
            }

            uint32_t surfaceCount;
            xr = xrEnumerateSwapchainImages(swapchain, 0, &surfaceCount, nullptr);
            if (xr != XR_SUCCESS) {
                error("Could not enumerate OpenXR swapchain image count. Error: " + std::to_string(xr));
                return false;
            }

            std::vector<XrSwapchainImageD3D11KHR> surfaces;
            surfaces.resize(surfaceCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
            xr = xrEnumerateSwapchainImages(swapchain, surfaceCount, &surfaceCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(surfaces.data()));
            if (xr != XR_SUCCESS) {
                error("Could not enumerate OpenXR swapchain images Error: " + std::to_string(xr));
                return false;
            }

            std::vector<ID3D11RenderTargetView*> rtvs;
            for (const auto& surface : surfaces) {

                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
                ID3D11RenderTargetView* rtv;
                HRESULT hr = DirectXService::pDevice->CreateRenderTargetView(surface.texture, &rtvDesc, &rtv);
                if (hr != S_OK) {
                    error("Failed to create an RTV for OpenXR. Error: " + std::to_string(xr));
                    return false;
                }
                rtvs.push_back(rtv);
            }
            xrSwapchains.push_back(swapchain);
            xrRTVs.insert(std::pair(i, rtvs));
        }

        // Begin OpenXR xrSession
        XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        xr = xrBeginSession(xrSession, &beginInfo);
        if (xr != XR_SUCCESS) {
            error("Could not begin XR xrSession. Error: " + std::to_string(xr));
            return false;
        }

        // Configure actions
        PrepareActions();

        VRReady = true;
        return true;
    }

    bool OpenXRService::PrepareActions() {

        log("Preparing VR Actions ...");

        // Create main action set

        XrActionSetCreateInfo actionSetInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
        actionSetInfo.priority = 0;
        strcpy(actionSetInfo.actionSetName, "gameplay");
        strcpy(actionSetInfo.localizedActionSetName, "Gameplay");

        XrResult xr = xrCreateActionSet(xrInstance, &actionSetInfo, &actionSet);
        if (xr != XR_SUCCESS) {
            warn("Action set not created, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }


        // Hands pose action

        xrStringToPath(xrInstance, "/user/hand/left", &handPaths[0]);
        xrStringToPath(xrInstance, "/user/hand/right", &handPaths[1]);
        {

            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
            actionInfo.countSubactionPaths = 2;
            actionInfo.subactionPaths = handPaths.data();
            strcpy(actionInfo.actionName, "handpose");
            strcpy(actionInfo.localizedActionName, "Hand Pose");
            xr = xrCreateAction(actionSet, &actionInfo, &poseAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Hand click action

        xrStringToPath(xrInstance, "/user/hand/left/input/trigger/value", &triggerPaths[0]);
        xrStringToPath(xrInstance, "/user/hand/right/input/trigger/value", &triggerPaths[1]);
        {

            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
            actionInfo.countSubactionPaths = 2;
            actionInfo.subactionPaths = handPaths.data();
            strcpy(actionInfo.actionName, "firefloat");
            strcpy(actionInfo.localizedActionName, "Fire Gun");
            xr = xrCreateAction(actionSet, &actionInfo, &triggerAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for trigger, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Hand grip

        xrStringToPath(xrInstance, "/user/hand/left/input/squeeze/value", &gripPaths[0]);
        xrStringToPath(xrInstance, "/user/hand/right/input/squeeze/value", &gripPaths[1]);
        {

            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
            actionInfo.countSubactionPaths = 2;
            actionInfo.subactionPaths = handPaths.data();
            strcpy(actionInfo.actionName, "grip");
            strcpy(actionInfo.localizedActionName, "Left Right and Middle Abilities");
            xr = xrCreateAction(actionSet, &actionInfo, &gripAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for grip, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Menu click action
        xrStringToPath(xrInstance, "/user/hand/left/input/x/click", &menuPath);
        {
            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            actionInfo.countSubactionPaths = 1;
            actionInfo.subactionPaths = &handPaths[0];
            strcpy(actionInfo.actionName, "menu");
            strcpy(actionInfo.localizedActionName, "Toggle Menu");
            xr = xrCreateAction(actionSet, &actionInfo, &menuAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for menu, menu mode will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Battle roll action
        xrStringToPath(xrInstance, "/user/hand/right/input/b/click", &rollPath);
        {
            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            actionInfo.countSubactionPaths = 1;
            actionInfo.subactionPaths = &handPaths[1];
            strcpy(actionInfo.actionName, "roll");
            strcpy(actionInfo.localizedActionName, "Battle Roll");
            xr = xrCreateAction(actionSet, &actionInfo, &rollAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for roll, battle roll will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Jump Action
        xrStringToPath(xrInstance, "/user/hand/right/input/a/click", &jumpPath);
        {
            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            actionInfo.countSubactionPaths = 1;
            actionInfo.subactionPaths = &handPaths[1];
            strcpy(actionInfo.actionName, "jump");
            strcpy(actionInfo.localizedActionName, "Jump");
            xr = xrCreateAction(actionSet, &actionInfo, &jumpAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for jump, jumping will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Walking (left thumbstick) atcion
        xrStringToPath(xrInstance, "/user/hand/left/input/thumbstick", &walkPath);
        {
            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
            actionInfo.countSubactionPaths = 1;
            actionInfo.subactionPaths = &handPaths[0];
            strcpy(actionInfo.actionName, "walk");
            strcpy(actionInfo.localizedActionName, "Walk");
            xr = xrCreateAction(actionSet, &actionInfo, &walkAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for menu, menu mode will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        // Suggest interaction profile

        XrPath grip_pose_path[2];
        xrStringToPath(xrInstance, "/user/hand/left/input/grip/pose", &grip_pose_path[0]);
        xrStringToPath(xrInstance, "/user/hand/right/input/grip/pose", &grip_pose_path[1]);


        XrPath interaction_profile_path;
        xr = xrStringToPath(xrInstance, "/interaction_profiles/oculus/touch_controller", &interaction_profile_path);
        if (xr != XR_SUCCESS) {
            warn("Failed to get interaction profile, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        std::vector<XrActionSuggestedBinding> bindings;
        XrActionSuggestedBinding binding;
        binding.action = poseAction;
        binding.binding = grip_pose_path[0];
        bindings.push_back(binding);
        binding.action = poseAction;
        binding.binding = grip_pose_path[1];
        bindings.push_back(binding);

        binding.action = triggerAction;
        binding.binding = triggerPaths[0];
        bindings.push_back(binding);
        binding.action = triggerAction;
        binding.binding = triggerPaths[1];
        bindings.push_back(binding);

        binding.action = gripAction;
        binding.binding = gripPaths[0];
        bindings.push_back(binding);
        binding.action = gripAction;
        binding.binding = gripPaths[1];
        bindings.push_back(binding);

        binding.action = menuAction;
        binding.binding = menuPath;
        bindings.push_back(binding);

        binding.action = walkAction;
        binding.binding = walkPath;
        bindings.push_back(binding);

        binding.action = rollAction;
        binding.binding = rollPath;
        bindings.push_back(binding);

        binding.action = jumpAction;
        binding.binding = jumpPath;
        bindings.push_back(binding);


        XrInteractionProfileSuggestedBinding suggested_bindings = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggested_bindings.interactionProfile = interaction_profile_path;
        suggested_bindings.countSuggestedBindings = 10;
        suggested_bindings.suggestedBindings = bindings.data();

        xrSuggestInteractionProfileBindings(xrInstance, &suggested_bindings);
        if (xr != XR_SUCCESS) {
            warn("Failed to suggest bindings, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        // Attach actions

        XrSessionActionSetsAttachInfo actionset_attach_info = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
        actionset_attach_info.countActionSets = 1;
        actionset_attach_info.actionSets = &actionSet;
        xr = xrAttachSessionActionSets(xrSession, &actionset_attach_info);
        if (xr != XR_SUCCESS) {
            warn("Failed to attach action set, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        // Create hand pose spaces
        {
            XrActionSpaceCreateInfo action_space_info = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
            action_space_info.action = poseAction;
            action_space_info.poseInActionSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
            action_space_info.subactionPath = handPaths[0];

            xr = xrCreateActionSpace(xrSession, &action_space_info, &pose_action_spaces[0]);
            if (xr != XR_SUCCESS) {
                warn("Failed to create left hand pose space, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        {
            XrActionSpaceCreateInfo action_space_info = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
            action_space_info.action = poseAction;
            action_space_info.poseInActionSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
            action_space_info.subactionPath = handPaths[1];

            xr = xrCreateActionSpace(xrSession, &action_space_info, &pose_action_spaces[1]);
            if (xr != XR_SUCCESS) {
                warn("Failed to create left hand pose space, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        success("VR Actions Created.");

        return true;
    }

    bool OpenXRService::BeginFrameAndGetVectors(Vec3& Loc, Vec4& Rot, Matrix4 outMatrix)
    {
        // Wait on frame

        xrFrameState = { XR_TYPE_FRAME_STATE };
        XrResult xr = xrWaitFrame(xrSession, nullptr, &xrFrameState);
        if (xr != XR_SUCCESS) {
            warn("Could not wait on OpenXR frame. Error: " + std::to_string(xr));
            return false;
        }

        xr = xrBeginFrame(xrSession, nullptr);
        if (xr != XR_SUCCESS && xr != XR_FRAME_DISCARDED) {
            warn("Could not begin OpenXR frame. Error: " + std::to_string(xr));
            return false;
        }

        XrViewState viewState = { XR_TYPE_VIEW_STATE };
        XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
        locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        locateInfo.displayTime = xrFrameState.predictedDisplayTime;
        locateInfo.space = xrAppSpace;
        xr = xrLocateViews(xrSession, &locateInfo, &viewState, xrViewCount, &xrProjectionViewCount, xrViews.data());
        if (xr != XR_SUCCESS) {
            warn("Could not locate OpenXR views. Error: " + std::to_string(xr));
            return false;
        }
        xrProjectionViews.resize(xrProjectionViewCount);

        return true;

    }

    bool OpenXRService::SubmitFrame(ID3D11Texture2D* texture) {
        int currentEye = !LeftEye; // If the eye is left, Lefteye = 1, but the arrays have the left eye at position 1, index 0.

        xrProjectionViews.at(currentEye) = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
        xrProjectionViews.at(currentEye).pose = xrViews.at(currentEye).pose;
        xrProjectionViews.at(currentEye).fov = xrViews.at(currentEye).fov;
        xrProjectionViews.at(currentEye).subImage.swapchain = xrSwapchains.at(currentEye);
        xrProjectionViews.at(currentEye).subImage.imageRect.offset = {0, 0};
        xrProjectionViews.at(currentEye).subImage.imageRect.extent = {
            static_cast<int32_t>(EyeWidth),
            static_cast<int32_t>(EyeHeight)
        };


        // Wait for the frame to be ready to render to
        uint32_t imageId;
        XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        XrResult xr = xrAcquireSwapchainImage(xrSwapchains.at(currentEye), &acquireInfo, &imageId);
        if (xr != XR_SUCCESS) {
            warn("Could not acquire OpenXR swapchain image. Error: " + std::to_string(xr));
            return false;
        }

        XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        xr = xrWaitSwapchainImage(xrSwapchains.at(currentEye), &waitInfo);
        if (xr != XR_SUCCESS) {
            warn("Could not wait on OpenXR swapchain image. Error: " + std::to_string(xr));
            return false;
        }

        if (!DirectXService::RenderXRFrame(texture, xrRTVs.at(currentEye).at(imageId))) {
            warn("Skipped a frame.");
        }

        // Release the frame
        XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xr = xrReleaseSwapchainImage(xrSwapchains.at(currentEye), &releaseInfo);
        if (xr != XR_SUCCESS) {
            warn("Could not release OpenXR swapchain image. Error: " + std::to_string(xr));
            return false;
        }
        return true;
    }

    bool OpenXRService::UpdateActions() {

        // Sync actions

        XrActiveActionSet active_actionsets = { actionSet, XR_NULL_PATH };
        XrActionsSyncInfo actions_sync_info = { XR_TYPE_ACTIONS_SYNC_INFO };
        actions_sync_info.countActiveActionSets = 1;
        actions_sync_info.activeActionSets = &active_actionsets;

        XrResult xr = xrSyncActions(xrSession, &actions_sync_info);
        if (xr != XR_SUCCESS && xr != XR_SESSION_NOT_FOCUSED) {
            warn("Failed to sync actions. Skipping motion controls this frame. Error: " + std::to_string(xr));
            return false;
        }
        else if (xr == XR_SESSION_NOT_FOCUSED)
        {
            warn("Mod has no focus. Please return to the game.");
        }

        // Get hand poses

        for (int i = 0; i < 2; i++) {
            {
                XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
                XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
                get_info.action = poseAction;
                get_info.subactionPath = handPaths[i];
                xr = xrGetActionStatePose(xrSession, &get_info, &pose_state);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get pose state for a hand. Error: " + std::to_string(xr) + ". Hand: " + std::to_string(i + 1));
                }

                hand_locations[i].type = XR_TYPE_SPACE_LOCATION;

                xr = xrLocateSpace(pose_action_spaces[i], xrAppSpace, xrFrameState.predictedDisplayTime, &hand_locations[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get pose value for a hand. Error: " + std::to_string(xr) + ". Hand: " + std::to_string(i + 1));
                }
            }
            {
                // Get controller trigger states

                grab_value[i].type = XR_TYPE_ACTION_STATE_FLOAT;

                XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
                get_info.action = triggerAction;
                get_info.subactionPath = handPaths[i];
                xr = xrGetActionStateFloat(xrSession, &get_info, &grab_value[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get value for a trigger. Error: " + std::to_string(xr));
                }
            }
            {
                // Get controller trigger states

                grip_value[i].type = XR_TYPE_ACTION_STATE_FLOAT;

                XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
                get_info.action = gripAction;
                get_info.subactionPath = handPaths[i];
                xr = xrGetActionStateFloat(xrSession, &get_info, &grip_value[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get value for a grip. Error: " + std::to_string(xr));
                }
            }
        }

        {
            // Get menu button state

            menu_value.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
            get_info.action = menuAction;
            get_info.subactionPath = handPaths[0];
            xr = xrGetActionStateBoolean(xrSession, &get_info, &menu_value);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for menu button. Error: " + std::to_string(xr));
            }
            if (menu_value.currentState && !pressingMenu)
            {
                info("Toggled menu.");
                pressingMenu = true;
                Menu = !Menu;
                GameService::SetMenu(Menu);
            }
            if (!menu_value.currentState && pressingMenu)
            {
                pressingMenu = false;
            }

        }


        {
            // Get roll button state

            roll_value.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
            get_info.action = rollAction;
            get_info.subactionPath = handPaths[1];
            xr = xrGetActionStateBoolean(xrSession, &get_info, &roll_value);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for roll button. Error: " + std::to_string(xr));
            }

        }

        {
            // Get jump button state

            jump_value.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
            get_info.action = jumpAction;
            get_info.subactionPath = handPaths[1];
            xr = xrGetActionStateBoolean(xrSession, &get_info, &jump_value);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for jump button. Error: " + std::to_string(xr));
            }

        }

        {
            // Get walking state

            menu_value.type = XR_TYPE_ACTION_STATE_VECTOR2F;

            XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
            get_info.action = walkAction;
            get_info.subactionPath = handPaths[0];
            xr = xrGetActionStateVector2f(xrSession, &get_info, &walk_value);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for the left thumbstick. Error: " + std::to_string(xr));
            }

            // Set the xinput state

            InputService::lX = walk_value.currentState.x * 32767;
            InputService::lY = walk_value.currentState.y * 32767;

        }

        InputService::bL = grab_value[0].currentState * 255;
        InputService::bR = grab_value[1].currentState * 255;


        InputService::buttons = 0x0040; // Always sprint
        if (roll_value.currentState)
            InputService::buttons = InputService::buttons | 0x2000;
        if (jump_value.currentState)
            InputService::buttons = InputService::buttons | 0x1000;
        if (grip_value[0].currentState > 0.8)
            InputService::buttons = InputService::buttons | 0x0100;
        if (grip_value[1].currentState > 0.8)
            InputService::buttons = InputService::buttons | 0x0200;

        InputService::Update();

        return true;
    }

    bool OpenXRService::UpdatePoses() {
        int CurrentEye = !LeftEye;

        if (Reconfig) {
            RATIO = (xrViews.at(CurrentEye).fov.angleRight - xrViews.at(CurrentEye).fov.angleLeft) / (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown);
            info("The screen aspect ratio of an HMD eye is " + std::to_string(RATIO));
            SaveConfig();
            info("Saved the new config! You may restart the game now.");
            info("You can try unloading the mod by pressing the END key on your keyboard. This will sometimes crash the game but you might as well try if restarting anyway.");
            Reconfig = false;
        }

        const auto [q1, q2, q3, q0] = xrViews.at(CurrentEye).pose.orientation;
        const auto [lx, ly, lz] = xrViews.at(CurrentEye).pose.position;
        FOV = (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown) * 57.2958f * RATIO;

        Vec3 HMDPosition;

        float scale = 1.f;

        HMDPosition.x = lx * scale;
        HMDPosition.y = ly * scale;
        HMDPosition.z = lz * scale;

        Matrix4 HMDPose;

        HMDPose[0] = 2 * (q0 * q0 + q1 * q1) - 1;
        HMDPose[1] = 2 * (q1 * q2 - q0 * q3);
        HMDPose[2] = 2 * (q1 * q3 + q0 * q2);
        HMDPose[3] = 0;

        HMDPose[4] = 2 * (q1 * q2 + q0 * q3);
        HMDPose[5] = 2 * (q0 * q0 + q2 * q2) - 1;
        HMDPose[6] = 2 * (q2 * q3 - q0 * q1);
        HMDPose[7] = 0;

        HMDPose[8] = 2 * (q1 * q3 - q0 * q2);
        HMDPose[9] = 2 * (q2 * q3 + q0 * q1);
        HMDPose[10] = 2 * (q0 * q0 + q3 * q3) - 1;
        HMDPose[11] = 0;

        HMDPose[12] = 0;
        HMDPose[13] = 0;
        HMDPose[14] = 0;
        HMDPose[15] = 1;

        const auto [hq1, hq2, hq3, hq0] = hand_locations[1].pose.orientation;
        const auto [hlx, hly, hlz] = hand_locations[1].pose.position;
        Vec3 aimLoc;
        aimLoc.x = hlx;
        aimLoc.y = hly;
        aimLoc.z = hlz;

        Vec4 hudQuat;
        Vec4 aimQuat;
        Vec4 lookQuat;

        hudQuat.w = q0;
        hudQuat.x = q1;
        hudQuat.y = q2;
        hudQuat.z = q3;

        if (grab_value[1].currentState > 0.5f)
        {

            if (!Firing)
            {
                Firing = true;
            }
        }

        if (Firing)
        {
            // If the trigger is down, quickly point the gun at its direction

            if (!HEADAIM)
            {
                hudQuat.w = hq0;
                hudQuat.x = hq1;
                hudQuat.y = hq2;
                hudQuat.z = hq3;
            }
        }

        lookQuat.w = q0;
        lookQuat.x = q1;
        lookQuat.y = q2;
        lookQuat.z = q3;

        aimQuat.w = hq0;
        aimQuat.x = hq1;
        aimQuat.y = hq2;
        aimQuat.z = hq3;


        Vec3 hudEuler = EulerFromQuat(hudQuat);
        Vec3 aimEuler = EulerFromQuat(aimQuat);
        Vec3 lookEuler = EulerFromQuat(lookQuat);

        float yaw = -hudEuler.x;
        
        float pitch = hudEuler.z;

        GameService::UpdateCamera(HMDPosition, HMDPose, yaw, pitch, aimLoc, aimQuat);

        float speed = 1.3;
        DirectXService::crosshairX = (-aimEuler.x + lookEuler.x) * speed;
        DirectXService::crosshairY = (aimEuler.z - lookEuler.z) * speed;

        if (!grab_value[1].currentState > 0.5f && Firing)
        {
            // Set the hud back to the head

            hudQuat.w = q0;
            hudQuat.x = q1;
            hudQuat.y = q2;
            hudQuat.z = q3;

            Firing = false;
        }

        return true;
    }

    bool OpenXRService::EndFrame() {
        XrCompositionLayerProjection xrLayerProj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        xrLayerProj.space = xrAppSpace;
        xrLayerProj.viewCount = xrProjectionViewCount;
        xrLayerProj.views = xrProjectionViews.data();
        const auto xrLayer = reinterpret_cast<XrCompositionLayerBaseHeader*>(&xrLayerProj);

        XrFrameEndInfo frameEndInfo = { XR_TYPE_FRAME_END_INFO };
        frameEndInfo.displayTime = xrFrameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = xrBlend;
        frameEndInfo.layerCount = xrLayer == nullptr ? 0 : 1;
        frameEndInfo.layers = &xrLayer;
        XrResult xr = xrEndFrame(xrSession, &frameEndInfo);
        if (xr != XR_SUCCESS) {
            warn("Could not end OpenXR frame. Error: " + std::to_string(xr));
            return false;
        }
        return true;
    }

    void OpenXRService::EndXR() {

        for (uint64_t i = 0; i < xrRTVs.at(0).size(); i++) {
            xrRTVs.at(0).at(i)->Release();
            xrRTVs.at(1).at(i)->Release();
        }
        for (uint32_t i = 0; i < xrViewCount; i++) {
            if (xrSwapchains.at(i) != XR_NULL_HANDLE && xrDestroySwapchain(xrSwapchains.at(i)) != XR_SUCCESS) {
                error("Failed to destroy OpenXR swapchain.");
            }
        }
        if (xrAppSpace != XR_NULL_HANDLE && xrDestroySpace(xrAppSpace) != XR_SUCCESS) {
            error("Failed to destroy OpenXR app space");
        }
        if (xrSession != XR_NULL_HANDLE && xrDestroySession(xrSession) != XR_SUCCESS) {
            error("Failed to destroy OpenXR session");
        }
        if (xrInstance != XR_NULL_HANDLE && xrDestroyInstance(xrInstance) != XR_SUCCESS) {
            error("Failed to destroy OpenXR instance");
        }
    }
}