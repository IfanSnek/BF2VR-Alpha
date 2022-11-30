#include <Windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Globals.h"
#include "HookHelper.h"
#include "Matrices.h"
#include <time.h>

#pragma warning(disable: 4703)

bool IsValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr; }

// Function to start the VR Runtime.
bool InitVR() {

    // try to connect with openvr as an background application
    vr::EVRInitError eError = vr::VRInitError_None;
    m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None)
    {
        m_pHMD = NULL;
        std::cout << "Unable to init vr runtime." << eError << std::endl;
        Log << "Unable to init vr runtime." << eError << std::endl;
        return false;
    }

    if (!vr::VRCompositor()) {
        std::cout << "Failed to initialize compositor." << std::endl;
        Log << "Failed to initialize compositor." << std::endl;
        return false;
    }
    else {
        std::cout << "Successfully initialized VR Compositor" << std::endl;
        Log << "Successfully initialized VR Compositor" << std::endl;
    }

    std::cout << "Started VR" << std::endl;
    Log << "Started VR" << std::endl;
    return true;
}

// Function to get the position of a vr HMD or controller
bool GetVectors(vr::ETrackedDeviceClass device, Vec3& Loc, Vec4& Rot, vr::HmdMatrix34_t& outMatrix)
{
    if (!IsValidPtr(m_pHMD)) {
        return false;
    }
    for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
    {
        if (!m_pHMD->IsTrackedDeviceConnected(unDevice))
            continue;

        vr::VRControllerState_t state;
        if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state)))
        {
            vr::TrackedDevicePose_t trackedDevicePose;
            vr::HmdMatrix34_t matrix;
            vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);


            if (trackedDeviceClass == device) {
                m_pHMD->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &trackedDevicePose, 1);


                matrix = trackedDevicePose.mDeviceToAbsoluteTracking;

                vr::HmdQuaternion_t quat;

                quat.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
                quat.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
                quat.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
                quat.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
                quat.x = copysign(quat.x, matrix.m[2][1] - matrix.m[1][2]);
                quat.y = copysign(quat.y, matrix.m[0][2] - matrix.m[2][0]);
                quat.z = copysign(quat.z, matrix.m[1][0] - matrix.m[0][1]);

                Loc.x = matrix.m[0][3];
                Loc.y = matrix.m[1][3];
                Loc.z = matrix.m[2][3];

                Rot.w = quat.w;
                Rot.x = quat.x;
                Rot.y = quat.y;
                Rot.z = quat.z;

                outMatrix = matrix;

                return true;

            }
        }
    }
}

Vec3 eulerFromQuat(Vec4 q)
{
    #define PI 3.14159265358979323846
    Vec3 v;

    double test = q.x * q.y + q.z * q.w;
    if (test > 0.499)
    { // singularity at north pole
        v.x = 2 * atan2(q.x, q.w); // heading
        v.y = PI / 2; // attitude
        v.z = 0; // bank
        return v;
    }
    if (test < -0.499)
    { // singularity at south pole
        v.x = -2 * atan2(q.x, q.w); // headingq
        v.y = -PI / 2; // attitude
        v.z = 0; // bank
        return v;
    }
    double sqx = q.x * q.x;
    double sqy = q.y * q.y;
    double sqz = q.z * q.z;
    v.x = atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * sqy - 2 * sqz); // heading
    v.y = asin(2 * test); // attitude
    v.z = atan2(2 * q.x * q.w - 2 * q.y * q.z, 1 - 2 * sqx - 2 * sqz); // bank
    return v;
}

