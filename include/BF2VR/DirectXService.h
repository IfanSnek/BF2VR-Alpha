#pragma once
#include <d3d11.h>

#include <vector>
#include <map>

namespace BF2VR {
	class DirectXService {
	public:
		static inline bool FrameBegan = false;
		static inline bool FrameRendered = false;
		static inline bool DoPresent = true;

		static inline ID3D11Device* pDevice = nullptr;
		static inline ID3D11DeviceContext* pContext;
		static inline ID3D11Texture2D* CurrentFrame;

		static inline bool shadersCreated = false;
		static inline ID3D11VertexShader* VertexShaderRight = nullptr;
		static inline ID3D11VertexShader* VertexShaderLeft = nullptr;
		static inline ID3D11PixelShader* PixelShader = nullptr;
		static inline bool srvCreated = false;
		static inline ID3D11ShaderResourceView* srv = nullptr;
		static inline ID3D11Texture2D* copy = nullptr;

		static bool HookDirectX(HWND window);
		static void UnhookDirectX();
		static bool RenderXRFrame(ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv);


	private:

		typedef HRESULT(Present)(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags);
		static Present PresentDetour;
		static inline Present* PresentTarget = nullptr;
		static inline Present* PresentOriginal = nullptr;

		struct EyeBuffer {
			bool leftEye;
		};
	};
}