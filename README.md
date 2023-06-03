# BF2VR
### VR Mod for Star Wars: Battlefront II
#### Note this is a work-in-progress


[![](https://img.shields.io/badge/Discord-Releases%20-blueviolet)](https://discord.gg/mrKYwzd3N4)

# About
Battlefront II VR (BF2VR) is a project by developer, musician, and Star Wars enthusiast [Ethan Porcaro](https://ethanporcaro.com/). He has played Battlefront II for a while, and after being inspired by [praydog](https://github.com/praydog)'s upcoming Unreal Engine VR Injector, decided to see what would be possible to mod in the Frostbite Engine.

BF2VR started with code from [OpenGameCamera](https://github.com/coltonon/OpenGameCamera) to position the Battlefront Camera to the transformation matrix from the OpenVR API. The game's DirectX calls were intercepted to get access to the framebuffer, which was then sent to the VR screen. Stereoscopy was provided using Alternate Eye Rendering, a technique in which the rendered eye alternates every frame while the game camera shifts left and right to give a sense of depth. The game "Soldier" was also hooked to aim where the player looked.

New updates led the mod to be ported to [OpenXR](https://www.khronos.org/openxr/), a standard for XR runtime interaction. This improves performance and compatibility. Motion controls were added, allowing the player to aim with a controller rather than the headset.

BF2VR is in early alpha, with frequent updates but limited features. Releases to the mod can be found at Ethan's [Discord Server](https://discord.gg/mrKYwzd3N4). Keep in mind that the mod is unstable, and you won't get all the features a VR mod should include just yet.

# Using the mod
## Installation and setup
See [https://ifansnek.github.io/BF2VR-Docs/setup.html](https://ifansnek.github.io/BF2VR-Docs/setup.html).

## Playing the game
Most modes work in the game, with the following limitations:
* Mountable vehicles or turrets do not work yet. Mountable meaning the character model is also present. Eg. speeders don't work but X-wings do. Vehicles don't work *well* yet.
* **DO NOT** play multiplayer, as you risk **getting banned**. Please use [3rd party Kyber Servers](http://kyber.gg/) instead.
* Characters that do not support 1st person work, but a little bit weirdly.
* The campaign has lots of custom vehicles and modes that are incompatible with the mod currently. It is not recommended to play it yet.

Because the mod is still being worked on, you may experience random errors and crashes. Please report these.

### Hotkeys
* END: Attempts to eject the mod while keeping the game open. This sometimes crashes.
* HOME: This will reload and apply the configuration.

# Contributing
This code is open source. Please read the license if you want to fork. Contributions are welcome.
## Code structure
The code is split into the launcher and the mod DLL. The launcher is a single simple file. This explanation is for the mod DLL.

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

* #### GetClassFromName
Finds any child member class of a class reference by name. (Can be found in SDK)

## Current problems
- [ ] Add UI separation or a triggerable menu mode
- [ ] Find the mountable vehicle member for location
- [ ] Make the player turn with vehicles
- [ ] Make the crosshair look nicer and compensate for 3rd person offset from the character model
- [ ] Figure out how to set the view projection (it can be set, but it doesn't change anything) or a better auto calibration
- [ ] Make distant objects appear clearer.

# Credits
* [OpenGameCamera](https://github.com/coltonon/OpenGameCamera/)
* Some help from [BattleDash](https://github.com/BattleDash)
* My testers in my Discord server
* You?