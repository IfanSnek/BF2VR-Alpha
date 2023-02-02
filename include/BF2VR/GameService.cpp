#include "GameService.h"
#include <Windows.h>
#include "Utils.h"
#include "OpenXRService.h"

namespace BF2VR {

    bool GameService::HookCamera() {

        RenderView = GameRenderer::GetInstance()->renderView;
        if (!IsValidPtr(RenderView))
        {
            log("Unable to hook camera, Renderview is invalid.");
            return false;
        }

        MH_STATUS mh = MH_CreateHook(reinterpret_cast<LPVOID>(OffsetCamera), reinterpret_cast<LPVOID>(&UpdateDetour), reinterpret_cast<LPVOID*>(&UpdateOriginal));
        if (mh != MH_OK && mh != 9) {
            log("Error hooking BF2 UpdateCamera: " + std::to_string(mh));
            return false;
        }
        else if (mh == 9) {
            log("Error hooking BF2 UpdateCamera. Try launching the mod again (you can leave the game open). If this doesn't stop, try respawning first.");
            return false;
        }

        mh = MH_EnableHook(reinterpret_cast<LPVOID>(OffsetCamera));
        if (mh != MH_OK) {
            log("Error enabling BF2 UpdateCamera hook: " + std::to_string(mh));
            return false;
        }

        return true;
    }

    __int64 GameService::UpdateDetour(CameraObject* a1, CameraObject* a2)
    {
        if (a2 == RenderView && OpenXRService::VRReady) {
            OpenXRService::UpdatePoses();
            a2->cameraTransform = Transform;
        }
        return UpdateOriginal(a1, a2);
    }

    // Function to set the view of the game camera
    void GameService::UpdateCamera(Vec3 location, Matrix4 HmdRot, float yaw, float pitch) {
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
        pSettings->forceFov = FOV;

        HmdRot.invert();

        // Convert back to Matrix4x4
        Matrix4x4 out;
        out.x.x = HmdRot[0]; out.x.y = HmdRot[1]; out.x.z = HmdRot[2]; out.x.w = HmdRot[3];
        out.y.x = HmdRot[4]; out.y.y = HmdRot[5]; out.y.z = HmdRot[6]; out.y.w = HmdRot[7];
        out.z.x = HmdRot[8]; out.z.y = HmdRot[9]; out.z.z = HmdRot[10]; out.z.w = HmdRot[11];

        // Set location from HMD
        out.o.x = location.x;
        out.o.y = location.y;
        out.o.z = location.z;
        out.o.w = 1;

        // Get some game members, validating pointers along the way
        GameContext* CurrentContext = GameContext::GetInstance();
        if (!IsValidPtr(CurrentContext)) {
            return;
        }

        ClientLevel* CurrentLevel = CurrentContext->level;
        if (!IsValidPtr(CurrentLevel)) {
            return;
        }

        char* levelname = CurrentLevel->LevelName;
        if (!IsValidPtr(levelname)) {
            return;
        }

        if (levelname != Level)
        {
            // Check for when the level name changes
            std::string lvl = levelname;
            log("Switched to " + lvl);
            Level = levelname;
        }

        if (strcmp(levelname, "Levels/FrontEnd/FrontEnd") == 0)
        {
            out.o.y -= 3.5;
        }
        else
        {
            // Get more members, again, checking along the way
            PlayerManager* playerManager = CurrentContext->playerManager;
            if (!IsValidPtr(playerManager)) {
                return;
            }

            ClientPlayer* player = playerManager->LocalPlayer;
            if (!IsValidPtr(player)) {
                return;
            }

            ClientSoldierEntity* soldier = player->controlledControllable;
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
            out.o.y += playerPosition.y - heightOffset + 3;
            out.o.z += playerPosition.z;

            // Update the view angles
            // Check wich ViewAngle pointer is active. The signature of the active viewangle will start with 12 0xff bytes

            int matches = 0;

            LocalAimer* aimer = LocalAimer::GetInstance();
            if (!IsValidPtr(aimer))
            {
                log("Could not find address for LocalAimer. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                Transform = out;
                return;

            }

            Alternator* alternator = aimer->alternator;
            if (!IsValidPtr(alternator))
            {
                log("Could not find address for Alternator. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                Transform = out;
                return;

            }
            if (IsValidPtr(alternator->Primary))
            {
                for (int i = 0; i < 12; i++)
                {
                    if (alternator->Primary->Signature[i] == 0xFF)
                    {
                        matches++;
                    }
                    else {
                        break;
                    }
                }
                if (matches == 12)
                {
                    // Primary is active
                    alternator->Primary->Pitch = pitch;
                    alternator->Primary->Yaw = yaw - 3.14;
                    Transform = out;
                    return;

                }
                else {
                    if (IsValidPtr(alternator->Secondary))
                    {
                        matches = 0;

                        for (int i = 0; i < 12; i++)
                        {
                            if (alternator->Secondary->Signature[i] == 0xFF)
                            {
                                matches++;
                            }
                            else {
                                break;
                            }
                        }
                        if (matches == 12)
                        {
                            // Secondary is active
                            alternator->Secondary->Pitch = pitch;
                            alternator->Secondary->Yaw = yaw + 3.14;
                        }
                        else {
                            // Uh oh, none are active.
                            log("Could not find address for the Secondary viewangle. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                        }

                    }
                    else {
                        log("Could not find address for the Secondary angle. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                    }
                }
            }

        }

        // Update the transform that the CameraHook will use
        Transform = out;

    }


    void GameService::SetMenu(bool enabled) {
        UISettings* pUISettings = UISettings::GetInstance();
        if (!IsValidPtr(pUISettings))
        {
            log("UI pointer invalid");
            return;
        }
        pUISettings->drawEnable = enabled;
    }
}