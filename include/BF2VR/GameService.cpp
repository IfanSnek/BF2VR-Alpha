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
        if (mh != MH_OK) {
            log("Error hooking BF2 UpdateCamera: " + std::to_string(mh));
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

        ClientLevel* CurrentLevel = CurrentContext->GetClientLevel();
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
            out.o.y += playerPosition.y - heightOffset + 3.5;
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

            p2->pitch = pitch;
            p2->yaw = yaw + 135;
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