# BF2VR
### VR Mod for Star Wars: Battlefront II

[![](https://img.shields.io/badge/Discord-Releases%20-blueviolet)](https://discord.gg/mrKYwzd3N4)

# About
Battlefront II VR (BF2VR) is a project by developer, musician, and Star Wars enthusiast [Ethan Porcaro](https://ethanporcaro.com/). He has played Battlefront II for a while, and after being inspired by [praydog](https://github.com/praydog)'s upcoming Unreal Engine VR Injector, decided to see what would be possible to mod in the Frostbite Engine.

BF2VR started with code from [OpenGameCamera](https://github.com/coltonon/OpenGameCamera) to position the Battlefront Camera to the transformation matrix from the OpenVR API. The game's DirectX calls were intercepted to get access to the framebuffer, which was then sent to the VR screen. Stereoscopy was provided using Alternate Eye Rendering, a technique in which the rendered eye alternates every frame while the game camera shifts left and right to give a sense of depth. The games "Soldier" was also hooked to aim where the player looked.

New updates led the mod to be ported to [OpenXR](https://www.khronos.org/openxr/), a standard for XR runtime interaction. This improves performance and compatibility. Motion controlls were added, allowing the player to aim with a controller rather that the headset.

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

If you have an older version of the mod, delete `config.txt` so the new one can generate.

### Errors

If you see an error repeating over and over, please close the game and look in the game's installation folder for a file called `logs.txt`. You should send this file to me on my Discord server. If it is too large to send, then delete the repeating error but leave the first line of it. Common errors are automatically diagnosed, so read the logs.

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
This is responsible for managing hooks and calls related to DirectX. This includes:
* Hooking `Present()` from the original game.
* Drawing the presented buffer onto a fullscreen quad for OpenXR
* Drawing the crosshair over the quad.

### OpenXRService
This is responsible for OpenXR code including:
* Initialization
* Reading input
* Submitting frames

### GameService
This manages hooks and memory structure as defined in `sdk.h`. Things like:
* Setting the render view matrix.
* Setting the aim rotation.
* Toggling visibility of the game UI.

### InputService
Manages gamepad input using [ViGEm](https://github.com/ViGEm/ViGEmBus).

### Utils
Here are some useful utilities
* #### log (and others)
Logs a message to the console and `logs.txt`. Styles text color based on the logging type.

* #### IsValidPtr
Checks if a pointer is not null and has a valid memory range.

* #### Shutdown
Attempts to eject the mod from a hooked state by shutting down used hooks.

* #### ShutdownNoHooks
Attempts to shutdown the mod from a non-hooked state.

* #### LoadConfig
Opens `config.txt` and loads its values to global variables. If the file doesn't exist, it will put the mod in autoconfig mode and want the user.

* #### SaveConfig
Saves certain global variables to `config.txt`

* #### EulerFromQuat
Calculates an euler vector from a quaternion.

### dllmain
When the mod is injected this first opens a console, logs the time, and finds the window. If this fails it will call `ShutdownNoHooks`. 

It then loads the configuration and resizes the window to the tallest near-sqare to correct the FOV. It will then initialize minhook, the hooking library. 

It will try to call `OpenXRService::CreateXRInstanceWithExtensions` and call `ShutdownNoHooks` if that fails. 

It will wait for a few frames to render then capture the origin. It will try to call `GameService::HookCamera` and call `Shutdown` (with hooks) if that fails.

It will then run an infinite loop, shutting down if the END key is presed.

# Credits
* [OpenGameCamera](https://github.com/coltonon/OpenGameCamera/)
* Some help from [BattleDash](https://github.com/BattleDash)
* My testers in my Discrod server