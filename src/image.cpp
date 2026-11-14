#include <opencv2/opencv.hpp>

#include "image.hpp"

void FrameProcessor::show()
{
    cv::Mat display;
    cv::resize(bright_frame, display, cv::Size(500 * state->aspect_ratio, 500));
    cv::resize(resizedFrame, resizedFrame, cv::Size(500 * state->aspect_ratio, 500));
    cv::imshow("Bright_gray", display);
    cv::imshow("original", resizedFrame);

    cv::waitKey(1);
}

void FrameProcessor::process_frame(cv::Mat frame)
{
    cv::Mat hsv, hsv_bright;
    cv::resize(frame, resizedFrame, cv::Size(state->cols / 2, state->rows));

    cv::cvtColor(resizedFrame, gray, cv::COLOR_BGR2GRAY);

    cv::cvtColor(resizedFrame, hsv, cv::COLOR_BGR2HSV);
    hsv_bright = hsv.clone();
    std::vector<cv::Mat> ch;
    cv::split(hsv_bright, ch);
    ch[1].convertTo(ch[1], -1, bright_saturation, 0);
    ch[2].convertTo(ch[2], -1, bright_brightness, 0);
    cv::merge(ch, hsv_bright);
    cv::cvtColor(hsv_bright, bright_frame, cv::COLOR_HSV2BGR);
    // auto pixel = bright_frame.at<cv::Vec3b>(0, 0);
    // DEBUG_PRINT(state->debug, "[PROCESS FRAME] Color at 0, 0: (%d, %d, %d)\n", pixel[2], pixel[1], pixel[0]);
    cv::split(hsv, ch);
    ch[1].convertTo(ch[1], -1, resized_saturation, 0); // Saturation
    ch[2].convertTo(ch[2], -1, resized_brightness, 0); // Brightness
    cv::merge(ch, hsv);
    cv::cvtColor(hsv, resizedFrame, cv::COLOR_HSV2BGR);

    state->n_gray_frame = gray.clone();
    state->n_frame = resizedFrame.clone();
    state->n_bright_frame = bright_frame.clone();
    state->frame_count++;

    // DEBUG_PRINT(state->debug, "[IMAGE CLASS] [[%d]] Frame count: %d, current frame: %d\n", time(nullptr), state->frame_count, state->current_frame);
}
