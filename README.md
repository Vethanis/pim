# Pim

- [Pim](#pim)
  - [About](#about)
    - [Goals](#goals)
  - [Dependencies](#dependencies)
    - [Data](#data)
    - [Tools](#tools)
  - [Usage](#usage)
    - [Cloning](#cloning)
    - [Pulling](#pulling)
    - [Project File Generation](#project-file-generation)
    - [Compiling](#compiling)
    - [Running](#running)
    - [Cleaning](#cleaning)

## About

Pim is a [Quake engine](https://en.wikipedia.org/wiki/Quake_engine) in the conception phase of development.

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
      * Language will probably be either C (tcc + dll reload), or Lua

## Dependencies

### Data

* PAK files from [Quake](https://store.steampowered.com/app/2310/QUAKE/)
* Optional: The [soundtrack](https://steamcommunity.com/sharedfiles/filedetails/?id=119489135)

### Tools

* [clang-cl](https://llvm.org/builds/)
* [git bash](https://git-scm.com/downloads)
* [cmake](https://cmake.org/download/)

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

### Project File Generation

This step is only required on the first build or after editing a CMakeLists.txt file.

```
  ./generate
```

### Compiling

Options:
* $1: Debug, Release
  * Defaults to Debug

```
  ./compile Debug
```

### Running

Options:
* $1: Debug, Release
  * Defaults to Debug

```
  ./run Debug
```

### Cleaning

Removes the bin and build folders. Useful if you want to force a full recompile.

```
  ./clean
```