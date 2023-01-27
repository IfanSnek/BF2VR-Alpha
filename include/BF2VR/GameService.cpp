#include "GameService.h"
#include <Windows.h>
#include "Utils.h"
#include "OpenXRService.h"

namespace BF2VR {
    bool GameService::CaptureOrigin() {

        // Get a reference to the game's render view. This only updates the camera, and not the HUD or aim.
        RenderView = GameRenderer::GetInstance()->renderView;
        if (!IsValidPtr(RenderView))
        {
            log("Unable to capture origin, Renderview is invalid.");
            return false;
        }

        Origin = GameRenderer::GetInstance()->renderView->transform;
        log("Origin captured.");

        return true;
    }
    
    bool GameService::HookCamera() {

        CaptureOrigin();

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
        if (a2 == RenderView && UpdateLook && OpenXRService::VRReady) {
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

        Matrix4 specialOpsMatrix;
        Matrix4x4 c = Origin;

        // Matrix for the origin
        specialOpsMatrix.set(
            c.x.x, c.x.y, c.x.z, c.x.w,
            c.y.x, c.y.y, c.y.z, c.y.w,
            c.z.x, c.z.y, c.z.z, c.z.w,
            c.o.x, c.o.y, c.o.z, c.o.w
        );

        HmdRot.invert();
        specialOpsMatrix = specialOpsMatrix * HmdRot;

        // Convert back to Matrix4x4
        Matrix4x4 out;
        out.x.x = specialOpsMatrix[0]; out.x.y = specialOpsMatrix[1]; out.x.z = specialOpsMatrix[2]; out.x.w = specialOpsMatrix[3];
        out.y.x = specialOpsMatrix[4]; out.y.y = specialOpsMatrix[5]; out.y.z = specialOpsMatrix[6]; out.y.w = specialOpsMatrix[7];
        out.z.x = specialOpsMatrix[8]; out.z.y = specialOpsMatrix[9]; out.z.z = specialOpsMatrix[10]; out.z.w = specialOpsMatrix[11];

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

            // Set the gun's aim
            if (UpdateAim)
            {
                p2->pitch = pitch + AimPitchOffset;
                p2->yaw = yaw + AimYawOffset;
            }
            AimPitch = pitch;
            AimYaw = yaw;
        }

        // Update the transform that the CameraHook will use
        Transform = out;

    }

    // Reorient the VR
    void GameService::Reposition() {
        float oldPitch = AimPitch;
        float oldYaw = AimYaw;

        UpdateLook = false;
        UpdateAim = false;

        Sleep(100);

        // Capture the origins
        CaptureOrigin();
        AimPitchOffset = AimPitch - oldPitch;
        AimYawOffset = AimYaw - oldYaw;

        Sleep(100);

        UpdateLook = true;
        UpdateAim = true;

        log("Repositioned");
    }
}