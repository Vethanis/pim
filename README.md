# Pim

- [Pim](#pim)
  - [About](#about)
  - [Dependencies](#dependencies)
  - [Usage](#usage)
    - [Cloning](#cloning)
    - [Pulling](#pulling)
    - [Building](#building)
    - [Keybinds](#keybinds)
    - [Commands](#commands)

## About

Pim is a little "zen garden" for tinkering around, for my own personal use.

Video:
[![](https://img.youtube.com/vi/FMLjnC_sICE/0.jpg)](https://www.youtube.com/watch?v=FMLjnC_sICE)

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

* Install Visual Studio
* Install the Vulkan SDK
* Install Git Bash
* Clone the repo recursively
* Execute build.bat (change generator to 'Visual Studio 16 2019' if you are using that)
* Execute run.bat

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
* page up: increase path tracer resolution
* page down: reduce path tracer resolution


### Commands

* cmds: lists all commands
* help: displays info about commands
* exec: executes a script file
* quit: exits the program
* mapload: loads a map by name, eg. mapload e1m1
* mapsave: saves the currently loaded map to disk
* screenshot: saves a screenshot
* loadtest: loads then unloads every map file
* wait: waits N frames before executing the next command
* cornell_box: loads a cornell box scene
* teleport: teleports the camera to the given xyz coordinates
* lookat: orients the camera to look at the given xyz coordinates
* pt_stddev: calculates the standard deviation of the path tracers luminance buffer
* pt_test: runs a path tracing test


### Console Variables

See also: cvars.c

* con_logpath: path to the console log file
* in_pitchscale: pitch input sensitivity
* in_yawscale: yaw input sensitivity
* in_movescale: movement input sensitivity
* r_fov: vertical field of view, in degrees
* r_znear: near clipping plane, in meters
* r_zfar: far clipping plane, in meters
* r_whitepoint: luminance at which tonemapping will clip
* r_display_nits_min: Min display luminance, in nits
* r_display_nits_max: Max display luminance, in nits
* r_ui_nits: UI luminance, in nits
* r_maxdelqueue: Maximum number of released driver objects before force-finalizing
* r_scale: Render scale, path tracing only atm
* r_width: Unscaled render width
* r_height: Unscaled render height
* r_fpslimit: Limits fps when above this value
* r_bumpiness: Bumpiness of generated normal maps, requires map reload
* r_tex_custom: Enable loading custom textures
* r_brdflut_spf: Brdf LUT samples per frame
* pt_dist_meters: Path tracer light distribution meters per cell
* pt_trace: Enable path tracing
* pt_denoise: Denoise path tracing output
* pt_normal: Output path tracer normals
* pt_albedo: Output path tracer albedo
* r_refl_gen: Enable reflection generation
* r_sun_dir: Sun direction
* r_sun_lum: Sun luminance
* r_sun_res: Sky cubemap resolution
* r_sun_steps: Sky cubemap raymarch steps
* r_qlights: Enable loading quake light entities
* ui_opacity: UI opacity
* exp_standard: 1: Standard Calibration Exposure; 0: Saturation Based Exposure;
* exp_manual: 1: Manual Exposure; 0: Automatic Exposure
* exp_aperture: Exposure Aperture, in millimeters
* exp_shutter: Exposure Shutter Time, in seconds
* exp_adaptrate: Autoexposure Adaptation Rate
* exp_evoffset: Exposure EV compensation
* exp_evmin: Autoexposure Sensor Minimum
* exp_evmax: Autoexposure Sensor Maximum
* exp_cdfmin: Autoexposure Histogram Cdf Min
* exp_cdfmax: Autoexposure Histogram Cdf Max
* sky_rad_cr: Sky crust radius, kilometers
* sky_rad_at: Sky atmosphere relative radius, kilometers
* sky_rlh_mfp: Rayleigh mean free path, kilometers
* sky_rlh_sh: Rayleigh scale height, kilometers
* sky_mie_mfp: Mie mean free path, kilometers
* sky_mie_sh: Mie scale height, kilometers
* sky_mie_g: Mie mean scatter angle, cosine theta
* lm_upload: Upload the latest lightmap data to the GPU
* lm_gen: Progressively bake lightmaps every frame
* lm_density: Lightmap baking: texels per meter [0.1, 32]
* lm_timeslice: Lightmap baking: number of frames per sample
* lm_spp: Lightmap baking: samples per pixel
* fullscreen: Fullscreen windowing mode

