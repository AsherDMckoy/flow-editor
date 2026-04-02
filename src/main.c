#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_WINDOW_HEIGHT 800
#define DEFAULT_WINDOW_WIDTH 800

void scc(bool);
void *scp(void *);

void fillColorBlackOrWhite(bool *isBlack, SDL_Renderer **renderer)
{
    SDL_RenderClear(*renderer);
    if (!*isBlack) {
        SDL_SetRenderDrawColor(*renderer, 0x00, 0x00, 0x00, 0x00);
    }
    else {
        SDL_SetRenderDrawColor(*renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    }
    *isBlack = !*isBlack;
}

void scc(bool code)
{
    if (!code) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error: %s", SDL_GetError());
        exit(1);
    }
}

void *scp(void *ptr)
{
    if (ptr == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL Error: %s", SDL_GetError());
        exit(1);
    }
    return ptr;
}

int main(void)
{

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;

    SDL_Log("Available renderer drivers:");
    for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
        SDL_Log("%d. %s", i + 1, SDL_GetRenderDriver(i));
    }

    scc(SDL_InitSubSystem(SDL_INIT_VIDEO));

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    scc(SDL_CreateWindowAndRenderer(
        "Flow Editor", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE, &window, &renderer));

    bool isBlack = true;
    bool running = true;
    bool functionKeyComboInitiated = false;
    while (running) {
        SDL_PollEvent(&event);
        switch (event.type) {
        case SDL_EVENT_QUIT: {
            SDL_Log("Destroying the window");
            running = false;
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            fillColorBlackOrWhite(&isBlack, &renderer);
            SDL_Log("We are painting the window after resize event");
        } break;
        case SDL_EVENT_KEY_DOWN: {
            const bool *state = (const bool *)scp((void *)SDL_GetKeyboardState(NULL));
#define CTRL_KEY_UP (!state[SDL_SCANCODE_LCTRL] && !state[SDL_SCANCODE_RCTRL])
#define CTRL_KEY_DOWN (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
            SDL_Scancode key = event.key.scancode;
            if (CTRL_KEY_DOWN) {
                SDL_Log("Ctrl key pressed");
                if (!functionKeyComboInitiated && key == SDL_SCANCODE_X) {
                    functionKeyComboInitiated = true;
                    SDL_Log("Function sequence initiated");
                }
                if (key == SDL_SCANCODE_Q) {
                    functionKeyComboInitiated = false;
                    SDL_Log("Quit");
                    // TODO(Asher) Handle function quit
                }
                if (functionKeyComboInitiated && key == SDL_SCANCODE_C) {
                    running = false;
                }
            }
            if (functionKeyComboInitiated && CTRL_KEY_UP) {
                if (key == SDL_SCANCODE_R) {
                    SDL_Log("Reset");
                    fillColorBlackOrWhite(&isBlack, &renderer);
                    functionKeyComboInitiated = false;
                }
            }
        } break;
        default: {
            SDL_RenderPresent(renderer);
        } break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
