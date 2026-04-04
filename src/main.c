#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#define WINDOW_HEIGHT 900
#define WINDOW_WIDTH 900
#define global_variable static

typedef struct {
    int x;
    int y;
} cursor;

global_variable bool running = true;
/* global_variable bool functionKeyComboInitiated = false; */
global_variable bool isClear = false;

TTF_Font *font;
TTF_TextEngine *textEngine;

size_t pos;
size_t line;

char buffer[180000];

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

void fillColorBlackOrWhite(SDL_Renderer **renderer)
{
    if (!isClear) {
        SDL_SetRenderDrawColor(*renderer, 0x00, 0x00, 0x00, 0x00);
    }
    else {
        SDL_SetRenderDrawColor(*renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    }
    isClear = !isClear;
}
int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_TextEngine *textEngine;
    SDL_Event event;
    /* TODO(Asher): Create a debug mode for debug loop blocks */
    /* SDL_Log("Available renderer drivers:"); */
    /* for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) { */
    /*     SDL_Log("%d. %s", i + 1, SDL_GetRenderDriver(i)); */
    /* } */

    scc(SDL_InitSubSystem(SDL_INIT_VIDEO));
    scc(TTF_Init());

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    scc(SDL_CreateWindowAndRenderer("Flow Editor", WINDOW_WIDTH, WINDOW_HEIGHT,
                                    SDL_WINDOW_BORDERLESS, &window, &renderer));

    textEngine = scp(TTF_CreateRendererTextEngine(renderer));

    font = (TTF_Font *)scp(TTF_OpenFont("/usr/share/fonts/TTF/PragmataPro.ttf", 18.0f));

    SDL_Rect area = { .x = 0, .y = 0, .w = 100, .h = 900 };
    TTF_Text *text;
    scc(SDL_SetTextInputArea(window, &area, 0));
    scc(SDL_StartTextInput(window));
    if (SDL_TextInputActive(window)) {
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_Log("%s", SDL_GetRendererName(renderer));
        cursor curs = { .x = 0, .y = 0 };
        pos = 0;
        line = 1;
        buffer[pos] = '\0';
        while (running) {
            int w = 0, h = 0;
            const float scale = 1.0f;

            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT: {
                    SDL_Log("Exiting flow state");
                    running = false;
                } break;
                case SDL_EVENT_TEXT_INPUT: {
                    buffer[pos] = event.text.text[0];
                    pos += 1;
                    buffer[pos] = '\0';
                } break;
                }

                /* TTF_DrawRendererText(text, dst.x, dst.y); */
                /* SDL_RenderTexture(renderer, texture, NULL, &dst); */
            }
            text = scp(TTF_CreateText(textEngine, font, buffer, pos));
            scc(TTF_SetTextColor(text, 0, 0, 255, 255));
            SDL_RenderClear(renderer);
            if (text) {
                TTF_DrawRendererText(text, curs.x, curs.y);
            }
            SDL_RenderPresent(renderer);
        }
        TTF_DestroyText(text);
        SDL_StopTextInput(window);
        TTF_CloseFont(font);
        TTF_DestroyRendererTextEngine(textEngine);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();

        return 0;
    }
}
