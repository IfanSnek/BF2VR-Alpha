#include "OpenXRService.h"

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

#include "DirectXService.h"
#include "GameService.h"
#include "Utils.h"

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
            log("Could not query OpenXR extension count");
            return false;
        }

        std::vector<XrExtensionProperties> allExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
        xr = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, allExtensions.data());

        if (xr != XR_SUCCESS) {
            log("Could not query OpenXR extensions");
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
            log("the active OpenXR runtime does not support D3D11");
            return false;
        }

        XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        createInfo.enabledExtensionNames = enabledExtensions.data();
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strcpy_s(createInfo.applicationInfo.applicationName, "Star Wars Battlefront II VR");
        xr = xrCreateInstance(&createInfo, &xrInstance);
        if (xr != XR_SUCCESS) {
            log("Failed to create OpenXR xrInstance");
            log(std::to_string(xr));
            return false;

        }

        // Load D3D11 extension

        PFN_xrGetD3D11GraphicsRequirementsKHR xrExtGetD3D11GraphicsRequirements = nullptr;
        xr = xrGetInstanceProcAddr(xrInstance, "xrGetD3D11GraphicsRequirementsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&xrExtGetD3D11GraphicsRequirements));
        if (xr != XR_SUCCESS) {
            log("Could not get address for xrGetD3D11GraphicsRequirementsKHR");
            return false;

        }

        XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        xr = xrGetSystem(xrInstance, &systemInfo, &xrSystemId);
        if (xr != XR_SUCCESS) {
            log("Could not get XR system");
            return false;

        }

        uint32_t blendCount = 0;
        xr = xrEnumerateEnvironmentBlendModes(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 1, &blendCount, &xrBlend);
        if (xr != XR_SUCCESS) {
            log("Could not get XR blend modes");
            return false;

        }

        XrGraphicsRequirementsD3D11KHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
        xr = xrExtGetD3D11GraphicsRequirements(xrInstance, xrSystemId, &graphicsRequirements);

        if (xr != XR_SUCCESS) {
            log("Could not get XR graphic requirements");
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
            log("Failed to create OpenXR xrSession " + std::to_string(xr));
            return false;
        }

        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.poseInReferenceSpace = { { 0, 0, 0, 1 }, { 0, 0, 0 } };
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        xr = xrCreateReferenceSpace(xrSession, &spaceInfo, &xrAppSpace);
        if (xr != XR_SUCCESS) {
            log("Could not create OpenXR reference space");
            return false;
        }

        // Set eye resolution

        xr = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &xrViewCount, nullptr);
        if (xr != XR_SUCCESS) {
            log("Could not enumerate OpenXR view configuration view count");
            return false;
        }

        xrConfigViews.resize(xrViewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xrViews.resize(xrViewCount, { XR_TYPE_VIEW });

        xr = xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, xrViewCount, &xrViewCount, xrConfigViews.data());
        if (xr != XR_SUCCESS) {
            log("Could not enumerate OpenXR view configuration views");
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
                log("Could not create XR swapchain.");
                return false;
            }

            uint32_t surfaceCount;
            xr = xrEnumerateSwapchainImages(swapchain, 0, &surfaceCount, nullptr);
            if (xr != XR_SUCCESS) {
                log("Could not enumerate OpenXR swapchain image count: " + std::to_string(xr));
                return false;
            }

            std::vector<XrSwapchainImageD3D11KHR> surfaces;
            surfaces.resize(surfaceCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
            xr = xrEnumerateSwapchainImages(swapchain, surfaceCount, &surfaceCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(surfaces.data()));
            if (xr != XR_SUCCESS) {
                log("Could not enumerate OpenXR swapchain images");
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
                    log("Failed to create an RTV for OpenXR");
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
            log("Could not begin XR xrSession.");
            return false;
        }

        VRReady = true;
        return true;
    }

    bool OpenXRService::BeginFrameAndGetVectors(Vec3& Loc, Vec4& Rot, Matrix4 outMatrix)
    {
        // Wait on frame

        xrFrameState = { XR_TYPE_FRAME_STATE };
        XrResult xr = xrWaitFrame(xrSession, nullptr, &xrFrameState);
        if (xr != XR_SUCCESS) {
            log("Could not wait on OpenXR frame");
            return false;
        }

        xr = xrBeginFrame(xrSession, nullptr);
        if (xr != XR_SUCCESS && xr != XR_FRAME_DISCARDED) {
            log("Could not begin OpenXR frame " + std::to_string(xr));
            return false;
        }

        XrViewState viewState = { XR_TYPE_VIEW_STATE };
        XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
        locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        locateInfo.displayTime = xrFrameState.predictedDisplayTime + 2 * xrFrameState.predictedDisplayPeriod;
        locateInfo.space = xrAppSpace;
        xr = xrLocateViews(xrSession, &locateInfo, &viewState, xrViewCount, &xrProjectionViewCount, xrViews.data());
        if (xr != XR_SUCCESS) {
            log("Could not locate OpenXR views");
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
            log("Could not acquire OpenXR swapchain image" + std::to_string(xr));
            return false;
        }

        XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        waitInfo.timeout = XR_INFINITE_DURATION;
        xr = xrWaitSwapchainImage(xrSwapchains.at(currentEye), &waitInfo);
        if (xr != XR_SUCCESS) {
            log("Could not wait on OpenXR swapchain image" + std::to_string(xr));
            return false;
        }

        if (!DirectXService::RenderXRFrame(texture, xrRTVs.at(currentEye).at(imageId))) {
            log("Failed to render a frame");
            
        }

        // Release the frame
        XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xr = xrReleaseSwapchainImage(xrSwapchains.at(currentEye), &releaseInfo);
        if (xr != XR_SUCCESS) {
            log("Could not release OpenXR swapchain image" + std::to_string(xr));
            return false;
        }
        return true;
    }

    bool OpenXRService::UpdatePoses() {

        int CurrentEye = !LeftEye;

        if (Reconfig) {
            RATIO = (xrViews.at(CurrentEye).fov.angleRight - xrViews.at(CurrentEye).fov.angleLeft) / (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown);
            log("The screen aspect ratio of an HMD eye is " + std::to_string(RATIO));
            SaveConfig();
            log("Saved the new config! You may restart the game now.");
            log("You can try unloading the mod by pressing the END key on your keyboard. This will sometimes crash the game but you might as well try if restarting anyway.");
            Reconfig = false;
        }

        const auto [q1, q2, q3, q0] = xrViews.at(CurrentEye).pose.orientation;
        const auto [lx, ly, lz] = xrViews.at(CurrentEye).pose.position;
        FOV = (xrViews.at(CurrentEye).fov.angleUp - xrViews.at(CurrentEye).fov.angleDown) * 57.2958 * RATIO;

        Vec3 HMDPosition;

        float scale = 1.1f;

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

        Vec4 quat;
        quat.w = q0;
        quat.x = q1;
        quat.y = q2;
        quat.z = q3;

        Vec3 euler = EulerFromQuat(quat);

        float yaw = -euler.x;
        float pitch = euler.z;

        GameService::UpdateCamera(HMDPosition, HMDPose, yaw, pitch);

        // TODO: Track controllers

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
            log("Could not end OpenXR frame" + std::to_string(xr));
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
                log("Failed to destroy OpenXR swapchain");
            }
        }
        if (xrAppSpace != nullptr && xrAppSpace != XR_NULL_HANDLE && xrDestroySpace(xrAppSpace) != XR_SUCCESS) {
            log("Failed to destroy OpenXR app space");
        }
        if (xrSession != nullptr && xrSession != XR_NULL_HANDLE && xrDestroySession(xrSession) != XR_SUCCESS) {
            log("Failed to destroy OpenXR session");
        }
        if (xrInstance != nullptr && xrInstance != XR_NULL_HANDLE && xrDestroyInstance(xrInstance) != XR_SUCCESS) {
            log("Failed to destroy OpenXR instance");
        }
    }
}