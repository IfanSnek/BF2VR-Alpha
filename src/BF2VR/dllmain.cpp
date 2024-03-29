// dllmain.cpp - Entry point and main setup/loop for the mod.
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
#include "OpenXRService.h"
#include "GameService.h"
#include "InputService.h"
#include "Utils.h"

using namespace BF2VR;

// Main mod thread
DWORD __stdcall mainThread(HMODULE module)
{
    ownModule = module;

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    time_t my_time = time(NULL);
    std::cout << "New startup at " << ctime(&my_time) << std::endl;
    logFile << std::endl << std::endl << "New startup at " << ctime(&my_time) << std::endl;

    log("Attempting to find Battlefront window ...");
    ownWindow = FindWindowA("Frostbite", "STAR WARS Battlefront II");
    if (!ownWindow)
    {
        error("Couldn't find window.");
        shutdownNoHooks();
        return 1;
    }
    else {
        success("Found window.");
    }

    loadConfig();

    // Eye resolution stuff
    RECT desktop;
    GetWindowRect(ownWindow, &desktop);
    
    OpenXRService::eyeWidth = desktop.right;
    OpenXRService::eyeHeight = desktop.bottom;

    MH_Initialize();

    // Initialize OpenXR
    log("Attempting to initialize OpenXR ...");
    if (!OpenXRService::createXRInstance()) {
        error("Unable to initialize vr");
        shutdownNoHooks();
        return 1;
    }
    else {
        success("Initialized OpenXR.");
    }

    // Hook DirectX. Present will be the main logic loop.
    log("Attempting to hook DirectX ...");
    if (!DirectXService::hookDirectX()) {
        error("Unable to Hook DirectX.");
        shutdownNoHooks();
        return 1;
    }
    else {
        success("Hooked DirectX.");
    }

    log("Attempting to start ViGEm ...");
    if (!InputService::connect()) {
        error("Unable to start ViGEm.");
        DirectXService::unhookDirectX();
        shutdownNoHooks();
        return 1;
    }
    else {
        success("Started ViGEm.");
    }

    log("Attempting to hook the BF2 Camera ...");
    if (!GameService::enableHooks()) {
        error("Unable to Hook the BF2 Camera.");
        shutdown();
        return 1;
    }
    else {
        success("Hooked the BF2 Camera.");
    }

    info("Started Sucessfully");

    for (;;) {
        Sleep(200);
        if (GetAsyncKeyState(VK_END)) {
            shutdown();
            return 0;
        }
        if (GetAsyncKeyState(VK_HOME)) {
            loadConfig();
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