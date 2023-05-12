// DirectXService.cpp - Code that interacts with the DirectX API, including hooks and rendering.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include "DirectXService.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "OpenXRService.h"
#include "Utils.h"

#include <DirectXColors.h>
#include "../../third-party/DirectXTK/SimpleMath.h"

namespace BF2VR {

    HRESULT DirectXService::presentDetour(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags)
    {
        HRESULT result = S_OK;

        if (shouldPresent)
        {
            if (!OpenXRService::isVRReady && pDevice == nullptr)
            {
                ID3D11Device* pDummyDevice;
                ID3D11DeviceContext* pDummyContext;
                pInstance->GetDevice(__uuidof(pDummyDevice), reinterpret_cast<PVOID*>(&pDummyDevice));
                pDummyDevice->GetImmediateContext(&pDummyContext);

                pDevice = pDummyDevice;
                pContext = pDummyContext;

                log("Attempting to finalize OpenXR session ...");
                if (!OpenXRService::beginXRSession(pDevice)) {
                    error("Unable to begin session.");
                    shutdown();
                } else {
                    success("Finalized OpenXR.");
                }
            }

            if (OpenXRService::isVRReady) {

                if (OpenXRService::onLeftEye) {

                    Vec3 hmd_location;
                    Vec4 hmd_quatrotation;
                    Matrix4 hmd_transformationmatrix{};

                    if (!OpenXRService::beginFrameAndGetVectors(hmd_location, hmd_quatrotation, hmd_transformationmatrix)) {
                        warn("Did not begin a frame");
                    }

                    OpenXRService::updateActions();
                }

                OpenXRService::updatePoses();

                // Get the color buffer from the screen
                HRESULT hr = pInstance->GetBuffer(0, IID_PPV_ARGS(&pFrame));
                if (SUCCEEDED(hr))
                {
                    OpenXRService::submitFrame(pFrame);
                }

                if (!OpenXRService::onLeftEye) {

                    OpenXRService::endFrame();
                }

                // Switch eyes
                OpenXRService::onLeftEye = !OpenXRService::onLeftEye;

                
            }
        }

        if (OpenXRService::onLeftEye) {
            return pPresentOriginal(pInstance, SyncInterval, Flags);
        }
        else {
            return S_OK;
        }
    }

