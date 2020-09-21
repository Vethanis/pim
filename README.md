# Pim

- [Pim](#pim)
  - [About](#about)
    - [Goals](#goals)
  - [Dependencies](#dependencies)
  - [Usage](#usage)
    - [Cloning](#cloning)
    - [Pulling](#pulling)
    - [Building](#building)
    - [Keybinds](#keybinds)

## About

Pim is a [Quake engine](https://en.wikipedia.org/wiki/Quake_engine) in an early phase of development.

Demo video:
[![](https://img.youtube.com/vi/gL8CX1NF9rw/0.jpg)](https://www.youtube.com/watch?v=gL8CX1NF9rw)

### Goals

Initial goals:
* Preserve the look and feel of Quake
  * Tight, responsive controls
  * Minimal deviation from software renderer's render target
  * Smooth, low latency interaction loop

Long term goals:
* Create user friendly in-engine tools for content creation and modding
  * Data driven
    * No hardcoded settings
    * Compatibility with original data formats
    * Optional custom formats where prudent
  * Fast iteration loop
    * Minimize time between making a change and seeing it
    * Ensure user can make edits without closing the game or map
  * Editor tools
    * CAD-like 3-view + isometric modeling tool
      * eg. UnrealEditor / Hammer
    * Inspector tool
      * View and edit fields of the selected item
    * Map tool
      * Arrange and edit elements of the map
    * Asset tool
      * Manage your inventory of game assets
    * Script tool
      * For development of gameplay behavior

## Dependencies

* [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
  * Visual C/C++ Toolset v141
  * Windows SDK 17763
  * MSBuild
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home#sdk/downloadConfirm/latest/windows/vulkan-sdk.exe)
  * Requires a Vulkan 1.2 compatible driver
  * Requires shaderc_shared.dll for runtime shader compilation (included in SDK)
  * VK_KHR_swapchain is the only mandatory extension
  * VK_NV_ray_tracing may become mandatory in the future
* [PAK files](https://store.steampowered.com/app/2310/QUAKE/)
* [Git Bash](https://git-scm.com/downloads)
  * Used for tool scripts and version control
* Optional: The [soundtrack](https://steamcommunity.com/sharedfiles/filedetails/?id=119489135)

## Usage

### Cloning

This repository uses [submodules](https://github.blog/2016-02-01-working-with-submodules/).

```
  git clone --recursive https://github.com/gheshu/pim.git
```

### Pulling

```
  git pull
  git submodule update
```

### Building

* Clone the repo recursively
* Run src/tools/dllcopy.sh in Git Bash
* Run src/tools/datacopy.sh in Git Bash
* Open proj/pim.sln in Visual Studio 2017 or newer
* Build with F7 or Ctrl+Shift+B
* Run with F5

### Keybinds

* F1: Toggle between UI mode and flycam mode
* F8: Toggle denoiser for path tracer
* F9: Toggle path tracer
* F10: Screenshot path tracer / denoiser output
* grave accent: Toggle console window
* A/D: left/right strafe in flycam mode
* W/S: forward/backward in flycam mode
* space/left shift: upward/downward in flycam mode
* mouse: yaw and pitch in flycam mode