// Function to set the view of the game camera
typedef __int64(*__fastcall tupdateCamera2)(class CameraObject*, class CameraObject*);
tupdateCamera2 OriginalCamera = nullptr;
void UpdateCamera(Vec3 Loc, vr::HmdMatrix34_t Rot, float yaw, float pitch) {

    GameRenderer* pGameRenderer = GameRenderer::GetInstance();
    if (!IsValidPtr(pGameRenderer))
    {
        return;
    }

    auto r = Rot.m;

    Matrix4 matrix;
    Matrix4 hmd_rot;
    Matrix4x4 m = g_Origin;

    // Matrix for the HMD rotation
    hmd_rot.set(
        r[0][0], r[0][1], r[0][2], 0,
        r[1][0], r[1][1], r[1][2], 0,
        r[2][0], r[2][1], r[2][2], 0,
        0, 0, 0, 1
    );

    // Matrix for the origin
    matrix.set(
        m.x.x, m.x.y, m.x.z, m.x.w,
        m.y.x, m.y.y, m.y.z, m.y.w,
        m.z.x, m.z.y, m.z.z, m.z.w,
        m.o.x, m.o.y, m.o.z, m.o.w
    );

    hmd_rot.invert();
    matrix = matrix * hmd_rot;


    // Convert back to Matrix4x4
    Matrix4x4 out;
    out.x.x = matrix[0]; out.x.y = matrix[1]; out.x.z = matrix[2]; out.x.w = matrix[3];
    out.y.x = matrix[4]; out.y.y = matrix[5]; out.y.z = matrix[6]; out.y.w = matrix[7];
    out.z.x = matrix[8]; out.z.y = matrix[9]; out.z.z = matrix[10]; out.z.w = matrix[11];
    out.o.x = matrix[12]; out.o.y = matrix[13]; out.o.z = matrix[14]; out.o.w = matrix[15];

    // Set location from HMD
    out.o.x = -Loc.x;
    out.o.y = Loc.y;
    out.o.z = -Loc.z;
    out.o.w = 1;

    GameContext* CurrentContext = GameContext::GetInstance();
    if (!IsValidPtr(CurrentContext)) {
        return;
    }
    ClientLevel* CurrentLevel = CurrentContext->GetClientLevel();
    if (!IsValidPtr(CurrentLevel)) {
        return;
    }
    if (!IsValidPtr(CurrentLevel->LevelName)) {
        return;
    }
    std::string levelname = CurrentLevel->LevelName;

    if (levelname != Level)
    {
        std::cout << "Switched to " << levelname << std::endl;
        Log << "Switched to " << levelname << std::endl;
        Level = levelname;
    }

    if (levelname == "Levels/FrontEnd/FrontEnd")
    {
        // Calibrate Values
    }
    else
    {
        PlayerManager* PM = CurrentContext->GetPlayerManager();
        if (!IsValidPtr(PM)) {
            return;
        }

        ClientPlayer* CLP = PM->GetLocalPlayer();
        if (!IsValidPtr(CLP)) {
            return;
        }

        ClientSoldierEntity* Soldier = CLP->GetClientSoldier();
        if (!IsValidPtr(Soldier)) {
            return;
        }

        ClientSoldierPrediction* Prediction = Soldier->clientSoldierPrediction;
        if (!IsValidPtr(Prediction)) {
            return;
        }

        Vec3 playerPos = Prediction->Location;
        float offset = Soldier->HeightOffset;

        out.o.x += playerPos.x;
        out.o.y += playerPos.y - offset + 2;
        out.o.z += playerPos.z;


        LocalAimer* aimer = LocalAimer::Instance();
        if (!IsValidPtr(aimer))
        {
            return;
        }

        UnknownPtr1* p1 = aimer->UnknownPtr1;
        if (!IsValidPtr(p1))
        {
            return;
        }
        UnknownPtr2* p2 = p1->UnknownPtr2;
        if (!IsValidPtr(p2))
        {
            return;
        }
        p2->pitch = pitch;
        p2->yaw = yaw;
    }

    if (lefteye) {
        out.o.x += 0.00065;
    }
    else {
        out.o.x -= 0.00065;
    }

    g_Transform = out;

}

