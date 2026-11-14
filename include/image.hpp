#pragma once
#include <opencv2/opencv.hpp>
#include "utils.hpp"

class FrameProcessor
{
public:
    void process_frame(cv::Mat frame);
    void show();
    void clearCache();

    SharedState *state;
    float bright_brightness = 1.2;
    float bright_saturation = .7;

    float resized_brightness = 1;
    float resized_saturation = 1.0;

private:
    cv::Mat resizedFrame;
    cv::Mat gray;
    cv::Mat bright_frame;
    cv::Mat hsv_frame;
};