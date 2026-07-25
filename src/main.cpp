#include <opencv2/opencv.hpp>
#include <iostream>
#include <signal.h>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <math.h>
#include <ctime>

#define DO_COLOR

volatile sig_atomic_t running = 1;
volatile sig_atomic_t resized = 0;
typedef std::chrono::steady_clock chrono_clock;

void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGWINCH:
        resized = 1;
        break;
    case SIGINT:
        running = 0;
        break;
    default:
        break;
    }
}

int to256(int r, int g, int b)
{
    int R, G, B;
    R = (r * 6) / 256;
    G = (g * 6) / 256;
    B = (b * 6) / 256;

    return 16 + (36 * R) + (6 * G) + B;
}

short colorCache[256] = {0};

std::string shades[] = {
    ".",
    "⠆",
    "⠇",
    "|",
    "⠧",
    "@",
    "⠷",
    "&",
    "⠿",
    "#",
    "░",
    "▒",
    "▓",
    "█"};

void debug_print(FILE *debug, const char *msg)
{
    fprintf(debug, "%s\n", msg);
    fflush(debug);
}

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

std::mutex frame_mutex;
cv::Mat resizedFrame, gray, display, bright_frame;

void n_render()
{
    
}

int main()
{

    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGWINCH, signal_handler);
    FILE *debug = fopen("./output.log", "w");
    fprintf(debug, "[%d] running", time(nullptr));
    fflush(debug);
    cv::VideoCapture cap(0);
    initscr();
    start_color();
    noecho();
    keypad(stdscr, true);

    if (!cap.isOpened())
    {
        std::cerr << "Failed to open video\n";
        return 1;
    }
    cv::Mat frame;
    double fps = cap.get(cv::CAP_PROP_FPS);

    auto prevFrame = chrono_clock::now();

    fprintf(debug, "COLS=%d LINES=%d\n", COLS, LINES);
    fflush(debug);
    int rows, cols;
    while (running)
    {
        getmaxyx(stdscr, rows, cols);
        cap.read(frame);

        if (!cap.read(frame))
        {
            debug_print(debug, "[WARNING] cap.read failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        float aspect_ratio = static_cast<float>(cols / 2) / rows;

        cv::resize(frame, resizedFrame, cv::Size(cols / 2, rows));

        float delta = std::chrono::duration<float>(chrono_clock::now() - prevFrame).count();
        prevFrame = chrono_clock::now();
        cv::cvtColor(resizedFrame, gray, cv::COLOR_BGR2GRAY);
        resizedFrame.convertTo(bright_frame, -1, 1.0, 50.0);
        refresh();

        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols / 2; x++)
            {
                cv::Vec3b pixel = resizedFrame.at<cv::Vec3b>(y, x);
                cv::Vec3b bright_pixel = bright_frame.at<cv::Vec3b>(y, x);
                uchar grayPixel = gray.at<uchar>(y, x);
                int color = to256(pixel[2], pixel[1], pixel[0]);
                int fg_color = to256(bright_pixel[2], bright_pixel[1], bright_pixel[0]);
                if (colorCache[color] == 0)
                {
                    init_pair(color + 1, fg_color, color);
                    colorCache[color] = 1;
                    fprintf(debug, "Color: %d\n", color);
                    fflush(debug);
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
                mvaddstr(y, x * 2, (shade + shade).c_str());

#ifdef DO_COLOR
                attroff(COLOR_PAIR(color + 1));
#endif
            }
        }

        cv::resize(bright_frame, display, cv::Size(resizedFrame.size().width * 5, resizedFrame.size().height * 5));
        cv::resize(resizedFrame, resizedFrame, cv::Size(resizedFrame.size().width * 5, resizedFrame.size().height * 5));

        cv::imshow("Bright_gray", display);
        cv::imshow("original", resizedFrame);
        if (resized)
        {
            fprintf(debug, "[WARNING] RESIZED\n", cols, rows);
            fflush(debug);
            resized = 0;
            endwin();
            refresh();
        }

        if (cv::waitKey(1) == 27)
            break;
    }

    debug_print(debug, "\nExited safely\n");
    cv::destroyAllWindows();
    fclose(debug);
    endwin();
    return 0;
}