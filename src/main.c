#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define WINDOW_HEIGHT 900
#define WINDOW_WIDTH 900
#define global_variable static
#define local_persist static
#define internal static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

global_variable bool running = true;
global_variable SDL_Texture *Texture;
global_variable void *BitMapMemory;
global_variable int BitMapWidth;
global_variable int BitMapHeight;
global_variable int BytesPerPixel = 4;

void *scp(void *ptr)
{
    if (ptr == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error: %s", SDL_GetError());
        exit(1);
    }
    return ptr;
}
void scc(bool code)
{
    if (!code) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error: %s", SDL_GetError());
        exit(1);
    }
}
internal void RenderWeirdGradient(int BlueOffset, int GreenOffset)
{
    int Width = BitMapWidth;
    int Height = BitMapHeight;

    int Pitch = Width * BytesPerPixel;
    uint8 *Row = (uint8 *)BitMapMemory;
    for (int Y = 0; Y < BitMapHeight; ++Y) {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < BitMapWidth; ++X) {
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);

            // ARGB8888 format: (Alpha << 24) | (Red << 16) | (Green << 8) | Blue
            *Pixel++ = (0xFF << 24) | (Green << 8) | Blue;
        }

        Row += Pitch;
    }
}
internal void ResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
    if (BitMapMemory) {
        munmap(BitMapMemory, BitMapWidth * BitMapHeight * BytesPerPixel);
    }
    if (Texture) {
        SDL_DestroyTexture(Texture);
    }
    Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, Width, Height);
    BitMapWidth = Width;
    BitMapHeight = Height;
    BitMapMemory = mmap(0, Width * Height * BytesPerPixel, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
internal void UpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer)
{
    SDL_UpdateTexture(Texture, 0, BitMapMemory, BitMapWidth * BytesPerPixel);
    SDL_RenderTexture(Renderer, Texture, NULL, NULL);
    SDL_RenderPresent(Renderer);
}

void HandleEvent(SDL_Event *event)
{
    switch (event->type) {
    case SDL_EVENT_QUIT: {
        SDL_Log("Exiting flow state");
        running = false;
    } break;
    case SDL_EVENT_WINDOW_RESIZED: {
        SDL_Window *Window = SDL_GetWindowFromID(event->window.windowID);
        SDL_Renderer *Renderer = SDL_GetRenderer(Window);
        SDL_Log("SDL_WINDOW_RESIZED (%d, %d)", event->window.data1, event->window.data2);
        ResizeTexture(Renderer, event->window.data1, event->window.data2);
    } break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED: {
        SDL_Log("Focused Gained!");
    } break;
    case SDL_EVENT_WINDOW_EXPOSED: {
        SDL_Window *Window = SDL_GetWindowFromID(event->window.windowID);
        SDL_Renderer *Renderer = SDL_GetRenderer(Window);
        /* static bool IsWhite = true; */
        /* if (IsWhite == true) { */
        /*     SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255); */
        /*     IsWhite = false; */
        /* } */
        /* else { */
        /*     SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255); */
        /*     IsWhite = false; */
        /* } */
        /* SDL_RenderClear(Renderer); */
        /* SDL_RenderPresent(Renderer); */
        UpdateWindow(Window, Renderer);
    } break;
    case SDL_EVENT_TEXT_INPUT: {
        SDL_Log("Text input");
    } break;
    }
}

int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    /* TODO(Asher): Create a debug mode for debug loop blocks */
    /* SDL_Log("Available renderer drivers:"); */
    /* for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) { */
    /*     SDL_Log("%d. %s", i + 1, SDL_GetRenderDriver(i)); */
    /* } */

    scc(SDL_InitSubSystem(SDL_INIT_VIDEO));

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, SDL_SOFTWARE_RENDERER);

    scc(SDL_CreateWindowAndRenderer("Flow Editor", WINDOW_WIDTH, WINDOW_HEIGHT,
                                    SDL_WINDOW_BORDERLESS /*| SDL_WINDOW_RESIZABLE*/,
                                    &window, &renderer));
    ResizeTexture(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    int XOffset = 0;
    int YOffset = 0;
    while (running) {
        while (SDL_PollEvent(&event)) {
            HandleEvent(&event);
        }
        RenderWeirdGradient(XOffset, YOffset);
        UpdateWindow(window, renderer);
        ++XOffset;
        YOffset += 2;
    }
    SDL_StopTextInput(window);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
