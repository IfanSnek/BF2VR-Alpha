#include "GameService.h"
#include <Windows.h>
#include "Utils.h"
#include "OpenXRService.h"
#include "InputService.h"
#include "BFSDK.h"

namespace BF2VR {

    bool GameService::hookCamera() {

        pRenderView = GameRenderer::GetInstance()->renderView;
        if (!isValidPtr(pRenderView))
        {
            error("Unable to hook camera, Renderview is invalid.");
            return false;
        }

        MH_STATUS mh = MH_CreateHook(reinterpret_cast<LPVOID>(OFFSETCAMERA), reinterpret_cast<LPVOID>(&cameraUpdateDetour), reinterpret_cast<LPVOID*>(&updateOriginal));
        if (mh != MH_OK && mh != 9) {
            error("Error hooking BF2 UpdateCamera. Error: " + std::to_string(mh));
            return false;
        }
        else if (mh == 9) {
            error("Error hooking BF2 UpdateCamera. Try launching the mod again (you can leave the game open). If this doesn't stop, try respawning first.");
            return false;
        }

        mh = MH_EnableHook(reinterpret_cast<LPVOID>(OFFSETCAMERA));
        if (mh != MH_OK) {
            error("Error enabling BF2 UpdateCamera hook. Error: " + std::to_string(mh));
            return false;
        }

        return true;
    }

    __int64 GameService::cameraUpdateDetour(CameraObject* a1, CameraObject* a2)
    {
        if (a2 == pRenderView && OpenXRService::isVRReady) {
            OpenXRService::updatePoses();
            a2->cameraTransform = cameraTransfrom;
        }


        return updateOriginal(a1, a2);
    }

