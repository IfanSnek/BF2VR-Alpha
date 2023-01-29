#include "DirectXService.h"
#include "PixelShader.h"
#include "VSR.h"
#include "VSL.h"
#include "OpenXRService.h"
#include "Utils.h"

#include <DirectXColors.h>
#include "../../third-party/DirectXTK/SimpleMath.h"

namespace BF2VR {
    class OpenXRService;

    HRESULT DirectXService::PresentDetour(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags)
    {
        HRESULT result = S_OK;

        if (DoPresent)
        {
            if (!OpenXRService::VRReady && pDevice == nullptr)
            {
                ID3D11Device* device;
                ID3D11DeviceContext* context;
                pInstance->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device));
                device->GetImmediateContext(&context);

                pDevice = device;
                pContext = context;

                log("Attempting to finalize OpenXR session ...");
                if (!OpenXRService::BeginXRSession(pDevice)) {
                    log("Unable to begin session");
                    Shutdown();
                } else {
                    log("Success");
                }
            }

            if (OpenXRService::VRReady) {

                if (OpenXRService::LeftEye) {

                    Vec3 hmd_location;
                    Vec4 hmd_quatrotation;
                    Matrix4 hmd_transformationmatrix{};

                    if (!OpenXRService::BeginFrameAndGetVectors(hmd_location, hmd_quatrotation, hmd_transformationmatrix)) {
                        log("Did not begin a frame");
                    }

                    OpenXRService::UpdateActions();
                }

                // Get the color buffer from the screen
                HRESULT hr = pInstance->GetBuffer(0, IID_PPV_ARGS(&CurrentFrame));
                if (SUCCEEDED(hr))
                {
                    OpenXRService::SubmitFrame(CurrentFrame);
                }

                if (!OpenXRService::LeftEye) {

                    OpenXRService::EndFrame();
                }

                // Switch eyes
                OpenXRService::LeftEye = !OpenXRService::LeftEye;

                
            }
        }
        if (OpenXRService::LeftEye) {
            return PresentOriginal(pInstance, SyncInterval, Flags);
        }
        else {
            return S_OK;
        }
    }

    bool DirectXService::HookDirectX(HWND window) {

        ID3D11Device* pDummyDevice;
        IDXGISwapChain* pDummySwapChain;
        ID3D11DeviceContext* pDummyContext;

        // Some nice little settings for our dummy device
        auto featureLevel = D3D_FEATURE_LEVEL_11_0;

        DXGI_SWAP_CHAIN_DESC desc = { };

        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 1;

        // Finish setting up  our dummy
        desc.OutputWindow = window;
        desc.Windowed = true;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        // Create the dummy
        if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel, 1, D3D11_SDK_VERSION, &desc, &pDummySwapChain, &pDummyDevice, nullptr, &pDummyContext)))
        {
            log("Could not create dummy DirectX device");
            Shutdown();
            return false;
        }

        // Get our Present pointer
        auto pTable = *reinterpret_cast<PVOID**>(pDummySwapChain);

        // Hook our dummy DirectX stuff into the real one
        PresentTarget = reinterpret_cast<Present*>(pTable[8]);
        MH_STATUS mh = MH_CreateHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(PresentTarget)), reinterpret_cast<LPVOID>(&PresentDetour), reinterpret_cast<LPVOID*>(&PresentOriginal));
        if (mh != MH_OK) {
            log("Error hooking DirectX present: " + std::to_string(mh));
            return false;
        }

        mh = MH_EnableHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(PresentTarget)));
        if (mh != MH_OK) {
            log("Error enabling DirectX present hook: " + std::to_string(mh));
            return false;
        }

        pDummySwapChain->Release();
        pDummyDevice->Release();
        pDummyContext->Release();

        return true;
    }

    void DirectXService::UnhookDirectX() {

        MH_DisableHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(PresentTarget)));
        MH_RemoveHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(PresentTarget)));

        if (srvCreated)
        {
            srv->Release();
        }
        if (shadersCreated)
        {
            VertexShaderRight->Release();
            VertexShaderLeft->Release();
            PixelShader->Release();
        }
        if (copy != nullptr)
        {
           copy->Release();
        }

    }

    void DirectXService::RenderOverlays(ID3D11Device* device, ID3D11DeviceContext* context) {


        // This uses some parts of DirectXTK
        // PrimitiveBatch

        m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

        // BasicEffect
        m_effect.reset(new DirectX::BasicEffect(device));
        m_effect->SetVertexColorEnabled(true);

        void const* shaderByteCode;
        size_t byteCodeLength;

        m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        // Set InputLayout
        device->CreateInputLayout(
            DirectX::VertexPositionColor::InputElements,
            DirectX::VertexPositionColor::InputElementCount,
            shaderByteCode,
            byteCodeLength,
            &InputLayout);
        

        m_effect->SetView(DirectX::SimpleMath::Matrix::CreateLookAt(DirectX::SimpleMath::Vector3(0.f, 0.f, 1),
            DirectX::SimpleMath::Vector3::Zero, DirectX::SimpleMath::Vector3::UnitY));


        m_effect->Apply(context);
        context->IASetInputLayout(InputLayout);

        // Actually draw the line

        m_batch->Begin();

        float lineLength = 0.1f;
        float shift = (OpenXRService::LeftEye ? 0.f : -0.3f);
        shift += crosshairX;

        DirectX::VertexPositionColor v1(DirectX::SimpleMath::Vector3(shift - lineLength, crosshairY + 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v2(DirectX::SimpleMath::Vector3(shift + lineLength, crosshairY + 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v3(DirectX::SimpleMath::Vector3(shift - lineLength, crosshairY - 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v4(DirectX::SimpleMath::Vector3(shift + lineLength, crosshairY - 0.003f, 0), DirectX::Colors::Green);

        m_batch->DrawQuad(v1, v2, v4, v3);

        DirectX::VertexPositionColor v5(DirectX::SimpleMath::Vector3(shift + 0.003f, -lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v6(DirectX::SimpleMath::Vector3(shift + 0.003f, lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v7(DirectX::SimpleMath::Vector3(shift - 0.003f, -lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v8(DirectX::SimpleMath::Vector3(shift - 0.003f, lineLength + crosshairY, 0), DirectX::Colors::Green);

        m_batch->DrawQuad(v5,v6,v8,v7);

        m_batch->End();
    }

    bool DirectXService::RenderXRFrame(ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv)
    {

        ID3D11Device* device;
        rtv->GetDevice(&device);
        ID3D11DeviceContext* context;
        device->GetImmediateContext(&context);

        // Create shaders if needed
        if (!shadersCreated)
        {
            HRESULT hr = device->CreatePixelShader(
                gPS,
                ARRAYSIZE(gPS),
                nullptr,
                &PixelShader
            );

            if (FAILED(hr)) {
                log("Could not create pixel shader " + std::to_string(hr));
                return false;
            }

            hr = device->CreateVertexShader(
                gVSL,
                ARRAYSIZE(gVSL),
                nullptr,
                &VertexShaderLeft
            );

            if (FAILED(hr)) {
                log("Could not create vertex shader for left eye " + std::to_string(hr));
                return false;
            }

            hr = device->CreateVertexShader(
                gVSR,
                ARRAYSIZE(gVSR),
                nullptr,
                &VertexShaderRight
            );

            if (FAILED(hr)) {
                log("Could not create vertex shader for right eye " + std::to_string(hr));
                return false;
            }

            log("Shaders Created");
            shadersCreated = true;

        }

        // Invalidate the old srv if it changes

        srvCreated = true;
        if (srv == nullptr) {
            srvCreated = false;
        }
        else {
            D3D11_TEXTURE2D_DESC sourceDesc;
            CurrentFrame->GetDesc(&sourceDesc);
            D3D11_TEXTURE2D_DESC copyDesc;
            copy->GetDesc(&copyDesc);
            if (copyDesc.Format != sourceDesc.Format
                || copyDesc.Width != sourceDesc.Width
                || copyDesc.Height != sourceDesc.Height) {
                srvCreated = false;
                srv->Release();
                copy->Release();
            }
        }

        // Create new resource view if needed
        if (!srvCreated)
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            CurrentFrame->GetDesc(&textureDesc);
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &copy);
            if (FAILED(hr))
            {
                log("Failed to copy texture");
                return false;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC  srvDesc;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            hr = device->CreateShaderResourceView(copy, &srvDesc, &srv);
            if (FAILED(hr))
            {
                log("Failed to create shader resource view");
                return false;
            }
            log("SRV Created");
            srvCreated = true;
        }

        // Finally, draw.

        context->CopyResource(copy, CurrentFrame);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, static_cast<DXGI_FORMAT>(0), 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader((OpenXRService::LeftEye ? VertexShaderLeft : VertexShaderRight), nullptr, 0);
        context->PSSetShader(PixelShader, nullptr, 0);
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->PSSetShaderResources(0, 1, &srv);
        context->Draw(3, 0);

        RenderOverlays(device, context);

        context->Release();
        device->Release();
    }
}