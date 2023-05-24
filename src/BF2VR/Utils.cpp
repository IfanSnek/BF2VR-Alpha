// Utils.cpp - Various utilities like logging, config loading and saving,
// and a few math ones.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include "Utils.h"
#include "GameService.h"
#include <string>
#include "DirectXService.h"
#include "OpenXRService.h"
#include "InputService.h"

#include "../../third-party/INI.h"

namespace BF2VR
{
    // Print to console and a file
    void log(std::string message) {
        logFile << message << std::endl;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        std::cout << message << std::endl;
    }
    void error(std::string message) {
        logFile << message << std::endl;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED);
        std::cout << message << std::endl;
    }
    void warn(std::string message) {

        if (warnCount == -1)
        {
            return;
        }

        if (warnCount > 50)
        {
            error("Too many warnings. Halting log.");
            warnCount = -1;
            return;
        }

        if (message == lastWarn)
        {
            if (message == "Mod has no focus. Please return to the game.")
            {
                return;
            }
            warnCount++;
        }
        else {
            warnCount = 0;
        }
        logFile << message << std::endl;

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_GREEN);
        std::cout << message << std::endl;

        lastWarn = message;
    }
    void success(std::string message) {
        logFile << message << std::endl;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_GREEN);
        std::cout << message << std::endl;
    }
    void info(std::string message) {
        logFile << message << std::endl;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::cout << message << std::endl;
    }
    void deb(std::string message) {
        logFile << message << std::endl;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_BLUE);
        std::cout << message << std::endl;
    }

    // Shutdown and eject the mod.
    void shutdown() {
        log("Ending VR");
        OpenXRService::shouldStop = true;
        while (!OpenXRService::stopping) {
            Sleep(10);
        }
        
        OpenXRService::endXR();
        OpenXRService::isVRReady = false;

        log("Unhooking Camera");
        MH_DisableHook((LPVOID)OFFSETCAMERA);
        MH_RemoveHook((LPVOID)OFFSETCAMERA);

        log("Unhooking Pose");
        MH_DisableHook((LPVOID)OFFSETPOSE);
        MH_RemoveHook((LPVOID)OFFSETPOSE);

        log("Unhooking DirectX");
        DirectXService::unhookDirectX();

        // Reset override FOV
        GameRenderer* pGameRenderer = GameRenderer::GetInstance();
        if (!isValidPtr(pGameRenderer))
        {
            warn("Unable to restore FOV to default value.");
        }
        else {
            GameRenderSettings* pSettings = pGameRenderer->gameRenderSettings;
            if (!isValidPtr(pSettings))
            {
                warn("Unable to restore FOV to default value.");
            }
            else {
                pSettings->forceFov = -1;
            }
        }

        log("Disconnecting ViGEm");
        InputService::disconnect();

        shutdownNoHooks();
    }

    // Shutdown without removing hooks. This is for early initialization when nothing has been hooked yet.
    void shutdownNoHooks() {
        log("Uninitializing Minhook");
        MH_Uninitialize();

        info("Mod shut down.");

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
        std::cout << "You may now close this window." << std::endl;
        logFile << "End of log." << std::endl;

        FreeConsole();
        FreeLibraryAndExitThread(ownModule, 0);
    }


    void loadConfig() {
        log("Loading Config");

        // Open config.txt and read it line by line, applying the values

        std::ofstream Config("config.ini", std::ios_base::app);
        Config.close();

        // Set global parsing/saving options
        INI::PARSE_FLAGS = INI::PARSE_COMMENTS_ALL | INI::PARSE_COMMENTS_SLASH | INI::PARSE_COMMENTS_HASH;
        INI::SAVE_FLAGS = INI::SAVE_PRUNE | INI::SAVE_PADDING_SECTIONS | INI::SAVE_SPACE_SECTIONS | INI::SAVE_SPACE_KEYS | INI::SAVE_TAB_KEYS | INI::SAVE_SEMICOLON_KEYS;

        INI ini("config.ini", true);  // Assign ini file and parse

        ini.select("Core");

        RATIO = ini.getAs<float>("Core", "EyeAspectRatio", 1.f);
        HEADAIM = ini.getAs<bool>("Core", "AimWithHead", false);
        FOVOverride = ini.getAs<float>("Core", "FOVOverride", -1.f);
        ConvergeOverride = ini.getAs<int>("Core", "EyeConvergenceOverride", -1);

        success("Loaded Config.");

        // Prevent multi-triggering
        Sleep(100);
    }

    void saveConfig() {
        log("Saving Config");

        // Make the file
        std::ofstream Config("config.ini", std::ios_base::app);
        Config.close();

        // Set global parsing/saving options
        INI::PARSE_FLAGS = INI::PARSE_COMMENTS_ALL | INI::PARSE_COMMENTS_SLASH | INI::PARSE_COMMENTS_HASH;
        INI::SAVE_FLAGS = INI::SAVE_PRUNE | INI::SAVE_PADDING_SECTIONS | INI::SAVE_SPACE_SECTIONS | INI::SAVE_SPACE_KEYS | INI::SAVE_TAB_KEYS | INI::SAVE_SEMICOLON_KEYS;

        INI ini("config.ini", true);  // Assign ini file and parse

        ini.create("Core");

        // Does nothing for now


        ini.save("config.ini");
        success("Saved Config.");
    }

    // Converts quats to euler for the Aiming function
    Vec3 eulerFromQuat(Vec4 q)
    {
        Vec3 v;

        float test = q.x * q.y + q.z * q.w;
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
        float sqx = q.x * q.x;
        float sqy = q.y * q.y;
        float sqz = q.z * q.z;
        v.y = atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * sqy - 2 * sqz); // heading
        v.x = asin(2 * test); // attitude
        v.z = atan2(2 * q.x * q.w - 2 * q.y * q.z, 1 - 2 * sqx - 2 * sqz); // bank
        return v;
    }


    Vec4 quatFromEuler(Vec3 e)
    {
        auto [pitch, yaw, roll] = e;
        float qx = sin(roll / 2) * cos(pitch / 2) * cos(yaw / 2) - cos(roll / 2) * sin(pitch / 2) * sin(yaw / 2);
        float qy = cos(roll / 2) * sin(pitch / 2) * cos(yaw / 2) + sin(roll / 2) * cos(pitch / 2) * sin(yaw / 2);
        float qz = cos(roll / 2) * cos(pitch / 2) * sin(yaw / 2) - sin(roll / 2) * sin(pitch / 2) * cos(yaw / 2);
        float qw = cos(roll / 2) * cos(pitch / 2) * cos(yaw / 2) + sin(roll / 2) * sin(pitch / 2) * sin(yaw / 2);
        return Vec4(qx, qy, qz, qw);
    }
}