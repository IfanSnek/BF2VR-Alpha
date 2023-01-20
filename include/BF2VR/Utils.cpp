#include "Utils.h"
#include "GameService.h"
#include <string>
#include "DirectXService.h"
#include "OpenXRService.h"
#include "HookHelper.h"

namespace BF2VR
{
    // Print to console and a file
    void log(std::string message) {
        std::cout << message << std::endl;
        Log << message << std::endl;
    }

    // Check if a pointer is legit
    bool IsValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr; }

    // Shutdown and eject the mod. Currently only works before DirextX gets hooked.
    void Shutdown() {
        log("Unhooking Camera");
        HookHelper::DestroyHook(OffsetCamera);

        log("Ending VR");
        OpenXRService::EndXR();

        OpenXRService::VRReady = false;
        DirectXService::DoPresent = false;
        Sleep(100); // Let a few frames render for anyncronous present

        log("Unhooking DirectX");
        DirectXService::UnhookDirectX();

        ShutdownNoHooks();
    }

    // Shutdown without removing hooks. This is for early initialization when nothing has been hooked yet.
    void ShutdownNoHooks() {
        log("Uninitializing Minhook");
        MH_Uninitialize();

        std::cout << "You may now close this window." << std::endl;
        Log << "End of log." << std::endl;
        FreeConsole();
        FreeLibraryAndExitThread(OwnModule, 0);
    }


    void LoadConfig() {

        return;

        // Open config.txt and read it line by line, applying the values

        std::string text;
        std::string MultiLineProperty = "";

        std::ifstream Config("config.txt");
        while (getline(Config, text)) {

            if (MultiLineProperty != "")
            {
                // Continue where we left off

                if (MultiLineProperty == "IPDGAMEUNITS")
                {
                    IPD = std::stof(text);
                }
                else if (MultiLineProperty == "EYERATIO")
                {
                    RATIO = std::stof(text);
                }
                else if (MultiLineProperty == "MAPPINGLEFTMIN")
                {
                    leftmin = std::stof(text);
                }
                else if (MultiLineProperty == "MAPPINGLEFTMAX")
                {
                    leftmax = std::stof(text);
                }
                else if (MultiLineProperty == "MAPPINGRIGHTMIN")
                {
                    rightmin = std::stof(text);
                }
                else if (MultiLineProperty == "MAPPINGRIGHTMAX")
                {
                    rightmax = std::stof(text);
                }
                else if (MultiLineProperty == "VRHEIGHT")
                {
                    VRHeight = std::stof(text);
                }
                else if (MultiLineProperty == "FOV")
                {
                    FOV = std::stof(text);
                }

                //std::cout << MultiLineProperty << "=" << text << std::endl;
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

        log("Loaded Config.");

        // Prevent multi-triggering
        Sleep(100);
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