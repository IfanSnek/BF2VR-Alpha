#include "DirectXService.h"
#include "OpenXRService.h"
#include "GameService.h"
#include "Utils.h"

using namespace BF2VR;

// Our mod thread
DWORD __stdcall mainThread(HMODULE module)
{
    OwnModule = module;

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    time_t my_time = time(NULL);
    Log << std::endl << std::endl << "New startup at " << ctime(&my_time) << std::endl;

    LoadConfig();

    MH_Initialize();


    log("Attempting to find Battlefront window ...");
    OwnWindow = FindWindowA("Frostbite", "STAR WARS Battlefront II");
    if (!OwnWindow)
    {
        log("Couldn't find window");
        ShutdownNoHooks();
        return 1;
    }
    else {
        log("Success");
    }

    // Resize the window to the largest square
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);

    OpenXRService::EyeHeight = desktop.bottom - 50;
    OpenXRService::EyeWidth = OpenXRService::EyeHeight * RATIO;

    MoveWindow(OwnWindow, 0, 0, OpenXRService::EyeWidth, OpenXRService::EyeHeight, TRUE);

    log("Attempting to start OpenXR ...");
    if (!OpenXRService::CreateXRInstanceWithExtensions()) {
        log("Unable to start vr");
        ShutdownNoHooks();
        return 1;
    }
    else {
        log("Success");
    }

    Sleep(100); // Wait for a few frames to render

    log("Attempting to capture the soldier's position ...");
    if (!GameService::CaptureOrigin()) {
        log("Unable to capture origin. The game will continue, but you may have to recenter with the HOME key yourself.");
    }
    else {
        log("Success");
    }

    log("Attempting to hook DirectX ...");
    if (!DirectXService::HookDirectX(OwnWindow)) {
        log("Unable to Hook DirectX.");
        ShutdownNoHooks();
    }
    else {
        log("Success");
    }

    log("Attempting to hook the BF2 Camera ...");
    if (!GameService::HookCamera()) {
        log("Unable to Hook the BF2 Camera.");
        ShutdownNoHooks();
    }
    else {
        log("Success");
    }

    log("Started Sucessfully");


    for (;;) {

        if (GetAsyncKeyState(VK_HOME)) {
            GameService::Reposition();
        }

        if (GetAsyncKeyState(VK_INSERT)) {
            LoadConfig();
        }

        if (GetAsyncKeyState(VK_END)) {
            Shutdown();
            return 1;
        }
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
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