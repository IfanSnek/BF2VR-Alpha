// GameService.cpp - Code that interacts with the reverse 
// engineered game memory structure. It also implements hooks into some
// game functions.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include "GameService.h"
#include <Windows.h>
#include "Utils.h"
#include "OpenXRService.h"
#include "InputService.h"

namespace BF2VR {

    __int64 GameService::cameraUpdateDetour(CameraObject* a1, CameraObject* a2)
    {
        if (a2 == pRenderView && OpenXRService::isVRReady) {
            a2->cameraTransform = cameraTransfrom;
        }

        __int64 toReturn = cameraUpdateOriginal(a1, a2);

        // Update render settings
        GameRenderer::GetInstance()->renderView->aspectRatio = RATIO;

        // Fix TAA
        WorldRenderSettings* worldRenderSettings = WorldRenderSettings::GetInstance();
        if (isValidPtr(worldRenderSettings)) {
            if (worldRenderSettings->aaDisocclusionFactor != 1.f)
            {
                success("Disabled TAA");
                worldRenderSettings->aaDisocclusionFactor = 1.f;
            }
        }

        return toReturn;
    }

    // Function to set the view of the game camera
    void GameService::updateCamera(Vec3 hmdLocation, Matrix4 hmdMat, float yaw, float pitch) {
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

        pSettings->forceFov = FOV;

        // Set location from HMD
        hmdMat.o.x = hmdLocation.x;
        hmdMat.o.y = hmdLocation.y;
        hmdMat.o.z = hmdLocation.z;
        hmdMat.o.w = 1;

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
            hmdMat.o.y -= 3.5;

            // Update the transform that the CameraHook will use
            cameraTransfrom = hmdMat;
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

            Vec3 playerPosition = Vec3(0, 0, 0);
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

            hmdMat.o.x += playerPosition.x;
            hmdMat.o.y += playerPosition.y - heightOffset + 2.f;
            hmdMat.o.z += playerPosition.z;

            // Update the transform that the CameraHook will use
            cameraTransfrom = hmdMat;

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

            Alternator* viewAngleSwitch = aimer->alternator;
            if (!isValidPtr(viewAngleSwitch))
            {
                warn("Could not find address for either ViewAngle. If this shows up a lot, please report this to the dev. Try respawning to see if it temporarially fixes it.");
                return;

            } else if (isValidPtr(viewAngleSwitch->Primary))
            {
                for (int i = 0; i < 12; i++)
                {
                    if (viewAngleSwitch->Primary->Signature[i] == 0xFF)
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
                    viewAngleSwitch->Primary->Pitch = pitch;
                    viewAngleSwitch->Primary->Yaw = yaw - 3.14f;

                }
                else {
                    if (isValidPtr(viewAngleSwitch->Secondary))
                    {
                        matches = 0;

                        for (int i = 0; i < 12; i++)
                        {
                            if (viewAngleSwitch->Secondary->Signature[i] == 0xFF)
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
                            viewAngleSwitch->Secondary->Pitch = pitch;
                            viewAngleSwitch->Secondary->Yaw = yaw + 3.14f;
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


    __int64 GameService::poseUpdateDetour(int a1, int a2, int a3, int a4, __int64 a5)
    {
        __int64 toReturn = poseUpdateOriginal(a1, a2, a3, a4, a5);

        if (!isPosing)
        {
            isPosing = true;
            applyBones();
            isPosing = false;
        }

        return toReturn;
    }

    bool GameService::applyBones()
    {
        GameContext* CurrentContext = GameContext::GetInstance();
        if (!isValidPtr(CurrentContext)) {
            return false;
        }

        PlayerManager* playerManager = CurrentContext->playerManager;
        if (!isValidPtr(playerManager)) {
            return false;
        }

        ClientPlayer* player = playerManager->LocalPlayer;
        if (!isValidPtr(player)) {
            return false;
        }

        ClientSoldierEntity* soldier = player->controlledControllable;
        if (!isValidPtr(soldier)) {
            return false;
        }

        ClientBoneCollisionComponent* bones = GetClassFromName<ClientBoneCollisionComponent*>(soldier, "ClientBoneCollisionComponent");

        if (!isValidPtr(bones)) {
            warn("Could not find bones. 6dof hands will not work.");
            return false;
        }

        BasicSkeleton skeleton = BasicSkeleton(bones);
        if (!skeleton.valid)
        {
            warn("Could not find the skeleton. 6dof hands will not work.");
            return false;
        }


        for (boneState state : boneStates)
        {
            if (!skeleton.poseBone(state.boneName, state.location, state.rotation, state.scale))
            {
                warn(std::format("Bone {} could not be posed.", state.boneName));
            }
        }

        return true;
    }


    void GameService::updateBone(const char* boneName, Vec3 location, Vec4 rotation)
    {
        // Check for existing entry
        for (int i = 0; i < boneStates.size(); i++)
        { 
            boneState state = boneStates.at(i);
            if (strcmp(state.boneName, boneName) == 0)
            {
                state.location = location;
                state.rotation = rotation;

                boneStates.at(i) = state;

                isPosing = false;
                return;
            }
        }

        boneStates.push_back({ boneName, location, rotation, Vec3(1, 1, 1) });

        isPosing = false;
    }

    bool GameService::enableHooks() {

        pRenderView = GameRenderer::GetInstance()->renderView;
        if (!isValidPtr(pRenderView))
        {
            error("Unable to hook camera, Renderview is invalid.");
            return false;
        }

        MH_STATUS mh = MH_CreateHook((LPVOID)OFFSETCAMERA, (LPVOID)&cameraUpdateDetour, reinterpret_cast<LPVOID*>(&cameraUpdateOriginal));
        if (mh != MH_OK && mh != 9) {
            error("Error hooking BF2 UpdateCamera. Error: " + std::to_string(mh));
            return false;
        }
        else if (mh == 9) {
            error("Error hooking BF2 UpdateCamera. Try launching the mod again (you can leave the game open). If this doesn't stop, try respawning first.");
            return false;
        }

        mh = MH_EnableHook((LPVOID)OFFSETCAMERA);
        if (mh != MH_OK) {
            error("Error enabling BF2 UpdateCamera hook. Error: " + std::to_string(mh));
            return false;
        }

        mh = MH_CreateHook((LPVOID)OFFSETPOSE, (LPVOID)&poseUpdateDetour, reinterpret_cast<LPVOID*>(&poseUpdateOriginal));
        if (mh != MH_OK && mh != 9) {
            error("Error hooking BF2 UpdatePose. Error: " + std::to_string(mh));
            return false;
        }
        else if (mh == 9) {
            error("Error hooking BF2 UpdatePose. Try launching the mod again (you can leave the game open). If this doesn't stop, try respawning first.");
            return false;
        }

        mh = MH_EnableHook((LPVOID)OFFSETPOSE);
        if (mh != MH_OK) {
            error("Error enabling BF2 UpdatePose hook. Error: " + std::to_string(mh));
            return false;
        }

        return true;
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