    // Function to set the view of the game camera
    void GameService::updateCamera(Vec3 hmdLocation, Matrix4 hmdRot, float yaw, float pitch) {
        GameRenderer* pGameRenderer = GameRenderer::GetInstance();
        if (!isValidPtr(pGameRenderer))
        {
            return;
        }
        GameRenderSettings* pSettings = pGameRenderer->gameRenderSettings;
        if (!isValidPtr(pSettings))
        {
            return;
        }

        if (!NOFOV)
        {
            pSettings->forceFov = FOV;
        }

        hmdRot.invert();

        // Convert back to Matrix4x4
        Matrix4x4 outputHeadMatrix;
        outputHeadMatrix.x.x = hmdRot[0]; outputHeadMatrix.x.y = hmdRot[1]; outputHeadMatrix.x.z = hmdRot[2]; outputHeadMatrix.x.w = hmdRot[3];
        outputHeadMatrix.y.x = hmdRot[4]; outputHeadMatrix.y.y = hmdRot[5]; outputHeadMatrix.y.z = hmdRot[6]; outputHeadMatrix.y.w = hmdRot[7];
        outputHeadMatrix.z.x = hmdRot[8]; outputHeadMatrix.z.y = hmdRot[9]; outputHeadMatrix.z.z = hmdRot[10]; outputHeadMatrix.z.w = hmdRot[11];

        // Set location from HMD
        outputHeadMatrix.o.x = hmdLocation.x;
        outputHeadMatrix.o.y = hmdLocation.y;
        outputHeadMatrix.o.z = hmdLocation.z;
        outputHeadMatrix.o.w = 1;

        // Get some game members, validating pointers along the way
        GameContext* CurrentContext = GameContext::GetInstance();
        if (!isValidPtr(CurrentContext)) {
            return;
        }

        ClientLevel* CurrentLevel = CurrentContext->level;
        if (!isValidPtr(CurrentLevel)) {
            return;
        }

        char* levelname = CurrentLevel->LevelName;
        if (!isValidPtr(levelname)) {
            return;
        }

        if (levelname != level)
        {
            // Check for when the level name changes
            std::string lvl = levelname;
            info("Switched to " + lvl);
            level = levelname;
        }

        if (strcmp(levelname, "Levels/FrontEnd/FrontEnd") == 0)
        {
            outputHeadMatrix.o.y -= 3.5;

            // Update the transform that the CameraHook will use
            cameraTransfrom = outputHeadMatrix;
        }
        else
        {
            // Get more members, again, checking along the way
            PlayerManager* playerManager = CurrentContext->playerManager;
            if (!isValidPtr(playerManager)) {
                return;
            }

            ClientPlayer* player = playerManager->LocalPlayer;
            if (!isValidPtr(player)) {
                return;
            }

            Vec3 playerPosition = {0, 0, 0};
            float heightOffset = 0;
            bool inVehicle = false;

            AttachedControllable* vehicle = player->attachedControllable;

            if (isValidPtr(vehicle))
            {
                playerPosition = vehicle->GetVehicleLocation();

                if (playerPosition.x == 0 && playerPosition.y == 0 && playerPosition.z == 0)
                {
                    warn("Vehicle could not be located.");
                }

                heightOffset = .8f;
                inVehicle = true;
            }
            else {
                inVehicle = false;

                ClientSoldierEntity* soldier = player->controlledControllable;
                if (!isValidPtr(soldier)) {
                    InputService::useRight = inVehicle;
                    return;
                }

                ClientSoldierPrediction* prediction = soldier->clientSoldierPrediction;
                if (!isValidPtr(prediction)) {
                    InputService::useRight = inVehicle;
                    return;

                }
                playerPosition = prediction->Location;
                heightOffset = soldier->HeightOffset;
            }

            InputService::useRight = inVehicle;

            outputHeadMatrix.o.x += playerPosition.x;
            outputHeadMatrix.o.y += playerPosition.y - heightOffset + 3;
            outputHeadMatrix.o.z += playerPosition.z;

            // Update the transform that the CameraHook will use
            cameraTransfrom = outputHeadMatrix;

            // Update the view angles
            // Check wich ViewAngle pointer is active. The signature of the active viewangle will start with 12 0xff bytes

            if (inVehicle)
            {
                return;
            }

            int matches = 0;

            LocalAimer* aimer = LocalAimer::GetInstance();
            if (!isValidPtr(aimer))
            {
                warn("Could not find address for LocalAimer. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                return;

            }

            Alternator* alternator = aimer->alternator;
            if (!isValidPtr(alternator))
            {
                warn("Could not find address for Alternator. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                return;

            } else if (isValidPtr(alternator->Primary))
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
                    alternator->Primary->Yaw = yaw - 3.14f;

                }
                else {
                    if (isValidPtr(alternator->Secondary))
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
                            alternator->Secondary->Yaw = yaw + 3.14f;
                        }
                        else {
                            // Uh oh, none are active.
                            warn("Could not find address for the Secondary viewangle. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                        }

                    }
                    else {
                        warn("Could not find address for the Secondary angle. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                    }
                }
            }
            return;
        }

    }

    void GameService::updateBone(const char* boneName, Vec3 location, Vec4 rotation)
    {
        GameContext* CurrentContext = GameContext::GetInstance();
        if (!isValidPtr(CurrentContext)) {
            return;
        }
        
        PlayerManager* playerManager = CurrentContext->playerManager;
        if (!isValidPtr(playerManager)) {
            return;
        }

        ClientPlayer* player = playerManager->LocalPlayer;
        if (!isValidPtr(player)) {
            return;
        }
        
        ClientSoldierEntity* soldier = player->controlledControllable;
        if (!isValidPtr(soldier)) {
            return;
        }

        ClientSoldierPrediction* prediction = soldier->clientSoldierPrediction;
        if (!isValidPtr(prediction)) {
            return;

        }
        ClientBoneCollisionComponent* bones = GetClassFromName<ClientBoneCollisionComponent*>(soldier, "ClientBoneCollisionComponent");
        if (bones == nullptr)
        {
            bones = GetClassFromName<ClientBoneCollisionComponent*>(soldier, "ClientBoneCollisionComponent", 0x2000, true);
        }
        if (!isValidPtr(bones)) {
            warn("Could not find bones. 6dof hands will not work.");
            return;
        }

        // Find bone index

        AnimationSkeleton* skeleton = bones->skeleton;
        if (!isValidPtr(bones)) {
            warn("Could not find animation skeleton. 6dof hands will not work.");
            return;
        }

        SkeletonAsset* asset = skeleton->asset;
        if (!isValidPtr(bones)) {
            warn("Could not find skeleton asset. 6dof hands will not work.");
            return;
        }

        int boneIndex = -1;

        for (int i = 0; i < skeleton->boneCount; i++) {
            if (strcmp(asset->boneNames[i], boneName) == 0)
            {
                boneIndex = i;
            }
        }

        if (boneIndex == -1)
        {
            warn("Bone not found by name. 6dof hands will not work.");
            return;
        }

        // Set vector
        UpdatePoseResultData pose = bones->pose;
        QuatTransformArray* transforms = pose.transforms;
        if (!isValidPtr(bones)) {
            warn("Could not find bone transform array. 6dof hands will not work.");
            return;
        }

        location.x += prediction->Location.x;
        location.y += prediction->Location.y;
        location.z += prediction->Location.z;

        transforms->transform[boneIndex].Translation .x = location.x;
        transforms->transform[boneIndex].Translation.y = location.y;
        transforms->transform[boneIndex].Translation.z = location.z;

        transforms->transform[boneIndex].Quat = rotation;


    }

    void GameService::setUIDrawState(bool enabled) {
        UISettings* pUISettings = UISettings::GetInstance();
        if (!isValidPtr(pUISettings))
        {
            warn("UI pointer invalid. Cannot toggle.");
            return;
        }
        pUISettings->drawEnable = enabled;
    }
}