    bool DirectXService::hookDirectX(HWND window) {

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
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        // Create the dummy
        if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel, 1, D3D11_SDK_VERSION, &desc, &pDummySwapChain, &pDummyDevice, nullptr, &pDummyContext)))
        {
            error("Could not create dummy DirectX device.");
            shutdown();
            return false;
        }

        // Get our Present pointer
        auto pTable = *reinterpret_cast<PVOID**>(pDummySwapChain);

        // Hook our dummy DirectX stuff into the real one
        pPresentTarget = reinterpret_cast<Present*>(pTable[8]);
        MH_STATUS mh = MH_CreateHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(pPresentTarget)), reinterpret_cast<LPVOID>(&presentDetour), reinterpret_cast<LPVOID*>(&pPresentOriginal));
        if (mh != MH_OK) {
            error("Error hooking DirectX present. Error: " + std::to_string(mh));
            return false;
        }

        mh = MH_EnableHook(reinterpret_cast<LPVOID>(reinterpret_cast<DWORD64>(pPresentTarget)));
        if (mh != MH_OK) {
            error("Error enabling DirectX present hook. Error: " + std::to_string(mh));
            return false;
        }

        pDummySwapChain->Release();
        pDummyDevice->Release();
        pDummyContext->Release();

        return true;
    }

    void DirectXService::unhookDirectX() {

        MH_DisableHook(reinterpret_cast<LPVOID>(pPresentTarget));
        MH_RemoveHook(reinterpret_cast<LPVOID>(pPresentTarget));

        if (srvCreated)
        {
            pSRV->Release();
        }
        if (shadersCreated)
        {
            pVertexShader->Release();
            pPixelShader->Release();
        }
        if (pFrameCopy != nullptr)
        {
           pFrameCopy->Release();
        }

    }

    void DirectXService::renderOverlays(ID3D11Device* device, ID3D11DeviceContext* context) {


        // This uses some parts of DirectXTK
        // PrimitiveBatch

        batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

        // BasicEffect
        effect.reset(new DirectX::BasicEffect(device));
        effect->SetVertexColorEnabled(true);

        void const* shaderByteCode;
        size_t byteCodeLength;

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        // Set InputLayout
        device->CreateInputLayout(
            DirectX::VertexPositionColor::InputElements,
            DirectX::VertexPositionColor::InputElementCount,
            shaderByteCode,
            byteCodeLength,
            &pInputLayout);
        

        effect->SetView(DirectX::SimpleMath::Matrix::CreateLookAt(DirectX::SimpleMath::Vector3(0.f, 0.f, 1),
            DirectX::SimpleMath::Vector3::Zero, DirectX::SimpleMath::Vector3::UnitY));


        effect->Apply(context);
        context->IASetInputLayout(pInputLayout);

        // Actually draw the line

        batch->Begin();

        float lineLength = 0.1f;
        float xOffset = crosshairX;

        DirectX::VertexPositionColor v1(DirectX::SimpleMath::Vector3(xOffset - lineLength, crosshairY + 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v2(DirectX::SimpleMath::Vector3(xOffset + lineLength, crosshairY + 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v3(DirectX::SimpleMath::Vector3(xOffset - lineLength, crosshairY - 0.003f, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v4(DirectX::SimpleMath::Vector3(xOffset + lineLength, crosshairY - 0.003f, 0), DirectX::Colors::Green);

        batch->DrawQuad(v1, v2, v4, v3);

        DirectX::VertexPositionColor v5(DirectX::SimpleMath::Vector3(xOffset + 0.003f, -lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v6(DirectX::SimpleMath::Vector3(xOffset + 0.003f, lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v7(DirectX::SimpleMath::Vector3(xOffset - 0.003f, -lineLength + crosshairY, 0), DirectX::Colors::Green);
        DirectX::VertexPositionColor v8(DirectX::SimpleMath::Vector3(xOffset - 0.003f, lineLength + crosshairY, 0), DirectX::Colors::Green);

        batch->DrawQuad(v5,v6,v8,v7);

        batch->End();
    }

    bool DirectXService::renderXRFrame(ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv)
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
                &pPixelShader
            );

            if (FAILED(hr)) {
                error("Could not create pixel shader. Error: " + std::to_string(hr));
                return false;
            }

            hr = device->CreateVertexShader(
                gVS,
                ARRAYSIZE(gVS),
                nullptr,
                &pVertexShader
            );

            if (FAILED(hr)) {
                error("Could not create vertex shader. Error: " + std::to_string(hr));
                return false;
            }

            info("Shaders Created.");
            shadersCreated = true;

        }

        // Invalidate the old srv if it changes

        srvCreated = true;
        if (pSRV == nullptr) {
            srvCreated = false;
            info("Invalidated SRV, regenerating.");
        }
        else {
            D3D11_TEXTURE2D_DESC sourceDesc;
            pFrame->GetDesc(&sourceDesc);
            D3D11_TEXTURE2D_DESC copyDesc;
            pFrameCopy->GetDesc(&copyDesc);
            if (copyDesc.Format != sourceDesc.Format
                || copyDesc.Width != sourceDesc.Width
                || copyDesc.Height != sourceDesc.Height) {
                srvCreated = false;
                info("Invalidated SRV, regenerating.");
                pSRV->Release();
                pFrameCopy->Release();
            }
        }

        // Create new resource view if needed
        if (!srvCreated)
        {
            D3D11_TEXTURE2D_DESC textureDesc;
            pFrame->GetDesc(&textureDesc);
            textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            HRESULT hr = device->CreateTexture2D(&textureDesc, nullptr, &pFrameCopy);
            if (FAILED(hr))
            {
                warn("Failed to copy texture");
                return false;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC  srvDesc;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            hr = device->CreateShaderResourceView(pFrameCopy, &srvDesc, &pSRV);
            if (FAILED(hr))
            {
                warn("Failed to create shader resource view");
                return false;
            }
            info("SRV Created.");
            srvCreated = true;
        }

        // Finally, draw.

        context->CopyResource(pFrameCopy, pFrame);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, static_cast<DXGI_FORMAT>(0), 0);
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(pVertexShader, nullptr, 0);
        context->PSSetShader(pPixelShader, nullptr, 0);
        context->OMSetRenderTargets(1, &rtv, nullptr);
        context->PSSetShaderResources(0, 1, &pSRV);
        context->Draw(3, 0);

        renderOverlays(device, context);

        context->Release();
        device->Release();

        return true;
    }
}