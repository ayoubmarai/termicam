#include <opencv2/opencv.hpp>
#include <iostream>
#include <signal.h>
#include <ncurses.h>
#include <chrono>
#include <thread>

volatile sig_atomic_t running = 1;
volatile sig_atomic_t resized = 0;

const char *shades[] = { // empty
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

void debug_print(FILE *debug, const char *msg)
{
    fprintf(debug, "%s\n", msg);
    fflush(debug);
}

typedef std::chrono::steady_clock chrono_clock;

int main()
{

    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGWINCH, signal_handler);
    FILE *debug = fopen("./output.log", "w");

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
    cv::Mat resizedFrame, gray, display;
    fprintf(debug, "COLS=%d LINES=%d\n", COLS, LINES);
    fflush(debug);
    int rows, cols;
    while (running)
    {
        getmaxyx(stdscr, rows, cols);
        cap.read(frame);

        if (!cap.read(frame))
        {
            debug_print(debug, "cap.read failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        float aspect_ratio = static_cast<float>(cols / 2) / rows;

        cv::resize(frame, resizedFrame, cv::Size(cols / 2, rows));

        fprintf(debug, "cols=%d rows=%d aspect ratio=%f\n", cols / 2, rows, aspect_ratio);
        fflush(debug);

        float delta = std::chrono::duration<float>(chrono_clock::now() - prevFrame).count();
        prevFrame = chrono_clock::now();
        cv::cvtColor(resizedFrame, gray, cv::COLOR_BGR2GRAY);
        refresh();

        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols / 2; x++)
            {
                uchar pixel = gray.at<uchar>(y, x);
                constexpr int N = sizeof(shades) / sizeof(shades[0]);
                const char *shade = shades[pixel * N / 256];
                mvaddstr(y, x * 2, shade);
                mvaddstr(y, x * 2 + 1, shade);
                fprintf(debug, "x=%d, y=%d, pixel=%d i=%d shade=%s", x, y, pixel, pixel / 64, shades[pixel / 64]);
                fflush(debug);
            }
        }

        cv::resize(gray, display, cv::Size(gray.size().width * 5, gray.size().height * 5));

        cv::imshow("Video", display);
        if (resized)
        {
            fprintf(debug, "RESIZED\n", cols, rows);
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