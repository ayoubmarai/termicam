#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <mutex>
#include <iostream>
#include <signal.h>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <math.h>
#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "utils.hpp"

#define DO_COLOR


int to256(int r, int g, int b)
{
    // b = std::clamp(b - 50, 0, 255);
    // r = std::clamp(r - 20, 0, 255);
    // g = std::clamp(g - 20, 0, 255);
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


int rows, cols;




void n_render(SharedState* state)
{
    int frames_since_reset = 0;
    while (Signals::running)
    {
        if (state->frame_count == state->current_frame)
            continue;
        else
            state->current_frame = state->frame_count;

        frames_since_reset++;
        // stopwatch(debug, last_frame, "- Render thread receiving frame");
        //  last_frame = chrono_clock::now();
        // auto before_lock = chrono_clock::now();

        state->frame_mutex.lock();

        // stopwatch(debug, before_lock, "Render thread waiting for mutex");
        //  debug_print(debug, "RENDER THREAD LOCKED MUTEX");
        state->cols_rows_mutex.lock();
        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols / 2; x++)
            {
                cv::Vec3b pixel = state->n_frame.at<cv::Vec3b>(y, x);
                cv::Vec3b bright_pixel = state->n_bright_frame.at<cv::Vec3b>(y, x);
                uchar grayPixel = state->n_gray_frame.at<uchar>(y, x);
                int color = to256(pixel[2], pixel[1], pixel[0]);
                int fg_color = to256(bright_pixel[2], bright_pixel[1], bright_pixel[0]);
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
#endif
            }
        }
        refresh();
        state->frame_mutex.unlock();
        state->cols_rows_mutex.unlock();
        // debug_print(debug, "RENDER THREAD UNLOCKED MUTEX");
    }
}

