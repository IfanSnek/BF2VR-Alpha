#include <Windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Globals.h"
#include "HookHelper.h"
#include "Utils/Matrices.h"
#include <time.h>
#pragma warning(disable: 4703) // Make the compiler shut up about null pointers that Minhook will assign

bool IsValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr; }

// Start up OpenVR with the correct settings
bool InitVR() {

    // Try to connect with openvr as an background application
    vr::EVRInitError eError = vr::VRInitError_None;
    g_HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None)
    {
        g_HMD = NULL;
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
    if (!IsValidPtr(g_HMD)) {
        return false;
    }
    for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
    {
        if (!g_HMD->IsTrackedDeviceConnected(unDevice))
            continue;

        vr::VRControllerState_t state;
        if (g_HMD->GetControllerState(unDevice, &state, sizeof(state)))
        {
            vr::TrackedDevicePose_t trackedDevicePose;
            vr::HmdMatrix34_t matrix;
            vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);


            if (trackedDeviceClass == device) {
                g_HMD->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &trackedDevicePose, 1);


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
        else {
            std::cout << "Device bad index" << std::endl;
            Log << "Device bad index" << std::endl;

        }
    }
    std::cout << "Device never found" << std::endl;
    Log << "Device never found" << std::endl;

    return false;
}

// Converts quats to euler for the Aiming function
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

// Original and hooked camera functions
typedef __int64(*__fastcall tupdateCamera2)(class CameraObject*, class CameraObject*);
tupdateCamera2 OriginalCamera = nullptr;
__int64 __fastcall CameraHook(CameraObject* a1, CameraObject* a2)
{
    if (a2 == g_RenderView) {
        a2->cameraTransform = g_Transform;

    }
    return OriginalCamera(a1, a2);
}

// Function to set the view of the game camera
void UpdateCamera(Vec3 location, vr::HmdMatrix34_t matrix, float yaw, float pitch) {

    GameRenderer* pGameRenderer = GameRenderer::GetInstance();
    if (!IsValidPtr(pGameRenderer))
    {
        return;
    }
    GameRenderSettings* pSettings = pGameRenderer->gameRenderSettings;
    if (!IsValidPtr(pSettings))
    {
        return;
    }
    pSettings->forceFov = g_FOV;

    auto RotationMatrix = matrix.m;

    Matrix4 specialOpsMatrix;
    Matrix4 hmd_rot;
    Matrix4x4 m = g_Origin;

    // Matrix for the HMD rotation
    hmd_rot.set(
        RotationMatrix[0][0], RotationMatrix[0][1], RotationMatrix[0][2], 0,
        RotationMatrix[1][0], RotationMatrix[1][1], RotationMatrix[1][2], 0,
        RotationMatrix[2][0], RotationMatrix[2][1], RotationMatrix[2][2], 0,
        0, 0, 0, 1
    );

    // Matrix for the origin
    specialOpsMatrix.set(
        m.x.x, m.x.y, m.x.z, m.x.w,
        m.y.x, m.y.y, m.y.z, m.y.w,
        m.z.x, m.z.y, m.z.z, m.z.w,
        m.o.x, m.o.y, m.o.z, m.o.w
    );

    hmd_rot.invert();
    specialOpsMatrix = specialOpsMatrix * hmd_rot;

    // Convert back to Matrix4x4
    Matrix4x4 out;
    out.x.x = specialOpsMatrix[0]; out.x.y = specialOpsMatrix[1]; out.x.z = specialOpsMatrix[2]; out.x.w = specialOpsMatrix[3];
    out.y.x = specialOpsMatrix[4]; out.y.y = specialOpsMatrix[5]; out.y.z = specialOpsMatrix[6]; out.y.w = specialOpsMatrix[7];
    out.z.x = specialOpsMatrix[8]; out.z.y = specialOpsMatrix[9]; out.z.z = specialOpsMatrix[10]; out.z.w = specialOpsMatrix[11];
    out.o.x = specialOpsMatrix[12]; out.o.y = specialOpsMatrix[13]; out.o.z = specialOpsMatrix[14]; out.o.w = specialOpsMatrix[15];

    // Set location from HMD
    out.o.x = -location.x;
    out.o.y = location.y;
    out.o.z = -location.z;
    out.o.w = 1;

    // Get some game members, validating pointers along the way
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

    if (levelname != g_Level)
    {
        // Check for when the level name changes
        std::cout << "Switched to " << levelname << std::endl;
        Log << "Switched to " << levelname << std::endl;
        g_Level = levelname;
    }

    if (levelname == "Levels/FrontEnd/FrontEnd")
    {
        // TODO: Calibrate Values (soldier is not available)
    }
    else
    {
        // Get more members, again, checking along the way
        PlayerManager* playerManager = CurrentContext->GetPlayerManager();
        if (!IsValidPtr(playerManager)) {
            return;
        }

        ClientPlayer* player = playerManager->GetLocalPlayer();
        if (!IsValidPtr(player)) {
            return;
        }

        ClientSoldierEntity* soldier = player->GetClientSoldier();
        if (!IsValidPtr(soldier)) {
            return;
        }

        ClientSoldierPrediction* prediction = soldier->clientSoldierPrediction;
        if (!IsValidPtr(prediction)) {
            return;
        }

        Vec3 playerPosition = prediction->Location;
        float heightOffset = soldier->HeightOffset;

        out.o.x += playerPosition.x;
        out.o.y += playerPosition.y - heightOffset + g_VRHeight;
        out.o.z += playerPosition.z;


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

        // Set the gun's aim
        if (g_UpdateAim)
        {
            p2->pitch = pitch + g_AimPitchOffset;
            p2->yaw = yaw + g_AimYawOffset;
        }
        g_AimPitch = pitch;
        g_AimYaw = yaw;
    }

    // Switch eye every other frame. Switch happens on DirectX present
    if (g_lefteye) {
        out.o.x += g_IPD / 2;
    }
    else {
        out.o.x -= g_IPD / 2;
    }

    // Update the transform that the CameraHook will use
    g_Transform = out;

}

