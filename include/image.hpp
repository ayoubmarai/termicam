#pragma once
#include <opencv2/opencv.hpp>
#include "utils.hpp"

class FrameProcessor
{

    cv::Mat process_frame(cv::Mat frame, SharedState *state);

private:
    cv::Mat resizedFrame;
    cv::Mat gray;
    cv::Mat display;
    cv::Mat bright_frame;
    cv::Mat hsv_frame;
};