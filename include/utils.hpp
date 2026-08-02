#pragma once
#include <signal.h>
#include <mutex>
#include <opencv2/opencv.hpp>
#define DEBUG_PRINT(debug, ...)      \
    do                               \
    {                                \
        fprintf(debug, __VA_ARGS__); \
        fflush(debug);               \
    } while (0)

struct SharedState {
    FILE* debug;
    std::mutex frame_mutex;
    std::mutex cols_rows_mutex;
    int frame_count = 0, current_frame = 0;
    cv::Mat n_bright_frame, n_frame, n_gray_frame;
};

typedef std::chrono::steady_clock chrono_clock;

namespace Signals
{
    extern volatile sig_atomic_t running;
    extern volatile sig_atomic_t resized;

    void signal_handler(int sig);
}