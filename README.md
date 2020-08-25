# Pim

- [Pim](#pim)
  - [About](#about)
    - [Goals](#goals)
  - [Dependencies](#dependencies)
    - [Data](#data)
  - [Usage](#usage)
    - [Cloning](#cloning)
    - [Pulling](#pulling)

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

### Data

* PAK files from [Quake](https://store.steampowered.com/app/2310/QUAKE/)
  * Place at pim/data/id1/PAK0.PAK PAK1.PAK
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

Pim currently only uses visual studio 2017 for building the project.
* After cloning the repo recursively, open the solution file under the proj folder and press F7 to build.
* You may need to copy the following DLLs into the proj/x64/Release folder from the submodules:
  * embree3.dll
  * OpenImageDenoise.dll
  * tbb.dll
  * tbbmalloc.dll
