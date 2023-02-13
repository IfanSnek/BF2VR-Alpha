#include "Utils.h"
#include "GameService.h"
#include <string>
#include "DirectXService.h"
#include "OpenXRService.h"
#include "InputService.h"

namespace BF2VR
{
    // Print to console and a file
    void log(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        std::cout << message << std::endl;
        logFile << message << std::endl;
    }
    void error(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED);
        std::cout << message << std::endl;
        logFile << message << std::endl;
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

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_GREEN);
        std::cout << message << std::endl;
        logFile << message << std::endl;

        lastWarn = message;
    }
    void success(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_GREEN);
        std::cout << message << std::endl;
        logFile << message << std::endl;
    }
    void info(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::cout << message << std::endl;
        logFile << message << std::endl;
    }
    void deb(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_BLUE);
        std::cout << message << std::endl;
        logFile << message << std::endl;
    }

    // Shutdown and eject the mod. Currently only works before DirextX gets hooked.
    void shutdown() {
        OpenXRService::isVRReady = false;
        DirectXService::shouldPresent = false;

        Sleep(100); // Let a few frames render for anyncronous present

        log("Unhooking Camera");
        MH_DisableHook((LPVOID)OFFSETCAMERA);
        MH_RemoveHook((LPVOID)OFFSETCAMERA);

        log("Unhooking Pose");
        MH_DisableHook((LPVOID)OFFSETPOSE);
        MH_RemoveHook((LPVOID)OFFSETPOSE);

        log("Ending VR");
        OpenXRService::endXR();
        Sleep(100);

        log("Unhooking DirectX");
        DirectXService::unhookDirectX();

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

        // Open config.txt and read it line by line, applying the values

        std::ifstream Config("config.txt");

        if (!Config)
        {
            info("##############################");
            info("IMPORTANT NOTICE:");
            info("No valid configuration has been found. The game will autoconfigure, but then you will need to restart the game. Please read the below logs:");
            info("##############################");

            doReconfig = true;;
            return;
        }

        std::string text;
        std::string MultiLineProperty = "";

        while (getline(Config, text)) {

            if (MultiLineProperty != "")
            {
                // Continue where we left off

                if (MultiLineProperty == "EYERATIO")
                {
                    RATIO = std::stof(text);
                }

                //std::cout << MultiLineProperty << "=" << text << std::endl;
                MultiLineProperty = "";
            }
            // Boolean parameters
            else if (text == "NOFOV")
            {
                NOFOV = true;
                info("Manual fov enabled.");
            }
            else if (text == "NOHAND")
            {
                HEADAIM = true;
                info("Head aim enabled.");
            }
            // Value parameters
            else
            {
                MultiLineProperty = text;
            }
        }
        Config.close();

        success("Loaded Config.");

        // Prevent multi-triggering
        Sleep(100);
    }

    void saveConfig() {

        static std::ofstream Config("config.txt", std::ios_base::app);

        Config << "EYERATIO" << std::endl;
        Config << std::to_string(RATIO) << std::endl;

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
        v.x = atan2(2 * q.y * q.w - 2 * q.x * q.z, 1 - 2 * sqy - 2 * sqz); // heading
        v.y = asin(2 * test); // attitude
        v.z = atan2(2 * q.x * q.w - 2 * q.y * q.z, 1 - 2 * sqx - 2 * sqz); // bank
        return v;
    }


    Vec4 quatFromEuler(Vec3 e)
    {
        auto [yaw, pitch, roll] = e;
        float qx = sin(roll / 2) * cos(pitch / 2) * cos(yaw / 2) - cos(roll / 2) * sin(pitch / 2) * sin(yaw / 2);
        float qy = cos(roll / 2) * sin(pitch / 2) * cos(yaw / 2) + sin(roll / 2) * cos(pitch / 2) * sin(yaw / 2);
        float qz = cos(roll / 2) * cos(pitch / 2) * sin(yaw / 2) - sin(roll / 2) * sin(pitch / 2) * cos(yaw / 2);
        float qw = cos(roll / 2) * cos(pitch / 2) * cos(yaw / 2) + sin(roll / 2) * sin(pitch / 2) * sin(yaw / 2);
        return Vec4(qx, qy, qz, qw);
    }
}