#include "DirectXService.h"
#include "OpenXRService.h"
#include "GameService.h"
#include "InputService.h"
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

    // Eye resolution stuff

    LoadConfig();

    RECT desktop;
    GetWindowRect(OwnWindow, &desktop);
    MoveWindow(OwnWindow, 0, 0, desktop.bottom * RATIO, desktop.bottom, TRUE);

    OpenXRService::EyeWidth = desktop.bottom * RATIO;
    OpenXRService::EyeHeight = desktop.bottom;


    MH_Initialize();

    log("Attempting to start OpenXR ...");
    if (!OpenXRService::CreateXRInstanceWithExtensions()) {
        log("Unable to start vr");
        ShutdownNoHooks();
        return 1;
    }
    else {
        log("Success");
    }

    Sleep(100);

    log("Attempting to hook DirectX ...");
    if (!DirectXService::HookDirectX(OwnWindow)) {
        log("Unable to Hook DirectX.");
        ShutdownNoHooks();
    }
    else {
        log("Success");
    }

    log("Attempting to start ViGEm ...");
    if (!InputService::Connect()) {
        log("Unable to start ViGEm.");
        DirectXService::UnhookDirectX();
        ShutdownNoHooks();
    }
    else {
        log("Success");
    }

    log("Attempting to hook the BF2 Camera ...");
    if (!GameService::HookCamera()) {
        log("Unable to Hook the BF2 Camera.");
        Shutdown();
    }
    else {
        log("Success");
    }

    log("Started Sucessfully");


    for (;;) {

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