// Hook and original frame render present functions
HRESULT (*Present)(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags) = nullptr;
HRESULT PresentHook(IDXGISwapChain* pInstance, UINT SyncInterval, UINT Flags)
{

    if (IsValidPtr(g_HMD) && g_DXReady) {

        if (g_lefteye) {
            // Wait get poses when we get back to the first eye, it is required before submitting both eyes again
            vr::TrackedDevicePose_t rawPoses[vr::k_unMaxTrackedDeviceCount];
            vr::VRCompositor()->WaitGetPoses(rawPoses, vr::k_unMaxTrackedDeviceCount, NULL, 0);
        }

        // Get the color buffer from the screen
        ID3D11Texture2D* texture;
        HRESULT hr = pInstance->GetBuffer(0, IID_PPV_ARGS(&texture));
        if (SUCCEEDED(hr))
        {
            // Convert the buffer to OpenVR's type
            vr::Texture_t eye_texture = { (void*)texture, vr::TextureType_DirectX, vr::ColorSpace_Gamma };

            // Bounds determine the mapping of the buffer to the VR screen. See the config.
            vr::VRTextureBounds_t bounds;
            bounds.vMin = 0.0f;
            bounds.vMax = 1.0f;

            if (g_lefteye) {
                bounds.uMin = g_leftmin;
                bounds.uMax = g_leftmax;
                vr::VRCompositor()->Submit(vr::Eye_Left, &eye_texture, &bounds);
            }
            else {
                bounds.uMin = g_rightmin;
                bounds.uMax = g_rightmax;
                vr::VRCompositor()->Submit(vr::Eye_Right, &eye_texture, &bounds);

            }

            // Switch eyes
            g_lefteye = !g_lefteye;

            texture->Release();

        }

    }
    else {
        std::cout << "VR not ready yet." << std::endl;
        Log << "VR not ready yet." << std::endl;
    }
    return Present(pInstance, SyncInterval, Flags);
}

// Shutdown and eject the mod. Currently only works before DirextX gets hooked.
void Shutdown() {
    Log << "Removed Camera" << std::endl;
    HookHelper::DestroyHook(OffsetCamera);

    Log << "Shutdown VR" << std::endl;
    vr::VR_Shutdown();

    // TODO: LEt the mod eject DirectX nicely
    //Log << "Removed DX" << std::endl;
    //g_DXReady = FALSE;

    std::cout << "You may now close this window." << std::endl;
    Log << "End of log." << std::endl;
    FreeConsole();
    FreeLibraryAndExitThread(g_hOwnModule, 0);
}

// Hook the renderer
static void HookRenderer() {

    // Some nice little settings for our dummy device
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

    // Finish setting up  our dummy
    desc.OutputWindow = hWnd;
    desc.Windowed = true;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Create the dummy
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel, 1, D3D11_SDK_VERSION, &desc, &pSwapChain, &pDevice, nullptr, &pContext)))
    {
        std::cout << "Couldn't create DX device" << std::endl;
        Shutdown();
    }

    // Get our Present pointer
    auto pTable = *reinterpret_cast<PVOID**>(pSwapChain);
    pPresent = pTable[8];

    // Hook our dummy DirectX stuff into the real one
    MH_CreateHook(pPresent, PresentHook, reinterpret_cast<PVOID*>(&Present));
    MH_EnableHook(pPresent);

    // Resize the window to the largest square
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    long vertical = desktop.bottom - 50;
    if (hWnd != NULL) { MoveWindow(hWnd, 0, 0, vertical * g_RATIO, vertical, TRUE); }

}

