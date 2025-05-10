cmake_minimum_required(VERSION 3.16)
project(my_libretro_core LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include FetchContent module
include(FetchContent)

# Set policy to suppress deprecation warning
cmake_policy(SET CMP0048 NEW)

# Fetch Libretro common headers
FetchContent_Declare(
    libretro_common
    GIT_REPOSITORY https://github.com/libretro/libretro-common.git
    GIT_TAG master
)
FetchContent_MakeAvailable(libretro_common)

# Fetch SDL3 (required by SDL2-compat)
FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.12
)
set(SDL3_STATIC OFF CACHE BOOL "Build SDL3 as a static library" FORCE)
set(SDL3_SHARED ON CACHE BOOL "Build SDL3 as a shared library" FORCE)
FetchContent_MakeAvailable(sdl3)

# Fetch SDL2-compat
FetchContent_Declare(
    sdl2_compat
    GIT_REPOSITORY https://github.com/libsdl-org/sdl2-compat.git
    GIT_TAG release-2.32.56
)
set(SDL2COMPAT_STATIC OFF CACHE BOOL "Build SDL2-compat as a static library" FORCE)
set(SDL2COMPAT_SHARED ON CACHE BOOL "Build SDL2-compat as a shared library" FORCE)
FetchContent_MakeAvailable(sdl2_compat)

# Fetch FreeType with minimal features and static build
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build FreeType as a static library" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable zlib for FreeType" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable bzip2 for FreeType" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG for FreeType" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz for FreeType" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli for FreeType" FORCE)
FetchContent_MakeAvailable(freetype)

# Define the shared library (Libretro core)
add_library(my_libretro_core SHARED src/main.c)

# Include directories
target_include_directories(my_libretro_core PRIVATE
    ${libretro_common_SOURCE_DIR}/include
    ${sdl2_compat_SOURCE_DIR}/include
    ${freetype_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(my_libretro_core PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    ${freetype_BINARY_DIR}/Debug/freetyped.lib # Static FreeType library
)

# Ensure the output is a .dll with no "lib" prefix
set_target_properties(my_libretro_core PROPERTIES
    PREFIX ""
    OUTPUT_NAME "my_libretro_core"
)

# Platform-specific settings for Windows
if(WIN32)
    target_link_libraries(my_libretro_core PRIVATE SDL2::SDL2main)
endif()