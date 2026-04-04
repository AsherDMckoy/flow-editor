#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_HEIGHT 900
#define WINDOW_WIDTH 900
#define global_variable static
#define INITIAL_BUFFER_CAPACITY (1u << 20)
#define DEFAULT_PADDING 24
#define DEFAULT_TAB_WIDTH 4
#define TARGET_FPS 120

global_variable bool running = true;

void *scp(void *ptr)
{
    if (ptr == NULL) {
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

typedef struct {
    char *text;
    size_t len;
    size_t cap;
    size_t cursor;

    char *display;
    size_t display_cap;

    int padding;
    int tab_width;
    int glyph_w;
    int line_h;

    bool cursor_visible;
    Uint64 cursor_blink_ms;
    Uint64 next_cursor_toggle_ms;
} editor_state;

static bool ensure_text_capacity(editor_state *ed, size_t needed)
{
    if (needed <= ed->cap) {
        return true;
    }

    size_t next_cap = ed->cap;
    while (next_cap < needed) {
        if (next_cap > (SIZE_MAX / 2)) {
            return false;
        }
        next_cap *= 2;
    }

    char *new_text = (char *)realloc(ed->text, next_cap);
    if (new_text == NULL) {
        return false;
    }
    ed->text = new_text;
    ed->cap = next_cap;
    return true;
}

static bool ensure_display_capacity(editor_state *ed, size_t needed)
{
    if (needed <= ed->display_cap) {
        return true;
    }

    size_t next_cap = ed->display_cap;
    while (next_cap < needed) {
        if (next_cap > (SIZE_MAX / 2)) {
            return false;
        }
        next_cap *= 2;
    }

    char *new_display = (char *)realloc(ed->display, next_cap);
    if (new_display == NULL) {
        return false;
    }
    ed->display = new_display;
    ed->display_cap = next_cap;
    return true;
}

static size_t prev_utf8_start(const char *text, size_t cursor)
{
    if (cursor == 0) {
        return 0;
    }
    size_t idx = cursor - 1;
    while (idx > 0 && (((unsigned char)text[idx] & 0xC0u) == 0x80u)) {
        idx--;
    }
    return idx;
}

static size_t next_utf8_index(const char *text, size_t len, size_t cursor)
{
    if (cursor >= len) {
        return len;
    }
    size_t idx = cursor + 1;
    while (idx < len && (((unsigned char)text[idx] & 0xC0u) == 0x80u)) {
        idx++;
    }
    return idx;
}

static void reset_cursor_blink(editor_state *ed)
{
    ed->cursor_visible = true;
    ed->next_cursor_toggle_ms = SDL_GetTicks() + ed->cursor_blink_ms;
}

static bool insert_bytes(editor_state *ed, const char *bytes, size_t count)
{
    size_t needed = ed->len + count + 1;
    if (!ensure_text_capacity(ed, needed)) {
        return false;
    }

    memmove(ed->text + ed->cursor + count,
            ed->text + ed->cursor,
            ed->len - ed->cursor + 1);
    memcpy(ed->text + ed->cursor, bytes, count);
    ed->cursor += count;
    ed->len += count;
    return true;
}

static void delete_backspace(editor_state *ed)
{
    if (ed->cursor == 0) {
        return;
    }
    size_t start = prev_utf8_start(ed->text, ed->cursor);
    size_t removed = ed->cursor - start;
    memmove(ed->text + start,
            ed->text + ed->cursor,
            ed->len - ed->cursor + 1);
    ed->len -= removed;
    ed->cursor = start;
}

static size_t line_start(const editor_state *ed)
{
    size_t idx = ed->cursor;
    while (idx > 0 && ed->text[idx - 1] != '\n') {
        idx--;
    }
    return idx;
}

static size_t line_end(const editor_state *ed)
{
    size_t idx = ed->cursor;
    while (idx < ed->len && ed->text[idx] != '\n') {
        idx++;
    }
    return idx;
}

static void advance_layout_for_char(char c, int max_cols, int tab_width, int *row, int *col)
{
    if (c == '\n') {
        *row += 1;
        *col = 0;
        return;
    }

    if (c == '\t') {
        int spaces = tab_width - (*col % tab_width);
        for (int i = 0; i < spaces; i++) {
            if (*col >= max_cols) {
                *row += 1;
                *col = 0;
            }
            *col += 1;
        }
        return;
    }

    if (*col >= max_cols) {
        *row += 1;
        *col = 0;
    }
    *col += 1;
}

static void cursor_layout_position(const editor_state *ed, int max_cols, int *out_row, int *out_col)
{
    int row = 0;
    int col = 0;
    for (size_t i = 0; i < ed->cursor; i++) {
        advance_layout_for_char(ed->text[i], max_cols, ed->tab_width, &row, &col);
    }
    *out_row = row;
    *out_col = col;
}

static bool build_display_text(editor_state *ed, int max_cols, size_t *out_len)
{
    size_t worst_case = ed->len * (size_t)(ed->tab_width + 1) + 2;
    if (!ensure_display_capacity(ed, worst_case)) {
        return false;
    }

    size_t out = 0;
    int col = 0;

    for (size_t i = 0; i < ed->len; i++) {
        char c = ed->text[i];
        if (c == '\n') {
            ed->display[out++] = '\n';
            col = 0;
            continue;
        }

        if (c == '\t') {
            int spaces = ed->tab_width - (col % ed->tab_width);
            for (int s = 0; s < spaces; s++) {
                if (col >= max_cols) {
                    ed->display[out++] = '\n';
                    col = 0;
                }
                ed->display[out++] = ' ';
                col++;
            }
            continue;
        }

        if (col >= max_cols) {
            ed->display[out++] = '\n';
            col = 0;
        }
        ed->display[out++] = c;
        col++;
    }

    ed->display[out] = '\0';
    *out_len = out;
    return true;
}
int main(void)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_TextEngine *textEngine = NULL;
    TTF_Font *font = NULL;
    SDL_Event event;

    editor_state ed = { 0 };
    ed.cap = INITIAL_BUFFER_CAPACITY;
    ed.display_cap = INITIAL_BUFFER_CAPACITY;
    ed.padding = DEFAULT_PADDING;
    ed.tab_width = DEFAULT_TAB_WIDTH;
    ed.cursor_visible = true;
    ed.cursor_blink_ms = 500;
    ed.next_cursor_toggle_ms = SDL_GetTicks() + ed.cursor_blink_ms;

    ed.text = (char *)scp(malloc(ed.cap));
    ed.display = (char *)scp(malloc(ed.display_cap));
    ed.text[0] = '\0';

    scc(SDL_Init(SDL_INIT_VIDEO));
    scc(TTF_Init());
    scc(SDL_CreateWindowAndRenderer("Flow Editor", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer));
    SDL_SetRenderVSync(renderer, 1);

    textEngine = scp(TTF_CreateRendererTextEngine(renderer));
    font = (TTF_Font *)scp(TTF_OpenFont("/usr/share/fonts/TTF/PragmataPro.ttf", 18.0f));

    int glyph_h = 0;
    scc(TTF_GetStringSize(font, "M", 1, &ed.glyph_w, &glyph_h));
    ed.line_h = TTF_GetFontHeight(font);
    if (ed.line_h <= 0) {
        ed.line_h = glyph_h;
    }
    if (ed.glyph_w <= 0) {
        ed.glyph_w = 10;
    }
    if (ed.line_h <= 0) {
        ed.line_h = 18;
    }

    SDL_Rect area = { .x = ed.padding, .y = ed.padding, .w = WINDOW_WIDTH - (2 * ed.padding), .h = WINDOW_HEIGHT - (2 * ed.padding) };
    scc(SDL_SetTextInputArea(window, &area, 0));
    scc(SDL_StartTextInput(window));
    if (!SDL_TextInputActive(window)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Text input did not activate.");
        running = false;
    }

    while (running) {
        Uint64 frame_start_ms = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                area.x = ed.padding;
                area.y = ed.padding;
                area.w = event.window.data1 - (2 * ed.padding);
                area.h = event.window.data2 - (2 * ed.padding);
                scc(SDL_SetTextInputArea(window, &area, 0));
                break;
            case SDL_EVENT_TEXT_INPUT: {
                size_t bytes = SDL_strlen(event.text.text);
                if (bytes > 0 && !insert_bytes(&ed, event.text.text, bytes)) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Out of memory while inserting text.");
                    running = false;
                }
                reset_cursor_blink(&ed);
            } break;
            case SDL_EVENT_KEY_DOWN: {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_ESCAPE) {
                    running = false;
                    break;
                }
                if (key == SDLK_BACKSPACE) {
                    delete_backspace(&ed);
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (!insert_bytes(&ed, "\n", 1)) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Out of memory while inserting newline.");
                        running = false;
                    }
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_TAB) {
                    if (!insert_bytes(&ed, "\t", 1)) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Out of memory while inserting tab.");
                        running = false;
                    }
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_LEFT) {
                    ed.cursor = prev_utf8_start(ed.text, ed.cursor);
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_RIGHT) {
                    ed.cursor = next_utf8_index(ed.text, ed.len, ed.cursor);
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_HOME) {
                    ed.cursor = line_start(&ed);
                    reset_cursor_blink(&ed);
                    break;
                }
                if (key == SDLK_END) {
                    ed.cursor = line_end(&ed);
                    reset_cursor_blink(&ed);
                    break;
                }
            } break;
            }
        }

        Uint64 now_ms = SDL_GetTicks();
        if (now_ms >= ed.next_cursor_toggle_ms) {
            ed.cursor_visible = !ed.cursor_visible;
            ed.next_cursor_toggle_ms = now_ms + ed.cursor_blink_ms;
        }

        int win_w = 0;
        int win_h = 0;
        scc(SDL_GetWindowSize(window, &win_w, &win_h));
        int usable_w = win_w - (2 * ed.padding);
        if (usable_w < ed.glyph_w) {
            usable_w = ed.glyph_w;
        }
        int max_cols = usable_w / ed.glyph_w;
        if (max_cols < 1) {
            max_cols = 1;
        }

        size_t display_len = 0;
        if (!build_display_text(&ed, max_cols, &display_len)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Out of memory while building display text.");
            break;
        }

        SDL_SetRenderDrawColor(renderer, 0x16, 0x16, 0x1A, 0xFF);
        scc(SDL_RenderClear(renderer));

        TTF_Text *text = scp(TTF_CreateText(textEngine, font, ed.display, display_len));
        scc(TTF_SetTextColor(text, 0xE8, 0xE8, 0xEC, 0xFF));
        TTF_DrawRendererText(text, ed.padding, ed.padding);
        TTF_DestroyText(text);

        int cursor_row = 0;
        int cursor_col = 0;
        cursor_layout_position(&ed, max_cols, &cursor_row, &cursor_col);
        if (ed.cursor_visible) {
            SDL_FRect caret = {
                .x = (float)(ed.padding + (cursor_col * ed.glyph_w)),
                .y = (float)(ed.padding + (cursor_row * ed.line_h)),
                .w = 2.0f,
                .h = (float)ed.line_h
            };
            scc(SDL_SetRenderDrawColor(renderer, 0x7A, 0xD7, 0xF0, 0xFF));
            scc(SDL_RenderFillRect(renderer, &caret));
        }

        scc(SDL_RenderPresent(renderer));

        Uint64 elapsed_ms = SDL_GetTicks() - frame_start_ms;
        const Uint64 target_frame_ms = 1000u / TARGET_FPS;
        if (elapsed_ms < target_frame_ms) {
            SDL_Delay((Uint32)(target_frame_ms - elapsed_ms));
        }
    }

    SDL_StopTextInput(window);
    free(ed.text);
    free(ed.display);
    TTF_CloseFont(font);
    TTF_DestroyRendererTextEngine(textEngine);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
