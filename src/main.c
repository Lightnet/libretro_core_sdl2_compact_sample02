#include <libretro.h>
#include <SDL.h> // SDL2-compat provides SDL2 API
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string.h>
#include <stdio.h>

// Global Libretro callbacks
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Core state
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* screen_texture = NULL;
static FT_Library ft_library = NULL;
static FT_Face ft_face = NULL;
static int running = 0;

// Core information
void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(struct retro_system_info));
    info->library_name = "My Libretro Core";
    info->library_version = "1.0";
    info->valid_extensions = "";
    info->need_fullpath = 0;
    info->block_extract = 0;
}

// System A/V information
void retro_get_system_av_info(struct retro_system_av_info* info) {
    info->geometry.base_width = 640;
    info->geometry.base_height = 480;
    info->geometry.max_width = 640;
    info->geometry.max_height = 480;
    info->geometry.aspect_ratio = 4.0f / 3.0f;
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;
}

// Initialize the core
void retro_init(void) {
    printf("Initializing core...\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    if (FT_Init_FreeType(&ft_library) != 0) {
        fprintf(stderr, "FT_Init_FreeType failed\n");
        SDL_Quit();
        return;
    }

    // Try multiple font paths
    const char* font_paths[] = {
      "fonts/Kenney Mini.ttf"
    };
    int font_loaded = 0;
    FT_Error error;
    for (int i = 0; i < 3; i++) {
        error = FT_New_Face(ft_library, font_paths[i], 0, &ft_face);
        if (error == 0) {
            printf("Loaded font: %s\n", font_paths[i]);
            font_loaded = 1;
            break;
        }
        fprintf(stderr, "FT_New_Face failed for %s (error code: %d)\n", font_paths[i], error);
    }
    if (!font_loaded) {
        fprintf(stderr, "Could not load any font. Ensure arial.ttf is in one of the specified paths.\n");
        FT_Done_FreeType(ft_library);
        SDL_Quit();
        return;
    }

    // Set font size (24pt at 72 DPI)
    error = FT_Set_Char_Size(ft_face, 0, 24 * 64, 72, 72);
    if (error != 0) {
        fprintf(stderr, "FT_Set_Char_Size failed (error code: %d)\n", error);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        SDL_Quit();
        return;
    }

    window = SDL_CreateWindow(
        "My Libretro Core",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_HIDDEN // Hidden until rendering
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        SDL_Quit();
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        SDL_Quit();
        return;
    }

    screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        640, 480
    );
    if (!screen_texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        FT_Done_Face(ft_face);
        FT_Done_FreeType(ft_library);
        SDL_Quit();
        return;
    }

    running = 1;
}

// Deinitialize the core
void retro_deinit(void) {
    if (ft_face) FT_Done_Face(ft_face);
    if (ft_library) FT_Done_FreeType(ft_library);
    if (screen_texture) SDL_DestroyTexture(screen_texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    ft_face = NULL;
    ft_library = NULL;
    screen_texture = NULL;
    renderer = NULL;
    window = NULL;
    SDL_Quit();
    running = 0;
}

// Set environment callback
void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
    bool support = true;
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &support);
    unsigned pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

// Set video refresh callback
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }

// Set audio sample callback
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }

// Set audio sample batch callback
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }

// Set input callbacks
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

// Serialization functions
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void* data, size_t size) { return false; }
bool retro_unserialize(const void* data, size_t size) { return false; }

// Reset (not used)
void retro_reset(void) {}

// Run one frame
void retro_run(void) {
    if (!running) return;

    printf("Running frame...\n");

    // Lock the texture for direct pixel access
    void* pixels;
    int pitch;
    if (SDL_LockTexture(screen_texture, NULL, &pixels, &pitch) != 0) {
        fprintf(stderr, "SDL_LockTexture failed: %s\n", SDL_GetError());
        return;
    }

    // Clear the texture to black (XRGB8888 format)
    uint32_t* pixel_data = (uint32_t*)pixels;
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            pixel_data[y * (pitch / 4) + x] = 0xFF000000; // Black (0xAARRGGBB)
        }
    }

    // Draw a red rectangle (100,100,200,200)
    for (int y = 100; y < 300; y++) {
        for (int x = 100; x < 300; x++) {
            pixel_data[y * (pitch / 4) + x] = 0xFFFF0000; // Red (0xAARRGGBB)
        }
    }

    // Render "Hello World" text at (50, 50) using FreeType
    const char* text = "Hello World";
    int x = 50;
    int y = 50; // Baseline position
    int baseline_offset = 24; // Adjust for 24pt font to align text bottom at y=50
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(ft_face, *c, FT_LOAD_RENDER) != 0) {
            fprintf(stderr, "FT_Load_Char failed for '%c'\n", *c);
            continue;
        }

        FT_GlyphSlot slot = ft_face->glyph;
        FT_Bitmap* bitmap = &slot->bitmap;

        printf("Glyph '%c': width=%d, rows=%d, top=%d, left=%d, advance=%d\n",
               *c, bitmap->width, bitmap->rows, slot->bitmap_top, slot->bitmap_left, slot->advance.x >> 6);

        for (int by = 0; by < bitmap->rows && by + y - slot->bitmap_top + baseline_offset < 480; by++) {
            for (int bx = 0; bx < bitmap->width && bx + x + slot->bitmap_left < 640; bx++) {
                unsigned char pixel = bitmap->buffer[by * bitmap->width + bx];
                if (pixel > 0) { // Draw non-transparent pixels as white
                    pixel_data[(by + y - slot->bitmap_top + baseline_offset) * (pitch / 4) + (bx + x + slot->bitmap_left)] = 0xFFFFFFFF; // White
                }
            }
        }

        x += slot->advance.x >> 6; // Advance cursor (in 1/64th pixels)
    }

    SDL_UnlockTexture(screen_texture);

    // Send frame to RetroArch
    video_cb(pixels, 640, 480, pitch);

    // Optional: Update SDL renderer for debugging
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect rect = {100, 100, 200, 200};
    SDL_RenderFillRect(renderer, &rect);
    SDL_RenderPresent(renderer);

    // Poll input
    input_poll_cb();

    // No audio
    audio_batch_cb(NULL, 0);
}

// Load content (not used)
bool retro_load_game(const struct retro_game_info* game) {
    return true;
}

// Unload content
void retro_unload_game(void) {}

// Get API version
unsigned retro_api_version(void) { return RETRO_API_VERSION; }

// Set controller description (optional)
void retro_set_controller_port_device(unsigned port, unsigned device) {}

// Other unused callbacks
unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }
void* retro_get_memory_data(unsigned id) { return NULL; }
size_t retro_get_memory_size(unsigned id) { return 0; }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info) { return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char* code) {}