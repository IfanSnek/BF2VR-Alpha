#pragma once
#include "SDK.h"
#include <wtypes.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include "../../third-party/OpenVR/openvr.h"
#include "../../third-party/minhook/MinHook.h"

std::ofstream Log("logs.txt", std::ios_base::app);
//std::ifstream Config("Config.txt");
bool switcheyes = FALSE;

std::string Level = "";

HMODULE hOwnModule;

vr::IVRSystem* m_pHMD;

void* g_RenderView = NULL;
Vec4 CameraPosition = { 0, 0, 0, 0 };
static Matrix4x4 g_Transform;
static Matrix4x4 g_Origin;
bool lefteye = TRUE;

ID3D11Device* pDevice;
IDXGISwapChain* pSwapChain;
ID3D11DeviceContext* pContext;

PVOID pPresent;
PVOID pResizeBuffers;

bool DXReady = TRUE;