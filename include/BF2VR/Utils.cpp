#include "Utils.h"
#include "GameService.h"
#include <string>
#include "DirectXService.h"
#include "OpenXRService.h"
#include "InputService.h"
#include "HookHelper.h"

namespace BF2VR
{
    // Print to console and a file
    void log(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        std::cout << message << std::endl;
        Log << message << std::endl;
    }
    void error(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        time_t my_time = time(NULL);
        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED);
        std::cout << message << std::endl;
        Log << message << std::endl;
    }
    void warn(std::string message) {

        if (warnCount == -1)
        {
            return;
        }

        if (warnCount > 20)
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
        Log << message << std::endl;

        lastWarn = message;
    }
    void success(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_GREEN);
        std::cout << message << std::endl;
        Log << message << std::endl;
    }
    void info(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        std::cout << message << std::endl;
        Log << message << std::endl;
    }
    void deb(std::string message) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);

        std::cout << "[BF2VR]   ";

        SetConsoleTextAttribute(hConsole,
            FOREGROUND_RED | FOREGROUND_BLUE);
        std::cout << message << std::endl;
        Log << message << std::endl;
    }

    // Check if a pointer is legit
    bool IsValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr &&!IsBadReadPtr(p, sizeof(PVOID)); }

    // Shutdown and eject the mod. Currently only works before DirextX gets hooked.
    void Shutdown() {
        OpenXRService::VRReady = false;
        DirectXService::DoPresent = false;

        Sleep(100); // Let a few frames render for anyncronous present

        log("Unhooking Camera");
        HookHelper::DestroyHook(OffsetCamera);

        log("Ending VR");
        OpenXRService::EndXR();
        Sleep(100);

        log("Unhooking DirectX");
        DirectXService::UnhookDirectX();

        log("Disconnecting ViGEm");
        InputService::Disconnect();

        ShutdownNoHooks();
    }

    // Shutdown without removing hooks. This is for early initialization when nothing has been hooked yet.
    void ShutdownNoHooks() {
        log("Uninitializing Minhook");
        MH_Uninitialize();

        info("Mod shut down.");

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
        std::cout << "You may now close this window." << std::endl;
        Log << "End of log." << std::endl;

        FreeConsole();
        FreeLibraryAndExitThread(OwnModule, 0);
    }


    void LoadConfig() {

        // Open config.txt and read it line by line, applying the values

        std::ifstream Config("config.txt");

        if (!Config)
        {
            info("##############################");
            info("IMPORTANT NOTICE:");
            info("No valid configuration has been found. The game will autoconfigure, but then you will need to restart the game. Please read the below logs:");
            info("##############################");

            Reconfig = true;;
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

    void SaveConfig() {

        static std::ofstream Config("config.txt", std::ios_base::app);

        Config << "EYERATIO" << std::endl;
        Config << std::to_string(RATIO) << std::endl;

        success("Saved Config.");
    }

    // Converts quats to euler for the Aiming function
    Vec3 EulerFromQuat(Vec4 q)
    {
        Vec3 v;

        double test = q.x * q.y + q.z * q.w;
        if (test > 0.499)
        { // singularity at north pole
            v.x = 2 * atan2(q.x, q.w); // heading
            v.y = pi / 2; // attitude
            v.z = 0; // bank
            return v;
        }
        if (test < -0.499)
        { // singularity at south pole
            v.x = -2 * atan2(q.x, q.w); // headingq
            v.y = -pi / 2; // attitude
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
}