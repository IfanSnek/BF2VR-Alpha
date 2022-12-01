#pragma once
#include "SDK.h"
#include <wtypes.h>
#include <d3d11.h>
#include <iostream>
#include <fstream>
#include "../../third-party/OpenVR/openvr.h"
#include "../../third-party/minhook/MinHook.h"

// Create a file to write logs to in append mode
std::ofstream Log("logs.txt", std::ios_base::app);

// Config settings
float g_IPD = 0.0064f;
float g_RATIO = 0.95416f;
float g_leftmin = -0.1f;
float g_leftmax = 1.0f;
float g_rightmin = 0.1f;
float g_rightmax = 1.1f;
float g_VRHeight = 2.0f;
float g_FOV = 91.0f;

// Store the current level
std::string g_Level = "";

// Reference to this mod's module
HMODULE g_hOwnModule;

// VR Instance
vr::IVRSystem* g_HMD;

// Game view states
void* g_RenderView = NULL;
Vec4 g_CameraPosition = { 0, 0, 0, 0 };
static Matrix4x4 g_Transform;
static Matrix4x4 g_Origin;
bool g_lefteye = TRUE;
bool g_UpdateAim = TRUE;
float g_AimYaw = 0;
float g_AimPitch = 0;
float g_AimYawOffset = 0;
float g_AimPitchOffset = 0;

// DirectX stuff
ID3D11Device* pDevice;
IDXGISwapChain* pSwapChain;
ID3D11DeviceContext* pContext;

PVOID pPresent;
PVOID pResizeBuffers;

bool g_DXReady = TRUE;