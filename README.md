# libretro_core_sdl2_compact_sample

# License: MIT

# Information:
  Simple test to run retroarch to load libretro core to render red sqaure with sdl2-compat.

  Render Text Hello World. Using the freetype font loader.

  SDL_TTF does not work as conlficts from SDL2 and SDL3 which config to SDL2-compat cmake library.

# Tools:
- CMake
- VS2022

# Tested Window:
 Current need font folder on retroarch.exe current dir folder to work.

```
...\RetroArch-Win64\font\Kenney Mini.ttf
```
this is need to load font from my_libretro_core.dll.

```
...\RetroArch-Win64\ports\my_libretro_core
...\RetroArch-Win64\ports\my_libretro_core\font
...\RetroArch-Win64\ports\my_libretro_core\my_libretro_core.dll
```

```
@echo off
set RETROARCH_DIR=...\RetroArch-Win64
set CORE_DIR=%RETROARCH_DIR%\ports\my_libretro_core
cd %RETROARCH_DIR%
retroarch.exe --libretro "%CORE_DIR%\my_libretro_core.dll"
```

This is reference. 