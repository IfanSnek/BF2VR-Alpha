// DirectXService.h - Headers for the code that interacts with the DirectX API, including hooks and rendering.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#pragma once
#include <d3d11.h>
#include "../../third-party/DirectXTK/PrimitiveBatch.h"
#include "../../third-party/DirectXTK/VertexTypes.h"
#include "../../third-party/DirectXTK/Effects.h"

#include <vector>
#include <map>

namespace BF2VR {
	class DirectXService {
	public:
		static inline bool xrFrameBegan = false;
		static inline bool xrFrameRendered = false;

		static inline ID3D11Device* pDevice = nullptr;
		static inline ID3D11DeviceContext* pContext = nullptr;
		static inline ID3D11Texture2D* pFrame;

		static inline bool shadersCreated = false;
		static inline ID3D11VertexShader* pVertexShader = nullptr;
		static inline ID3D11PixelShader* pPixelShader = nullptr;
		static inline bool srvCreated = false;
		static inline ID3D11ShaderResourceView* pSRV = nullptr;
		static inline ID3D11Texture2D* pFrameCopy = nullptr;

		static inline float crosshairX = 0;
		static inline float crosshairY = 0;

		static bool hookDirectX();
		static void unhookDirectX();
		static void renderOverlays();
		static bool renderXRFrame(ID3D11Texture2D* pTexture, ID3D11RenderTargetView* pRTV);


	private:
		typedef HRESULT(Present)(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags);
		static Present presentDetour;
		static inline Present* pPresentTarget = nullptr;
		static inline Present* pPresentOriginal = nullptr;

		static inline std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> batch;
		static inline std::unique_ptr<DirectX::BasicEffect> effect;
		static inline ID3D11InputLayout* pInputLayout;
	};
}