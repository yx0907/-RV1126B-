#include <opencv2/opencv.hpp>
#include "normal_to_binary.h"

int normal_to_binary(const float * input_xyz, int width, int height, unsigned char * output)
{
    try {
        cv::Mat input(height, width, CV_32FC3, (void *)input_xyz);

        std::vector<cv::Mat> channels(3);
        cv::split(input, channels);
        cv::Mat dx = channels[0];
        cv::Mat dy = channels[1];
        cv::Mat nz = channels[2];

        cv::Mat dz = 1.0f - nz;
        cv::Mat grad_map;
        cv::magnitude(dx, dy, grad_map);
        cv::Mat merged_float = 0.4f * grad_map + 0.6f * dz;

        cv::Mat rough_8u;
        cv::Mat normed;
        cv::normalize(merged_float, normed, 0, 255, cv::NORM_MINMAX);
        normed.convertTo(rough_8u, CV_8UC1);

        cv::Mat lap;
        cv::Laplacian(rough_8u, lap, CV_64F, 1);
        cv::Mat lap_abs;
        cv::convertScaleAbs(lap, lap_abs);
        cv::Mat enhanced;
        cv::convertScaleAbs(lap_abs, enhanced, 2.5, 0.0);

        cv::Mat sharpened;
        cv::addWeighted(enhanced, 1.0, rough_8u, 1.0, 0.0, sharpened);

        cv::bitwise_not(sharpened, sharpened);

        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        cv::Mat final_img;
        clahe->apply(sharpened, final_img);

        std::memcpy(output, final_img.data, width * height);
        return 0;
    } catch (...) {
        return -1;
    }
}
