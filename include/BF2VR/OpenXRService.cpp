#include "OpenXRService.h"
#include "InputService.h"

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

#include "DirectXService.h"
#include "GameService.h"
#include "Utils.h"

namespace BF2VR {
    bool OpenXRService::createXRInstanceWithExtensions() {

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

    bool OpenXRService::beginXRSession(ID3D11Device* pDevice) {

        XrGraphicsBindingD3D11KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
        graphicsBinding.device = pDevice;
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
            swapchainInfo.width = eyeWidth;
            swapchainInfo.height = eyeHeight;
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
        prepareActions();

        isVRReady = true;
        return true;
    }

    bool OpenXRService::prepareActions() {

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
        xrStringToPath(xrInstance, "/user/hand/left/input/menu/click", &menuPath);
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

        // X click action
        xrStringToPath(xrInstance, "/user/hand/left/input/x/click", &reloadPath);
        {
            XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            actionInfo.countSubactionPaths = 1;
            actionInfo.subactionPaths = &handPaths[0];
            strcpy(actionInfo.actionName, "reload");
            strcpy(actionInfo.localizedActionName, "Reload");
            xr = xrCreateAction(actionSet, &actionInfo, &reloadAction);
            if (xr != XR_SUCCESS) {
                warn("Failed to create pose action for menu, reloading will not work. Error: " + std::to_string(xr));
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

        XrPath gripPosePath[2];
        xrStringToPath(xrInstance, "/user/hand/left/input/grip/pose", &gripPosePath[0]);
        xrStringToPath(xrInstance, "/user/hand/right/input/grip/pose", &gripPosePath[1]);


        XrPath interactionProfilePath;
        xr = xrStringToPath(xrInstance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePath);
        if (xr != XR_SUCCESS) {
            warn("Failed to get interaction profile, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        std::vector<XrActionSuggestedBinding> bindings;
        XrActionSuggestedBinding binding;
        binding.action = poseAction;
        binding.binding = gripPosePath[0];
        bindings.push_back(binding);
        binding.action = poseAction;
        binding.binding = gripPosePath[1];
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

        binding.action = reloadAction;
        binding.binding = reloadPath;
        bindings.push_back(binding);



        XrInteractionProfileSuggestedBinding suggestedBindings = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        suggestedBindings.interactionProfile = interactionProfilePath;
        suggestedBindings.countSuggestedBindings = 11;
        suggestedBindings.suggestedBindings = bindings.data();

        xrSuggestInteractionProfileBindings(xrInstance, &suggestedBindings);
        if (xr != XR_SUCCESS) {
            warn("Failed to suggest bindings, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        // Attach actions

        XrSessionActionSetsAttachInfo actionsetAttachInfo = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
        actionsetAttachInfo.countActionSets = 1;
        actionsetAttachInfo.actionSets = &actionSet;
        xr = xrAttachSessionActionSets(xrSession, &actionsetAttachInfo);
        if (xr != XR_SUCCESS) {
            warn("Failed to attach action set, motion controlls will not work. Error: " + std::to_string(xr));
            return false;
        }

        // Create hand pose spaces
        {
            XrActionSpaceCreateInfo actionSpaceInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
            actionSpaceInfo.action = poseAction;
            actionSpaceInfo.poseInActionSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
            actionSpaceInfo.subactionPath = handPaths[0];

            xr = xrCreateActionSpace(xrSession, &actionSpaceInfo, &poseActionSpaces[0]);
            if (xr != XR_SUCCESS) {
                warn("Failed to create left hand pose space, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        {
            XrActionSpaceCreateInfo actionSpaceInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
            actionSpaceInfo.action = poseAction;
            actionSpaceInfo.poseInActionSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
            actionSpaceInfo.subactionPath = handPaths[1];

            xr = xrCreateActionSpace(xrSession, &actionSpaceInfo, &poseActionSpaces[1]);
            if (xr != XR_SUCCESS) {
                warn("Failed to create left hand pose space, motion controlls will not work. Error: " + std::to_string(xr));
                return false;
            }
        }

        success("VR Actions Created.");

        return true;
    }

    bool OpenXRService::beginFrameAndGetVectors(Vec3& Loc, Vec4& Rot, Matrix4 outMatrix)
    {
        // Check if we need to stop
        if (shouldStop)
        {
            XrResult xr = xrRequestExitSession(xrSession);

            XrEventDataBuffer runtimeEvent = { XR_TYPE_EVENT_DATA_BUFFER };
            xr = xrPollEvent(xrInstance, &runtimeEvent);

            while (xr == XR_SUCCESS) {

                if (runtimeEvent.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
                {
                    XrEventDataSessionStateChanged* event = (XrEventDataSessionStateChanged*)&runtimeEvent;
                    if (event->state >= XR_SESSION_STATE_STOPPING) {
                        stopping = true;
                        return true;
                    }
                }
                if (runtimeEvent.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
                {
                    stopping = true;
                    return true;
                }

                runtimeEvent.type = XR_TYPE_EVENT_DATA_BUFFER;
                xr = xrPollEvent(xrInstance, &runtimeEvent);
            }
        }

        if (stopping)
        {
            return true;
        }

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

    bool OpenXRService::submitFrame(ID3D11Texture2D* texture) {

        if (stopping)
        {
            return true;
        }

        int currentEye = !onLeftEye; // If the eye is left, Lefteye = 1, but the arrays have the left eye at position 1, index 0.

        xrProjectionViews.at(currentEye) = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
        xrProjectionViews.at(currentEye).pose = xrViews.at(currentEye).pose;
        xrProjectionViews.at(currentEye).fov = xrViews.at(currentEye).fov;
        xrProjectionViews.at(currentEye).subImage.swapchain = xrSwapchains.at(currentEye);
        xrProjectionViews.at(currentEye).subImage.imageRect.offset = {0, 0};
        xrProjectionViews.at(currentEye).subImage.imageRect.extent = {
            static_cast<int32_t>(eyeWidth),
            static_cast<int32_t>(eyeHeight)
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

        if (!DirectXService::renderXRFrame(texture, xrRTVs.at(currentEye).at(imageId))) {
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

    bool OpenXRService::updateActions() {
        if (stopping)
        {
            return true;
        }

        // Sync actions

        XrActiveActionSet activeActionsets = { actionSet, XR_NULL_PATH };
        XrActionsSyncInfo actionsSyncInfo = { XR_TYPE_ACTIONS_SYNC_INFO };
        actionsSyncInfo.countActiveActionSets = 1;
        actionsSyncInfo.activeActionSets = &activeActionsets;

        XrResult xr = xrSyncActions(xrSession, &actionsSyncInfo);
        if (xr != XR_SUCCESS && xr != XR_SESSION_NOT_FOCUSED) {
            warn("Failed to sync actions. Skipping motion controls this frame. Error: " + std::to_string(xr));
            return false;
        }
        /*
        else if (xr == XR_SESSION_NOT_FOCUSED)
        {
            warn("Mod has no focus. Please return to the game.");
        }
        */

        // Get hand poses

        for (int i = 0; i < 2; i++) {
            {
                XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
                XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = poseAction;
                getInfo.subactionPath = handPaths[i];
                xr = xrGetActionStatePose(xrSession, &getInfo, &pose_state);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get pose state for a hand. Error: " + std::to_string(xr) + ". Hand: " + std::to_string(i + 1));
                }

                handLocations[i].type = XR_TYPE_SPACE_LOCATION;

                xr = xrLocateSpace(poseActionSpaces[i], xrAppSpace, xrFrameState.predictedDisplayTime, &handLocations[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get pose value for a hand. Error: " + std::to_string(xr) + ". Hand: " + std::to_string(i + 1));
                }
            }
            {
                // Get controller trigger states

                grabValue[i].type = XR_TYPE_ACTION_STATE_FLOAT;

                XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = triggerAction;
                getInfo.subactionPath = handPaths[i];
                xr = xrGetActionStateFloat(xrSession, &getInfo, &grabValue[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get value for a trigger. Error: " + std::to_string(xr));
                }
            }
            {
                // Get controller trigger states

                gripValue[i].type = XR_TYPE_ACTION_STATE_FLOAT;

                XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
                getInfo.action = gripAction;
                getInfo.subactionPath = handPaths[i];
                xr = xrGetActionStateFloat(xrSession, &getInfo, &gripValue[i]);
                if (xr != XR_SUCCESS) {
                    warn("Failed to get value for a grip. Error: " + std::to_string(xr));
                }
            }
        }

        {
            // Get menu button state

            menuValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = menuAction;
            getInfo.subactionPath = handPaths[0];
            xr = xrGetActionStateBoolean(xrSession, &getInfo, &menuValue);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for menu button. Error: " + std::to_string(xr));
            }
            if (menuValue.currentState && !isPressingMenu)
            {
                info("Toggled menu.");
                isPressingMenu = true;
                showUI = !showUI;
                GameService::setUIDrawState(showUI);
            }
            if (!menuValue.currentState && isPressingMenu)
            {
                isPressingMenu = false;
            }

        }


        {
            // Get roll button state

            rollValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = rollAction;
            getInfo.subactionPath = handPaths[1];
            xr = xrGetActionStateBoolean(xrSession, &getInfo, &rollValue);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for roll button. Error: " + std::to_string(xr));
            }

        }

        {
            // Get jump button state

            jumpValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = jumpAction;
            getInfo.subactionPath = handPaths[1];
            xr = xrGetActionStateBoolean(xrSession, &getInfo, &jumpValue);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for jump button. Error: " + std::to_string(xr));
            }

        }

        {
            // Get reload button state

            reloadValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;

            XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = reloadAction;
            getInfo.subactionPath = handPaths[0];
            xr = xrGetActionStateBoolean(xrSession, &getInfo, &reloadValue);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for reload button. Error: " + std::to_string(xr));
            }

        }

        {
            // Get walking state

            walkValue.type = XR_TYPE_ACTION_STATE_VECTOR2F;

            XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
            getInfo.action = walkAction;
            getInfo.subactionPath = handPaths[0];
            xr = xrGetActionStateVector2f(xrSession, &getInfo, &walkValue);
            if (xr != XR_SUCCESS) {
                warn("Failed to get value for the left thumbstick. Error: " + std::to_string(xr));
            }

            // Set the xinput state

            InputService::thumbLX = (short)(walkValue.currentState.x * 32767);
            InputService::thumbLY = (short)(walkValue.currentState.y * 32767);

        }

        InputService::leftTrigger = (byte)(grabValue[0].currentState * 255);
        InputService::rightTrigger = (byte)(grabValue[1].currentState * 255);


        InputService::buttons = 0x0040; // Always sprint
        if (rollValue.currentState)
            InputService::buttons = InputService::buttons | 0x2000;
        if (jumpValue.currentState)
            InputService::buttons = InputService::buttons | 0x1000;
        if (reloadValue.currentState)
            InputService::buttons = InputService::buttons | 0x4000;
        if (gripValue[0].currentState > 0.8)
            InputService::buttons = InputService::buttons | 0x0100;
        if (gripValue[1].currentState > 0.8)
            InputService::buttons = InputService::buttons | 0x0200;

        InputService::update();

        return true;
    }

    bool OpenXRService::updatePoses() {
        int CurrentEye = !onLeftEye;

        if (doReconfig) {
            RATIO = (xrViews.at(CurrentEye).fov.angleRight - xrViews.at(CurrentEye).fov.angleLeft) / (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown);
            FOV = (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown) * 57.2958f;
            info("The screen aspect ratio of an HMD eye is " + std::to_string(RATIO));
            saveConfig();
            info("Saved the new config! You may restart the game now.");
            info("You can try unloading the mod by pressing the END key on your keyboard. This will sometimes crash the game but you might as well try if restarting anyway.");
            doReconfig = false;
        }

        // Get the headset transform

        const auto [q1, q2, q3, q0] = xrViews.at(CurrentEye).pose.orientation;
        const auto [lx, ly, lz] = xrViews.at(CurrentEye).pose.position;

        // Transform scale
        float size = 1.f;

        Vec3 HMDLoc;
        HMDLoc.x = lx;
        HMDLoc.y = ly;
        HMDLoc.z = lz;
        HMDLoc = HMDLoc * Vec3(size, size, size);

        Vec4 HMDQuat;
        HMDQuat.w = q0;
        HMDQuat.x = q1;
        HMDQuat.y = q2;
        HMDQuat.z = q3;
        Vec3 HMDEuler = eulerFromQuat(HMDQuat);

        Matrix4 HMDMat;

        HMDMat.x.x = 2 * (q0 * q0 + q1 * q1) - 1;
        HMDMat.y.x = 2 * (q1 * q2 - q0 * q3);
        HMDMat.z.x = 2 * (q1 * q3 + q0 * q2);
        HMDMat.o.x = 0;

        HMDMat.x.y = 2 * (q1 * q2 + q0 * q3);
        HMDMat.y.y = 2 * (q0 * q0 + q2 * q2) - 1;
        HMDMat.z.y = 2 * (q2 * q3 - q0 * q1);
        HMDMat.o.y = 0;

        HMDMat.x.z = 2 * (q1 * q3 - q0 * q2);
        HMDMat.y.z = 2 * (q2 * q3 + q0 * q1);
        HMDMat.z.z = 2 * (q0 * q0 + q3 * q3) - 1;
        HMDMat.o.z = 0;

        HMDMat.x.w = 0;
        HMDMat.y.w = 0;
        HMDMat.x.w = 0;
        HMDMat.o.w = 1;

        // Get the right hand transform

        const auto [hq1, hq2, hq3, hq0] = handLocations[1].pose.orientation;
        const auto [hlx, hly, hlz] = handLocations[1].pose.position;


        Vec4 hudQuat;
        hudQuat.w = q0;
        hudQuat.x = q1;
        hudQuat.y = q2;
        hudQuat.z = q3;

        if (grabValue[1].currentState > 0.5f)
        {

            if (!isFiring)
                isFiring = true;
        }

        if (isFiring)
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
        Vec3 hudEuler = eulerFromQuat(hudQuat);


        Vec3 aimLoc;
        aimLoc.x = hlx;
        aimLoc.y = hly;
        aimLoc.z = hlz;
        aimLoc = aimLoc * Vec3(size, size, size);

        Vec4 aimQuat;
        aimQuat.w = hq0;
        aimQuat.x = hq1;
        aimQuat.y = hq2;
        aimQuat.z = hq3;
        Vec3 aimEuler = eulerFromQuat(aimQuat);

        float yaw = -hudEuler.y;
        float pitch = hudEuler.z;


        // Relative aim location in frostbite units
        Vec3 fbAimLoc = aimLoc - HMDLoc;

        Vec3 relAimLoc = rotateAround(aimLoc, HMDLoc, HMDEuler.y);

        fbAimLoc.x = relAimLoc.y - 0.5f; // Up down
        fbAimLoc.y = relAimLoc.x;
        fbAimLoc.z = -relAimLoc.z;

        float handSpeed = 0.5f;
        fbAimLoc = fbAimLoc * Vec3(handSpeed, handSpeed, handSpeed);


        Vec4 fbAimQuat;
        fbAimQuat.x = aimQuat.y;
        fbAimQuat.y = aimQuat.x;
        fbAimQuat.z = -aimQuat.z;
        fbAimQuat.w = aimQuat.w;

        float angle = 90.f;
        Vec4 correctiveRotation = Vec4(0, 0, sin(angle), cos(angle));
        fbAimQuat = fbAimQuat.rotateByEuler(0, 0, -90);

        
        GameService::updateCamera(HMDLoc, HMDMat, yaw, pitch - 0.37);
        GameService::updateBone("Wep_Root", fbAimLoc, fbAimQuat);

        DirectXService::crosshairX = (-aimEuler.y + HMDEuler.y) * 1.3f;
        DirectXService::crosshairY = (aimEuler.z - HMDEuler.z) * 1.3f - 0.5f;

        if (!(grabValue[1].currentState > 0.5f) && isFiring)
        {
            // Set the hud back to the head

            hudQuat.w = q0;
            hudQuat.x = q1;
            hudQuat.y = q2;
            hudQuat.z = q3;

            isFiring = false;
        }

        return true;
    }

    bool OpenXRService::endFrame() {

        if (stopping)
        {
            return true;
        }


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

    void OpenXRService::endXR() {

        XrResult xr = xrEndSession(xrSession);
        if (xr != XR_SUCCESS) {
            error("Failed to end OpenXR session. Error:" + std::to_string(xr));
        }

        for (uint64_t i = 0; i < xrRTVs.at(0).size(); i++) {
            xrRTVs.at(0).at(i)->Release();
            xrRTVs.at(1).at(i)->Release();
        }
        for (uint32_t i = 0; i < xrViewCount; i++) {
            xr = xrDestroySwapchain(xrSwapchains.at(i));
            if (xr != XR_SUCCESS) {
                error("Failed to destroy OpenXR swapchain. Error:" + std::to_string(xr));
            }
        }
        xr = xrDestroySpace(xrAppSpace);
        if (xr != XR_SUCCESS) {
            error("Failed to destroy OpenXR app space. Error:" + std::to_string(xr));
        }

        xr = xrDestroySession(xrSession);
        if (xr != XR_SUCCESS) {
            error("Failed to destroy OpenXR session. Error:" + std::to_string(xr));
        }

        xr = xrDestroyInstance(xrInstance);
        if (xr != XR_SUCCESS) {
            error("Failed to destroy OpenXR instance. Error:" + std::to_string(xr));
        }

    }
}