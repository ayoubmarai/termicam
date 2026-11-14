#include <iostream>
#include <opencv2/opencv.hpp>
#include <ncurses.h>

#include "utils.hpp"
#include "n_render.hpp"

#define DO_COLOR

int to256(int r, int g, int b, SharedState* state)
{
    r = std::clamp(r - (int)(state->roff * 255), 0, 255);
    g = std::clamp(g - (int)(state->goff * 255), 0, 255);
    b = std::clamp(b - (int)(state->boff * 255), 0, 255);
    int R, G, B;
    R = (r * 6) / 256;
    G = (g * 6) / 256;
    B = (b * 6) / 256;

    return 16 + (36 * R) + (6 * G) + B;
}

short colorCache[256] = {0};
std::string shades[] = {
    "..",
    "``",
    "^^",
    "\"\"",
    ",,",
    "::",
    ";;",
    "II",
    "ll",
    "!!",
    "ii",
    ">>",
    "<<",
    "~~",
    "++",
    "__",
    "--",
    "??",
    "]]",
    "[[",
    "}}",
    "{{",
    "11",
    "))",
    "((",
    "||",
    "\\\\",
    "//",
    "tt",
    "ff",
    "jj",
    "rr",
    "xx",
    "nn",
    "uu",
    "vv",
    "cc",
    "zz",
    "XX",
    "YY",
    "UU",
    "JJ",
    "CC",
    "LL",
    "QQ",
    "00",
    "OO",
    "ZZ",
    "mm",
    "ww",
    "qq",
    "pp",
    "dd",
    "bb",
    "kk",
    "hh",
    "aa",
    "oo",
    "**",
    "##",
    "MM",
    "WW",
    "&&",
    "88",
    "%%",
    "BB",
    "@@",
    "$$"};

static const int level[6] = {0, 95, 135, 175, 215, 255};

int nearest(int v)
{
    int min = 0;
    int min_dist = abs(v - level[0]);
    for (int i = 1; i < 6; i++)
    {

        int dist = abs(v - level[i]);
        if (dist < min_dist)
        {
            min = i;
            min_dist = dist;
        };
    };

    return min;
}

void n_render(SharedState *state)
{
    int frames_since_reset = 0;
    auto start = chrono_clock::now();
    auto pin = chrono_clock::now();
    while (Signals::running)
    {
        pin = chrono_clock::now();
        if (state->frame_count == state->current_frame)
            continue;
        int frame = state->frame_count.load();
        state->current_frame = frame;

        frames_since_reset++;
        // stopwatch(debug, last_frame, "- Render thread receiving frame");
        //  last_frame = chrono_clock::now();
        // auto before_lock = chrono_clock::now();

        state->frame_mutex.lock();

        // stopwatch(debug, before_lock, "Render thread waiting for mutex");
        // DEBUG_PRINT(state->debug, "RENDER THREAD LOCKED MUTEX\n");
        state->cols_rows_mutex.lock();

        // auto pixel = state->n_bright_frame.at<cv::Vec3b>(0, 0);
        // DEBUG_PRINT(state->debug, "[N_RENDER] Color at 0, 0: (%d, %d, %d)\n", pixel[2], pixel[1], pixel[0]);
        for (int y = 0; y < state->rows; y++)
        {
            for (int x = 0; x < state->cols / 2; x++)
            {
                cv::Vec3b pixel = state->n_frame.at<cv::Vec3b>(y, x);
                cv::Vec3b bright_pixel = state->n_bright_frame.at<cv::Vec3b>(y, x);
                uchar grayPixel = state->n_gray_frame.at<uchar>(y, x);
                int color = to256(pixel[2], pixel[1], pixel[0], state);
                int fg_color = to256(bright_pixel[2], bright_pixel[1], bright_pixel[0], state);
                if (colorCache[color] == 0)
                {
                    init_pair(color + 1, fg_color, color);
                    colorCache[color] = 1;
                    fprintf(state->debug, "Color: %d\n", color);
                    fflush(state->debug);
                }
                // if (x == 0 && y == 0)
                // {
                //     fprintf(debug, "Color at 0, 0: %d\n", color);
                //     fflush(debug);
                // }
                constexpr int N = sizeof(shades) / sizeof(shades[0]);
                std::string shade = shades[grayPixel * N / 256];
#ifdef DO_COLOR
                attron(COLOR_PAIR(color + 1));
#endif
                mvaddstr(y, x * 2, shade.c_str());
                char typed = mvinch(y, x * 2 + 1) & A_CHARTEXT;
                if (typed == 'G' || typed == 'F' || shade == "░░")
                    DEBUG_PRINT(state->debug, "Character drawn: %c, shade it was supposed to: %s\n", typed, shade.c_str());

#ifdef DO_COLOR
                attroff(COLOR_PAIR(color + 1));

                start = chrono_clock::now();
#endif
            }
        }
        refresh();
        state->frame_mutex.unlock();
        state->cols_rows_mutex.unlock();
        // stopwatch(state, start, "Drawing on screen");
        start = chrono_clock::now();
        // DEBUG_PRINT(state->debug, "RENDER THREAD UNLOCKED MUTEX\n");
        // stopwatch(state, pin, "Pin");
    }
}
