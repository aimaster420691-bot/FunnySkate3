# Skate 3 Recomp Fun Trainer (v2.0.2 Windows)

This package is designed for the exact v2.0.2 Windows architecture in the supplied build.

It uses the exported ReXGlue cvar API in `rexruntime.dll` instead of patching arbitrary game addresses.
The trainer DLL can toggle live renderer/camera cvars and provides fun presets.

## Hotkeys

F2 - show/hide trainer window
F3 - Moonlight preset (sun low + shafts/haze)
F4 - Chaos visuals
F5 - Super FOV
F6 - Drone/freecam
F7 - restore defaults

The trainer deliberately does NOT claim to implement player physics cheats (super jump, no bail,
gravity, balance) because those are not exposed as stable cvars in the supplied v2.0.2 binary.
Those require identifying the generated player simulation state from a source build.

## Build

Use LLVM/Clang on Windows:

clang++ -std=c++20 -shared -O2 -o Skate3FunTrainer.dll src/trainer.cpp user32.lib

For the launcher:

clang++ -std=c++20 -O2 -o Skate3TrainerLauncher.exe src/launcher.cpp user32.lib

Or use Visual Studio's clang-cl with equivalent libraries.

## Install

Keep `Skate3FunTrainer.dll` beside `Skate3TrainerLauncher.exe`.

Run the launcher, select `skate3.exe`, and launch it. The launcher injects the trainer DLL into
the running Skate 3 Recomp process. Use F2 to open the trainer window.

Only use this with your own local single-player installation.