// Reorient the VR
void Reposition() {

    // Turn off the camera update, getting the real player position
    HookHelper::DestroyHook(OffsetCamera);
    // Same with the aim, but capture the current positions first.
    float oldPitch = g_AimPitch;
    float oldYaw = g_AimYaw;
    g_UpdateAim = FALSE;

    Sleep(100);

    // Capture the origins
    g_Origin = GameRenderer::GetInstance()->renderView->transform;
    g_AimPitchOffset = g_AimPitch - oldPitch;
    g_AimYawOffset = g_AimYaw - oldYaw;

    // Reenable the hooks
    HookHelper::CreateHook(OffsetCamera, &CameraHook, &OriginalCamera);
    g_UpdateAim = TRUE;

    std::cout << "Repositioned" << std::endl;
    Log << "Repositioned" << std::endl;
}

void LoadConfig() {
    // Open config.txt and read it line by line, applying the values

    std::cout << "Loading config..." << std::endl;
    Log << "Loading config..." << std::endl;
    std::string text;
    std::string MultiLineProperty = "";

    std::ifstream Config("config.txt");
    while (getline(Config, text)) {

        if (MultiLineProperty != "")
        {
            // Continue where we left off

            if (MultiLineProperty == "IPDGAMEUNITS")
            {
                g_IPD = std::stof(text);
            }
            else if (MultiLineProperty == "EYERATIO")
            {
                g_RATIO = std::stof(text);
            }
            else if (MultiLineProperty == "MAPPINGLEFTMIN")
            {
                g_leftmin = std::stof(text);
            }
            else if (MultiLineProperty == "MAPPINGLEFTMAX")
            {
                g_leftmax = std::stof(text);
            }
            else if (MultiLineProperty == "MAPPINGRIGHTMIN")
            {
                g_rightmin = std::stof(text);
            }
            else if (MultiLineProperty == "MAPPINGRIGHTMAX")
            {
                g_rightmax = std::stof(text);
            }
            else if (MultiLineProperty == "VRHEIGHT")
            {
                g_VRHeight = std::stof(text);
            }
            else if (MultiLineProperty == "FOV")
            {
                g_FOV = std::stof(text);
            }

            std::cout << MultiLineProperty << "=" << text << std::endl;
            MultiLineProperty = "";
        }
        // Boolean parameters
        else if (FALSE)
        {

        }
        // Value parameters
        else
        {
            MultiLineProperty = text;
        }
    }
    Config.close();

    std::cout << "Loaded config" << std::endl;
    Log << "Loaded config" << std::endl;

    // Prevent multi-triggering
    Sleep(100);
}

// Our mod thread
DWORD __stdcall mainThread(HMODULE module)
{
    g_hOwnModule = module;

    // Open a console
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    // Timestamp startup
    time_t my_time = time(NULL);
    Log << std::endl << std::endl << "New startup at " << ctime(&my_time) << std::endl;

    // Load config
    LoadConfig();
    
    // Get a reference to the game's render view. This only updates the camera, and not the HUD or aim.
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

    // Before we start setting the custom view, get the original to make our custom view relative
    g_Origin = GameRenderer::GetInstance()->renderView->transform;
    std::cout << "Origin Captured" << std::endl;
    Log << "Origin Captured" << std::endl;

    // Then hook the camera, now it will display the view from g_CameraPosition (which is 0)
    HookHelper::CreateHook(OffsetCamera, &CameraHook, &OriginalCamera);
    std::cout << "Game Camera Hooked" << std::endl;
    Log << "Game Camera Hooked" << std::endl;

    // Initialize the OpenVR runtime
    if (!InitVR()) {
        Shutdown();
    }

    // Hook the DirectX present function to get frames when they are rendered
    HookRenderer();
    std::cout << "Renderer Hooked" << std::endl;
    Log << "Renderer Hooked" << std::endl;

    // All tests passed.
    std::cout << "Started Succesfully" << std::endl;
    Log << "Started Succesfully" << std::endl;

    // Main mod loop
    for (;;) {

        Vec3 hmd_location;
        Vec4 hmd_quatrotation;
        vr::HmdMatrix34_t hmd_transformationmatrix;

        if (!GetVectors(vr::TrackedDeviceClass_HMD, hmd_location, hmd_quatrotation, hmd_transformationmatrix)) {
            // This will fail if g_HMD is null or no device is connected
            std::cout << "Camera not updated." << std::endl;
            Log << "Camera not updated" << std::endl;
            continue;
        }
        Vec3 hmd_eulerrotation = eulerFromQuat(hmd_quatrotation);
        UpdateCamera(hmd_location, hmd_transformationmatrix, -hmd_eulerrotation.x, hmd_eulerrotation.z);


        if (GetAsyncKeyState(VK_HOME)) {
            Reposition();
        }

        if (GetAsyncKeyState(VK_INSERT)) {
            LoadConfig();

            // Update Window
            RECT desktop;
            const HWND hDesktop = GetDesktopWindow();
            GetWindowRect(hDesktop, &desktop);
            long vertical = desktop.bottom - 50;
            HWND hWnd = FindWindowA("Frostbite", "STAR WARS Battlefront II");
            if (hWnd != NULL) { MoveWindow(hWnd, 0, 0, vertical * g_RATIO, vertical, TRUE); }
        }

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

