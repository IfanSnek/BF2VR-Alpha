# BF2VR
### VR Mod for Star Wars: Battlefront II

[![](https://img.shields.io/badge/Discord-Releases%20-blueviolet)](https://discord.gg/mrKYwzd3N4)

# About
Battlefront II VR (BF2VR) is a project by developer, musician, and Star Wars enthusiast [Ethan Porcaro](https://ethanporcaro.com/). He has played Battlefront II for a while, and after being inspired by [praydog](https://github.com/praydog)'s upcoming Unreal Engine VR Injector, decided to see what would be possible to mod in the Frostbite Engine.

BF2VR started with code from [OpenGameCamera](https://github.com/coltonon/OpenGameCamera) to position the Battlefront Camera to the transformation matrix from the OpenVR API. The game's DirectX calls were intercepted to get access to the framebuffer, which was then sent to the VR screen. Stereoscopy was provided using Alternate Eye Rendering, a technique in which the rendered eye alternates every frame while the game camera shifts left and right to give a sense of depth. The games "Soldier" was also hooked to aim where the player looked.

New updates led the mod to be ported to [OpenXR](https://www.khronos.org/openxr/), a standard for XR runtime interaction. This improves performance and compatibility. Motion controlls were added, allowing the player to aim with a controller rather that the headset. A menu mode was added to allow the player to interact with game menus without having to remove the headset.

BF2VR is in early alpha, with frequent updates but limited features. Releases to the mod can be found at Ethan's [Discord Server](https://discord.gg/mrKYwzd3N4). Keep in mind that the mod is unstable, and you won't get all the features a VR mod should include just yet.

# Using the mod
## Installation
After downloading the latest zip file from the [releases channel](https://discord.com/channels/1046270181313351770/1047182227454300210), you will need to extract the files to some location on your drive. Some of the files will need to be moved to the Battlefront instalaltion folder. Your file layout should look like this:

    STAR WARS Battlefront II/
	 |      ...
    ├─ starwarsbattlefrontii.exe
    ├─ openxr_loader.dll
     |     ...
	
    AlphaXYZ/
    ├─ Launcher.exe
    ├─ BF2VR.dll
Note: Your antivirus may flag my mod, this is only because the injection technique is commonly used by malware to take over certain apps. In this case I only use it to take over the game, just to enable VR support. As long as you only downloaded the mod from my Discord server, you will be fine. You may have to add the AlphaXYZ folder to your antivirus exclusion list.

## First time setup
First launch Battlefront from whatever launcher you have (pirated copies won't work). You will then need to start your OpenXR runtime (Oculus, SteamVR, etc.). Once the game is loaded to the main menu and you can see your menu area on your VR headset, you may open Launcher.exe. You willl see a window pop up quickly, then a new logging window will appear on the game window. The game will resize to a square (for VR rendering), and the logging window will quickly display the startup checks. 

### Errors

If you see an error repeating over and over, please close the game and look in the game's installation folder for a file called `logs.txt`. You should send this file to me on my Discord server. If it is too large to send, then delete the repeating error but leave the first line of it. Here are some common errors:

* Failed to create OpenXR xrInstance

This means the headset is not attached, the XR runtime is not active, or some part of your VR wasn't set up yet. If this is not the case, please send me your logs. The number in the message corresponds with [these problems](https://registry.khronos.org/OpenXR/specs/0.90/man/html/XrResult.html).

* Error hooking BF2 UpdateCamera: 9

This means the game camera couldn't be hooked. Please try restarting the game and mod.

* Failed to render a frame

This just means a frame was skipped. If it only happens a bit, it is fine. This should not be happening every frame.

### Auto configuration

If you get no errors on the first setup, you may see a message in the log telling you about automatic configuration. Once you see the message `Saved the new config! You may restart the game now.`, you may restart the game and mod. The configuration will be saved in the installation directory of the game. Unless instructed otherwise, you may keep that file there after an update to avoid having to reconfigure.

## Playing the game
All modes work in the game, with the following limitations:
* Vehicles or turrets do not work yet. This will be a problem if you want to play campaign.
*  **DO NOT** play multiplayer, as you risk **getting banned**. Please use [3rd party Kyber Servers](http://kyber.gg/) instead.
* Characters that do not support 1st person work, but a little bit weirdly.

Because the mod is still being worked on, you may experience random errors and crashes. Please report these.

### Hotkeys
* END: Attempts to eject the mod while keeping the game open. This often will crash
* HOME: This will recenter the view. Press this if the perspective is wrong or movment is backwards.

# Contributing
This code is open source. Please read the licence if you want to fork. Contributions are welcome.
## Code structure
The code is split into the launcher and the mod DLL. The laucher is a single simple file. This explanation is for the mod DLL.

The different parts of the mod each have their own class in the BF2VR namespace. These "services" are:

### DirectXService
####  DirectXService::PresentDetour
This has the detour for the DirectX present function. It will call `OpenXRService::BeginFrameAndGetVectors` and ` OpenXRService::UpdateActions` on the first of every other frame (when the left eye is rendering). It will then call `OpenXRService::SubmitFrame` passing the framebuffer, and ont he second of evey other frame (right eye), it will call `OpenXRService::EndFrame`. The eyes are then switched.

#### DirectXService::HookDirectX
This created a dummy D3D11 device and attaches it to the game's device. It then hooks present which starts the `DirectXService::PresentDetour` documented above.

#### DirectXService::UnhookDirectX
This unhooks `DirectXService::PresentDetour` and releases all resources.

#### DirectXService::RenderOverlays
This draws overlays using the [DirectXTK](https://github.com/microsoft/DirectXTK) library. Examples include the crosshair.

#### DirectXService::RenderXRFrame
This draws the passed texture onto the passed render target view. The rtv is for OpenXR's swapchain. This will also call `DirectXService::RenderOverlays` at one point.

### GameService

#### GameService::CaptureOrigin
This sets the global variable `Origin` to the current camera transform.

#### GameService::HookCamera()
This will first Capture the origin then create a hook for the game's camera update, detouring it to `GameService::UpdateDetour`

#### GameService::UpdateDetour
This will check if the current camera is the view camera, and if so, set the view camera's transform to the global variable `Transform`. It will then call the original function.

#### GameService::UpdateCamera
This will calculate the view from the provided parameters, and apply it to the global variable `Transform`. It also sets the FOV, and log level changes. If the player is in a match, it uses the soldier's position to help set `Transform`. It also sets the player's aim from the provided `pitch` and `yaw`.

#### GameService::Reposition
This will make the camera and solider's aim return to their original unmodded positions. It captures the positions and sets some global offset variables to the difference. It then reenables the aim and camera.

### OpenXRService

#### OpenXRService::CreateXRInstanceWithExtensions
This will initialize OpenXR and enable the extensions the mod needs.

#### OpenXRService::BeginXRSession
This will create the `xrSession`, reference space, and swapchains. It will call `PrepareActions` and set `VRReady` to true, enabling a lot of other functions like the present hook.

#### OpenXRService::PrepareActions()
This will register and enable OpenXR inputs the mod will need, such as the hand poses, trigger values, etc.

#### OpenXRService::BeginFrameAndGetVectors
This will call `xrBeginFrame` which tells the runtime that the frame is going to be rendered. It also locates the eye views for the HMD.

#### OpenXRService::SubmitFrame
This will wait for the frame to be ready for rendering, and call `DirectXService::RenderXRFrame` on the current eye. It will then release the frame.

#### OpenXRService::UpdateActions
This will sync the OpenXR actions such as hand pose and retreive the values.

#### OpenXRService::UpdatePoses()
This will reconfigure the config file if necessary, and calculate the view matrix from the HMD pose and Origin. If the triggers are pulled, it will set the soldier aim to the controller's orientation and simulate a mouse click to fire the blaster. It will also set the global crosshair postion values that`DirectXService::RenderOverlays` uses.

#### OpenXRService::EndFrame
This will call ``xrEndFrame`` with the appropriate info.

#### OpenXRService::EndXR
This will shut down and end the OpenXR session.

### Utils
#### log
Logs a message to the console and `logs.txt`

#### IsValidPtr
Checks if a pointer is not null and is legit

#### Shutdown
Attempts to eject the mod

#### ShutdownNoHooks
Attempts to shutdown the mod from a non-hooked state.

#### LoadConfig
Opens `config.txt` and loads its values to global variables. If the file doesn't exist, it will put the mod in autoconfig mode and want the user.

#### SaveConfig
Saves certain global variables to `config.txt`

#### EulerFromQuat
Calculates an euler vector from a quaternion.

### dllmain
When the mod is injected this first opens a console, logs the time, and finds the window. If this fails it will call `ShutdownNoHooks`. 

It then loads the configuration and resizes the window to the tallest near-sqare to correct the FOV. It will then initialize minhook, the hooking library. 

It will try to call `OpenXRService::CreateXRInstanceWithExtensions` and call `ShutdownNoHooks` if that fails. 

It will wait for a few frames to render then capture the origin. It will try to call `GameService::HookCamera` and call `Shutdown` (with hooks) if that fails.

It will then run an infinite loop, calling `GameService::Reposition` if the HOME key is pressed and shutting down if the END key is presed.

# Credits
* [OpenGameCamera](https://github.com/coltonon/OpenGameCamera/)
* Some help from [BattleDash](https://github.com/BattleDash)
* My testers in my Discrod server
* An aimbot I wont mention (to solve the soldier aiming)