// Hook and original frame render present functions
HRESULT (*Present)(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags) = nullptr;
HRESULT PresentHook(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags)
{

    if (IsValidPtr(m_pHMD) && DXReady) {

        if (lefteye) {
            vr::TrackedDevicePose_t rawPoses[vr::k_unMaxTrackedDeviceCount];
            vr::VRCompositor()->WaitGetPoses(rawPoses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
        }

        ID3D11Texture2D* texture;
        HRESULT hr = pInstance->GetBuffer(0, IID_PPV_ARGS(&texture));
        if (SUCCEEDED(hr))
        {
            vr::Texture_t eye = { (void*)texture, vr::TextureType_DirectX, vr::ColorSpace_Gamma };

            vr::VRTextureBounds_t bounds;
            if (lefteye) {
                bounds.uMin = 0.0f;
                bounds.uMax = 0.9f;
                bounds.vMin = 0.0f;
                bounds.vMax = 1.0f;
                vr::VRCompositor()->Submit(vr::Eye_Left, &eye, &bounds);
            }
            else {
                bounds.uMin = 0.1f;
                bounds.uMax = 1.0f;
                bounds.vMin = 0.0f;
                bounds.vMax = 1.0f;
                vr::VRCompositor()->Submit(vr::Eye_Right, &eye, &bounds);

            }

            lefteye = !lefteye;

            texture->Release();

        }

    }
    else {
        std::cout << "VR not ready yet." << std::endl;
        Log << "VR not ready yet." << std::endl;
    }
    return Present(pInstance, SyncInterval, Flags);
}


// Camera hooked function
__int64 __fastcall CameraHook(CameraObject* a1, CameraObject* a2)
{
    if (a2 == g_RenderView) {
        a2->cameraTransform = g_Transform;

    }
    return OriginalCamera(a1, a2);
}

// Shutdown and eject the mod
void Shutdown() {
    Log << "Removed Camera" << std::endl;
    HookHelper::DestroyHook(OffsetCamera);

    Log << "Shutdown VR" << std::endl;
    vr::VR_Shutdown();

    Log << "Removed DX" << std::endl;
    DXReady = FALSE;

    std::cout << "You may now close this window." << std::endl;
    Log << "End of log." << std::endl;
    FreeConsole();
    FreeLibraryAndExitThread(hOwnModule, 0);
}

// Hook the renderer
static void HookRenderer() {

    auto featureLevel = D3D_FEATURE_LEVEL_11_0;

    DXGI_SWAP_CHAIN_DESC desc = { };

    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;

    std::cout << "Attempting to find Battlefront window" << std::endl;

    HWND hWnd = FindWindowA("Frostbite", "STAR WARS Battlefront II");

    if (!hWnd)
    {
        std::cout << "Couldn't find window" << std::endl;
        Shutdown();
    }

    desc.OutputWindow = hWnd;
    desc.Windowed = true;

    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel, 1, D3D11_SDK_VERSION, &desc, &pSwapChain, &pDevice, nullptr, &pContext)))
    {
        std::cout << "Couldn't create DX device" << std::endl;
        Shutdown();
    }

    auto pTable = *reinterpret_cast<PVOID**>(pSwapChain);

    pPresent = pTable[8];
    pResizeBuffers = pTable[13];

    MH_CreateHook(pPresent, PresentHook, reinterpret_cast<PVOID*>(&Present));
    MH_EnableHook(pPresent);

    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    long vertical = desktop.bottom - 50;

    if (hWnd != NULL) { MoveWindow(hWnd, 0, 0, vertical, vertical, TRUE); }

}

// Reorient the VR
void Reposition() {

    HookHelper::DestroyHook(OffsetCamera);

    Sleep(100);

    std::cout << "Repositioned" << std::endl;
    Log << "Repositioned" << std::endl;
    g_Origin = GameRenderer::GetInstance()->renderView->transform;
    HookHelper::CreateHook(OffsetCamera, &CameraHook, &OriginalCamera);
}

// Main mod loop
DWORD __stdcall mainThread(HMODULE module)
{
    hOwnModule = module;
    // Open a console
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    time_t my_time = time(NULL);
    Log << std::endl << std::endl << "New startup at " << ctime(&my_time) << std::endl;


    /*
    std::cout << "Loading config..." << std::endl;
    Log << "Loading config..." << std::endl;
    std::string text;
    while (getline(Config, text)) {
        if (text == "switcheyes")
        {
            switcheyes = TRUE;
        }
            
    }

    std::cout << "Loaded config" << std::endl;
    Log << "Loaded config" << std::endl;
    */
    g_RenderView = GameRenderer::GetInstance()->renderView;
    if (!IsValidPtr(g_RenderView))
    {
        std::cout << "Game Renderer Hook Failed!" << std::endl;
        Log << "Game Renderer Hook Failed!" << std::endl;
        Shutdown();
    }
    std::cout << "Game Renderer Hooked" << std::endl;
    Log << "Game Renderer Hooked" << std::endl;

    Sleep(1000); // Wait for a few frames to render

    g_Origin = GameRenderer::GetInstance()->renderView->transform;
    std::cout << "Origin Captured" << std::endl;
    Log << "Origin Captured" << std::endl;

    HookHelper::CreateHook(OffsetCamera, &CameraHook, &OriginalCamera);
    std::cout << "Game Camera Hooked" << std::endl;
    Log << "Game Camera Hooked" << std::endl;

    if (!InitVR()) {
        Shutdown();
    }

    HookRenderer();
    std::cout << "Renderer Hooked" << std::endl;
    Log << "Renderer Hooked" << std::endl;

    std::cout << "Started Succesfully" << std::endl;
    Log << "Started Succesfully" << std::endl;

    // Run loop until end key is pressed
    for (;;) {

        Vec3 l;
        Vec4 r;
        vr::HmdMatrix34_t mat;

        if (!GetVectors(vr::TrackedDeviceClass_HMD, l, r, mat)) {
            std::cout << "Camera not updated." << std::endl;
            Log << "Camera not updated" << std::endl;
            continue;
        }
        Vec3 e = eulerFromQuat(r);
        UpdateCamera(l, mat, -e.x, e.z);


        if (GetAsyncKeyState(VK_HOME)) {
            Reposition();
        }

        //if (GetAsyncKeyState(VK_END)) {
        //    Shutdown();
        //}

    }

    return 0;

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)mainThread, hModule, NULL, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
    break;
    }
    return TRUE;
}

