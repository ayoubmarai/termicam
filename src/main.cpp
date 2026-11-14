#include <linux/prctl.h> /* Definition of PR_* constants */
#include <sys/prctl.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <mutex>
#include <iostream>
#include <signal.h>
#include <format>
#include <unistd.h>
#include <ncurses.h>
#include <poll.h>
#include <math.h>
#include <chrono>
#include <iomanip>
#include <thread>
#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "utils.hpp"
#include "image.hpp"
#include "n_render.hpp"

int main()
{
    IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    std::thread render_thread;
    signal(SIGUSR1, Signals::signal_handler);
    signal(SIGINT, Signals::signal_handler);
    signal(SIGWINCH, Signals::signal_handler);
    SharedState *state = new SharedState();
    state->debug = fopen("./output.log", "w");
    ;

    int p2c_pipe[2], c2p_pipe[2];
    pipe(p2c_pipe);
    pipe(c2p_pipe);

    pid_t child_pid;

    DEBUG_PRINT(state->debug, "[%s] Started\n", timestamp());

    cv::VideoCapture cap(0);
    cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 3);
    DEBUG_PRINT(state->debug, "Exposure=%f, Auto exposure=%f\n", cap.get(cv::CAP_PROP_EXPOSURE), cap.get(cv::CAP_PROP_AUTO_EXPOSURE));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);

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

    cv::Mat frame;
    double fps = cap.get(cv::CAP_PROP_FPS);

    auto prevFrame = chrono_clock::now();

    DEBUG_PRINT(state->debug, "COLS=%d LINES=%d\n", COLS, LINES);

    bool ran_thread = 0;

    if ((child_pid = fork()) == 0)
    {
        pid_t parent = getppid();
        ImGui::CreateContext();
        prctl(PR_SET_PDEATHSIG, SIGINT);

        close(p2c_pipe[1]);
        close(c2p_pipe[0]);

        pollfd pfd;
        pfd.fd = p2c_pipe[0];
        pfd.events = POLLIN;

        if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            DEBUG_PRINT(state->debug, "SDL_Init failed: %s", SDL_GetError());
            return 1;
        }

        SDL_Window *window = SDL_CreateWindow("Nhb norkod taw", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 300, SDL_WINDOW_RESIZABLE);
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer2_Init(renderer);
        int x = 0, y = 0;
        SDL_Event event;
        float fg_brightness = 1.2f, fg_saturation = 0.7f;
        float bg_brightness = 1.0f, bg_saturation = 1.0f;

        float delta = 0, exposure = 50;
        float color_added[3] = {0, 0, 0};
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
                    if (getppid() == parent)
                        kill(parent, SIGINT);
                    Signals::running = 0;
                }
            }

            ImGuiViewport *viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowPos(viewport->WorkPos);

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

            int ready = poll(&pfd, 1, 10);

            if (ready > 0)
            {
                PipeMsg in_msg;
                read(p2c_pipe[0], &in_msg, sizeof(PipeMsg));
                if (in_msg.type == DELTA)
                    delta = in_msg.floatVal;
            }

            ImGui::Begin("config", nullptr, flags);

            ImGui::TextColored({1, 0, 0, 1}, "TEST");
            if (ImGui::DragFloat("FG Brightness", &fg_brightness, 0.05f, 0, 5))
            {
                PipeMsg out_msg;
                out_msg.type = MessageType::FG_BRIGHTNESS;
                out_msg.floatVal = fg_brightness;

                write(c2p_pipe[1], &out_msg, sizeof(out_msg));
            }
            if (ImGui::DragFloat("FG Saturation", &fg_saturation, 0.05f, 0, 5))
            {
                PipeMsg out_msg;
                out_msg.type = MessageType::FG_SATURATION;
                out_msg.floatVal = fg_saturation;

                write(c2p_pipe[1], &out_msg, sizeof(out_msg));
            }

            if (ImGui::DragFloat("BG Brightness", &bg_brightness, 0.05f, 0, 5))
            {
                PipeMsg out_msg;
                out_msg.type = MessageType::BG_BRIGHTNESS;
                out_msg.floatVal = bg_brightness;

                write(c2p_pipe[1], &out_msg, sizeof(out_msg));
            }

            if (ImGui::DragFloat("BG Saturation", &bg_saturation, 0.05f, 0, 5))
            {
                PipeMsg out_msg;
                out_msg.type = MessageType::BG_SATURATION;
                out_msg.floatVal = bg_saturation;

                write(c2p_pipe[1], &out_msg, sizeof(out_msg));
            }

            if (ImGui::ColorEdit3("Color", color_added))
            {
                PipeMsg out_msg;
                out_msg.type = COLOR_OFFSET;
                out_msg.colorVal[0] = color_added[0];
                out_msg.colorVal[1] = color_added[1];
                out_msg.colorVal[2] = color_added[2];
                write(c2p_pipe[1], &out_msg, sizeof(PipeMsg));
            };

            if (ImGui::Button("Report"))
            {
                DEBUG_PRINT(state->debug, "[%s] REPORT\n", timestamp());
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset color pairs"))
            {
                PipeMsg out_msg;
                out_msg.type = RESET_PAIR;
                write(c2p_pipe[1], &out_msg, sizeof(PipeMsg));
                DEBUG_PRINT(state->debug, "[%s] Reset pairs\n", timestamp());
            }

            ImGui::Text(std::format("Delta: {}", delta).c_str());
            ImGui::End();
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

            SDL_Delay(16);
            SDL_RenderPresent(renderer);
        }
        DEBUG_PRINT(state->debug, "[CHILD PROCESS] Exited Safely\n");
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

        pollfd pfd;
        pfd.fd = c2p_pipe[0];
        pfd.events = POLLIN;

        float brightness;

        FrameProcessor Frame_Processor;
        Frame_Processor.state = state;
        getmaxyx(stdscr, state->rows, state->cols);
        state->aspect_ratio = static_cast<float>(state->cols / 2) / state->rows;
        state->roff = 0;
        state->goff = 0;
        state->boff = 0;
        while (Signals::running)
        {

            float delta = std::chrono::duration<float>(chrono_clock::now() - prevFrame).count();
            {
                PipeMsg out_msg;
                out_msg.type = DELTA;
                out_msg.floatVal = delta;
                write(p2c_pipe[1], &out_msg, sizeof(PipeMsg));
            }
            if (delta > 0.12)
                DEBUG_PRINT(state->debug, "[WARNING] TOOK TOO LONG Delta=%f\n", delta);
            prevFrame = chrono_clock::now();

            // DEBUG_PRINT(state->debug, "MAIN THREAD LOCKED MUTEX\n");
            auto before_lock = chrono_clock::now();
            if (!cap.read(frame))
            {
                DEBUG_PRINT(state->debug, "[WARNING] cap.read failed\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                state->frame_mutex.unlock();
                continue;
            }
           
            // stopwatch(state, before_lock, "the long thing");
            int ready = poll(&pfd, 1, 10);

            if (ready > 0)
            {
                PipeMsg in_msg;
                read(c2p_pipe[0], &in_msg, sizeof(PipeMsg));
                switch (in_msg.type)
                {
                case BG_BRIGHTNESS:
                    DEBUG_PRINT(state->debug, "RECIEVED BRIGHTNESS: %f\n", in_msg.floatVal);
                    Frame_Processor.resized_brightness = in_msg.floatVal;
                    break;
                case BG_SATURATION:
                    DEBUG_PRINT(state->debug, "RECEIVED BG SATURATION: %f\n", in_msg.floatVal);
                    Frame_Processor.resized_saturation = in_msg.floatVal;
                    break;

                case FG_BRIGHTNESS:
                    DEBUG_PRINT(state->debug, "RECEIVED FG BRIGHTNESS: %f\n", in_msg.floatVal);
                    Frame_Processor.bright_brightness = in_msg.floatVal;
                    break;

                case FG_SATURATION:
                    DEBUG_PRINT(state->debug, "RECEIVED FG SATURATION: %f\n", in_msg.floatVal);
                    Frame_Processor.bright_saturation = in_msg.floatVal;
                    break;
                case RESET_PAIR:
                    DEBUG_PRINT(state->debug, "RESET PAIR");
                    reset_color_pairs();
                    std::fill(std::begin(colorCache), std::end(colorCache), 0);
                    break;
                case COLOR_OFFSET:
                    state->roff = in_msg.colorVal[0];
                    state->goff = in_msg.colorVal[1];
                    state->boff = in_msg.colorVal[2];
                    break;
                case EXPOSURE:
                    cap.set(cv::CAP_PROP_EXPOSURE, in_msg.floatVal);
                break;
                }
            }
            state->frame_mutex.lock();
            Frame_Processor.process_frame(frame);
            // Frame_Processor.show();j

            state->frame_mutex.unlock();

            // stopwatch(debug, before_lock, "Locking and unlocking mutex from main thread");
            // DEBUG_PRINT(state->debug, "MAIN THREAD UNLOCKED MUTEX\n");
            if (!ran_thread)
            {
                ran_thread = 1;
                render_thread = std::thread(n_render, state);
            }

            if (Signals::resized)
            {
                state->cols_rows_mutex.lock();
                DEBUG_PRINT(state->debug, "[WARNING] RESIZED\n");
                state->aspect_ratio = static_cast<float>(state->cols / 2) / state->rows;
                Signals::resized = 0;
                endwin();
                refresh();
                getmaxyx(stdscr, state->rows, state->cols);
                state->cols_rows_mutex.unlock();
            }
            // stopwatch(debug, SOMETHING, "SOME THING IN THE MAIN THREAD SLOWING EVERYTHING");
        }

        DEBUG_PRINT(state->debug, "\nExited safely\n");
        kill(child_pid, SIGINT);
        cv::destroyAllWindows();
        fclose(state->debug);
        endwin();
    }
    return 0;
}