int main()
{
    IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    std::thread render_thread;
    signal(SIGUSR1, Signals::signal_handler);
    signal(SIGINT, Signals::signal_handler);
    signal(SIGWINCH, Signals::signal_handler);
    SharedState* state = new SharedState();
    state->debug = fopen("./output.log", "w");;

    int p2c_pipe[2], c2p_pipe[2];
    pipe(p2c_pipe);
    pipe(c2p_pipe);

    pid_t child_pid;



    DEBUG_PRINT(state->debug, "[%d] running", time(nullptr));

    cv::VideoCapture cap(0);
    initscr();
    start_color();
    noecho();
    keypad(stdscr, true);

    // ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);

    if (!cap.isOpened())
    {
        std::cerr << "Failed to open video\n";
        return 1;
    }

    cv::Mat resizedFrame, gray, display, bright_frame, frame, hsv_frame;
    double fps = cap.get(cv::CAP_PROP_FPS);

    auto prevFrame = chrono_clock::now();

    DEBUG_PRINT(state->debug, "COLS=%d LINES=%d\n", COLS, LINES);

    bool ran_thread = 0;
    float bright_brightness = 20;
    float bright_saturation = .7;

    float resized_brightness = 0;
    float resized_saturation = 1.0;

    if ((child_pid = fork()) == 0)
    {

        ImGui::CreateContext();

        close(p2c_pipe[1]);
        close(c2p_pipe[0]);
        int a = 67;
        write(c2p_pipe[1], &a, sizeof(int));

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            DEBUG_PRINT(state->debug, "SDL_Init failed: %s", SDL_GetError());
            return 1;
        }

        SDL_Window *window = SDL_CreateWindow("Nhb norkod", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 500, SDL_WINDOW_RESIZABLE);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);
        int x = 0, y = 0;
        SDL_Event event;
        while (Signals::running)
        {
            ImGui_ImplSDL2_NewFrame();
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui::NewFrame();
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                {
                    DEBUG_PRINT(state->debug, "CALLED SDL_QUIT\n");
                    Signals::running = 0;
                    break;
                }
            }

            ImGuiViewport* viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowPos(viewport->WorkPos);

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize;

            ImGui::Begin("config", nullptr, flags);

            ImGui::TextColored({1, 0, 0, 1}, "Hello");
            ImGui::End();
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            SDL_Delay(16);
            SDL_RenderPresent(renderer);
        }

        ImGui_ImplSDL2_Shutdown();
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui::DestroyContext();
        SDL_Quit();
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
    }
    else
    {
        close(c2p_pipe[1]);
        close(p2c_pipe[0]);
        int a;
        read(c2p_pipe[0], &a, sizeof(int));

        DEBUG_PRINT(state->debug, "RECIEVED THIS MESSAGE FROM PIPE: %d\n", a);
        while (Signals::running)
        {

            // auto before_lock = chrono_clock::now();
            state->frame_mutex.lock();
            // stopwatch(debug, before_lock, "Main thread waiting for mutex");
            // debug_print(debug, "MAIN THREAD LOCKED MUTEX");
            getmaxyx(stdscr, rows, cols);
            // auto SOMETHING = chrono_clock::now();

            if (!cap.read(frame))
            {
                DEBUG_PRINT(state->debug, "[WARNING] cap.read failed");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                state->frame_mutex.unlock();
                continue;
            }
            float aspect_ratio = static_cast<float>(cols / 2) / rows;
            cv::resize(frame, resizedFrame, cv::Size(cols / 2, rows));
            float delta = std::chrono::duration<float>(chrono_clock::now() - prevFrame).count();
            DEBUG_PRINT(state->debug, "Delta=%f\n", delta);
            prevFrame = chrono_clock::now();

            cv::cvtColor(resizedFrame, gray, cv::COLOR_BGR2GRAY);

            cv::cvtColor(resizedFrame, bright_frame, cv::COLOR_BGR2HSV);
            std::vector<cv::Mat> ch;
            cv::split(bright_frame, ch);
            ch[1].convertTo(ch[1], -1, bright_saturation, 0);
            ch[2].convertTo(ch[2], -1, 1.0, bright_brightness);
            cv::merge(ch, bright_frame);
            cv::cvtColor(bright_frame, bright_frame, cv::COLOR_HSV2BGR);

            cv::cvtColor(resizedFrame, resizedFrame, cv::COLOR_BGR2HSV);
            cv::split(resizedFrame, ch);
            ch[1].convertTo(ch[1], -1, resized_saturation, 0);   // Saturation
            ch[2].convertTo(ch[2], -1, 1.0, resized_brightness); // Brightness
            cv::merge(ch, resizedFrame);
            cv::cvtColor(resizedFrame, resizedFrame, cv::COLOR_HSV2BGR);

            state->n_gray_frame = gray.clone();
            state->n_frame = resizedFrame.clone();
            state->n_bright_frame = bright_frame.clone();
            state->frame_count++;
            state->frame_mutex.unlock();

            // stopwatch(debug, before_lock, "Locking and unlocking mutex from main thread");
            // debug_print(debug, "MAIN THREAD UNLOCKED MUTEX");

            if (!ran_thread)
            {
                ran_thread = 1;
                render_thread = std::thread(n_render, state);
            }

            cv::resize(bright_frame, display, cv::Size(500 * aspect_ratio, 500));
            cv::resize(resizedFrame, resizedFrame, cv::Size(500 * aspect_ratio, 500));
            // cv::imshow("Bright_gray", display);
            // cv::imshow("original", resizedFrame);
            if (Signals::resized)
            {
                state->cols_rows_mutex.lock();
                DEBUG_PRINT(state->debug, "[WARNING] RESIZED\n", cols, rows);
                Signals::resized = 0;
                endwin();
                refresh();
                getmaxyx(stdscr, rows, cols);
                state->cols_rows_mutex.unlock();
            }
            // stopwatch(debug, SOMETHING, "SOME THING IN THE MAIN THREAD SLOWING EVERYTHING");
        }

        DEBUG_PRINT(state->debug, "bright_brightness: %f, bright_saturation: %f, resized_brightness: %f, resized_saturation: %f\n", bright_brightness, bright_saturation, resized_brightness, resized_saturation);

        DEBUG_PRINT(state->debug, "\nExited safely\n");
        cv::destroyAllWindows();
        fclose(state->debug);
        endwin();
    }
    return 0;
}