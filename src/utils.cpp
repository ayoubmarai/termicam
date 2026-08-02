#include <opencv2/opencv.hpp>
#include <mutex>
#include <iostream>
#include <signal.h>
#include <chrono>
#include <ctime>

#include "utils.hpp"


namespace Signals
{
    volatile sig_atomic_t running = 1;
    volatile sig_atomic_t resized = 0;

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
}

void stopwatch(SharedState* state, std::chrono::_V2::steady_clock::time_point start, const char *msg)
{
    float duration = std::chrono::duration<float>(chrono_clock::now() - start).count();
    DEBUG_PRINT(state->debug, "%s took %fs\n", msg, duration);
}