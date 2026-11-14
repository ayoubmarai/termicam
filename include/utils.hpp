#pragma once
#include <signal.h>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <condition_variable>

#define DEBUG_PRINT(debug, ...)      \
    do                               \
    {                                \
        fprintf(debug, __VA_ARGS__); \
        fflush(debug);               \
    } while (0)

enum MessageType {
    BG_BRIGHTNESS,
    BG_SATURATION,
    BG_HUE,
    FG_BRIGHTNESS,
    FG_SATURATION,
    COLOR_OFFSET,
    FG_HUE,
    DELTA,
    LOG,
    RESET_PAIR,
    EXPOSURE
};

struct PipeMsg {
    MessageType type;
    size_t size;

    union {
        int intVal;
        float floatVal;
        char strVal[256];
        float colorVal[3];
    };
};



struct SharedState
{
    float roff, goff, boff;
    int cols, rows;
    std::atomic<int> frame_count{0};
    std::atomic<int> current_frame{0};

    float aspect_ratio;
    float delta;

    FILE *debug;

    std::mutex frame_mutex;
    std::mutex cols_rows_mutex;
    std::condition_variable frame_cv;
    cv::Mat n_bright_frame, n_frame, n_gray_frame;
};

typedef std::chrono::steady_clock chrono_clock;

namespace Signals
{
    extern volatile sig_atomic_t running;
    extern volatile sig_atomic_t resized;

    void signal_handler(int sig);
}

void stopwatch(SharedState* state, std::chrono::_V2::steady_clock::time_point start, const char *msg);
bool is_process_running(pid_t process);
const char* timestamp();