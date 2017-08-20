#include "Clustering.hpp"
#include "Denoising.hpp"
#include "FrequencyDomain.hpp"
#include "ImageFiltering.hpp"
#include "ImageUtil.hpp"
#include "Preprocessing.hpp"
#include "ROI.hpp"
#include "SonarHolder.hpp"
#include "SonarImagePreprocessing.hpp"

namespace sonar_processing {

SonarImagePreprocessing::SonarImagePreprocessing()
    : mean_filter_ksize_(7)
    , mean_difference_filter_ksize_(25)
    , median_blur_filter_ksize_(5)
{
}

SonarImagePreprocessing::~SonarImagePreprocessing() {
}

void SonarImagePreprocessing::ExtractROI(
    const cv::Mat& source_image,
    const cv::Mat& source_mask,
    cv::Mat& roi_cart,
    uint32_t& roi_line,
    float alpha,
    int start_row,
    int end_row) const
{
    if (end_row<0) end_row=source_image.rows;

    // calculate the proportional mean of each image row
    std::vector<float> row_mean(end_row, 0);
    for (size_t i = start_row; i <= end_row; i++) {
        int r = source_image.rows-i-1;
        double value = cv::sum(source_image.row(r))[0] / cv::countNonZero(source_mask.row(r));
        row_mean[i] = std::isnan(value) ? 0 : value;
    }

    // accumulative sum
    std::vector<float> accum_sum(row_mean.size(), 0);
    std::partial_sum(row_mean.begin(), row_mean.end(), accum_sum.begin());

    // threshold
    float min = *std::min_element(accum_sum.begin(), accum_sum.end());
    float max = *std::max_element(accum_sum.begin(), accum_sum.end());
    float thresh = alpha * (max - min) + min;
    std::replace_if(accum_sum.begin(), accum_sum.end(), std::bind2nd(std::less<float>(), thresh), 0.0);

    // generate new cartesian mask
    std::vector<float>::iterator pos = std::find_if (accum_sum.begin(), accum_sum.end(), std::bind2nd(std::greater<float>(), 0));
    uint32_t new_y = std::distance(accum_sum.begin(), pos) + 1;
    roi_line = source_mask.rows-new_y;
    source_mask(cv::Rect(0, source_mask.rows - new_y, source_mask.cols, new_y)).setTo(cv::Scalar(0));
    source_mask.copyTo(roi_cart);
}


void SonarImagePreprocessing::Apply(
    const SonarHolder& sonar_holder,
    cv::Mat& preprocessed_image,
    cv::Mat& result_mask,
    float scale_factor) const
{
    Apply(
        sonar_holder.cart_image(),
        sonar_holder.cart_image_mask(),
        preprocessed_image,
        result_mask,
        scale_factor);
}

void SonarImagePreprocessing::Apply(
    const cv::Mat& source_image,
    const cv::Mat& source_mask,
    cv::Mat& preprocessed_image,
    cv::Mat& result_mask,
    float scale_factor) const
{
    cv::Mat roi_cart;
    uint32_t roi_line;
    ExtractROI(source_image, source_mask, roi_cart, roi_line, 0.005, 30, source_image.rows-1);
    PerformPreprocessing(source_image, roi_cart, preprocessed_image, result_mask, scale_factor, roi_line);
}

void SonarImagePreprocessing::PerformPreprocessing(
    const cv::Mat& source_cart_image,
    const cv::Mat& source_cart_mask,
    cv::Mat& preprocessed_image,
    cv::Mat& result_mask,
    float scale_factor,
    int start_cart_line) const
{

    cv::Mat cart_image = source_cart_image;
    cv::Mat cart_mask = source_cart_mask;

    if (scale_factor != 1.0) {
        cv::Size new_size = cv::Size(cart_image.size().width*scale_factor, cart_image.size().height*scale_factor);
        cv::resize(cart_image, cart_image, new_size);
        cv::resize(cart_mask, cart_mask, new_size);
    }

    cv::Mat cart_image_8u;
    cart_image.convertTo(cart_image_8u, CV_8U, 255);

    // apply insonification correction
    cv::Mat enhanced;
    image_filtering::insonification_correction(cart_image, cart_mask, enhanced);

    // image denoising
    cv::Mat denoised;
    image_filtering::mean_filter(enhanced, denoised, mean_filter_ksize_, cart_mask);

    // apply border filter
    cv::Mat border, denoised_8u;
    denoised.convertTo(denoised_8u, CV_8U, 255.0);
    image_filtering::border_filter(denoised_8u, border);

    // reduce mask size
    image_util::erode(cart_mask, cart_mask, cv::Size(15, 15), 2);
    cv::threshold(cart_mask, cart_mask, 128, 255, CV_THRESH_BINARY);

    // apply cartesian mask
    image_util::apply_mask(border, border, cart_mask);
    border.convertTo(border, CV_32F, 1.0/255.0);
    cv::normalize(border, border, 0, 1, cv::NORM_MINMAX, CV_32FC1, cart_mask);

    // mean difference filter
    cv::Mat mean_diff;
    image_filtering::mean_difference_filter(enhanced, border, mean_diff, mean_difference_filter_ksize_, cart_mask);

    // apply median filter
    mean_diff.convertTo(mean_diff, CV_8U, 255.0);
    cv::medianBlur(mean_diff, mean_diff, median_blur_filter_ksize_);
    mean_diff.convertTo(mean_diff, CV_32F, 1.0/255.0);

    preprocessed_image = cv::Mat::zeros(mean_diff.size(), mean_diff.type());
    cv::normalize(mean_diff, preprocessed_image, 0, 1, cv::NORM_MINMAX, CV_32FC1, cart_mask);

    cart_mask.copyTo(result_mask);

    if (scale_factor != 1.0) {
        cv::resize(preprocessed_image, preprocessed_image, source_cart_image.size());
        cv::resize(result_mask, result_mask, source_cart_image.size());
    }
}


} /* namespace sonar_processing */
