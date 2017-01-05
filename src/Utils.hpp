#ifndef sonar_processing_Utilities_Hpp
#define sonar_processing_Utilities_Hpp

#include <cstdio>
#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <opencv2/opencv.hpp>

namespace sonar_processing {

namespace utils {

inline cv::Rect clip_rect(const cv::Point& tl, const cv::Point& br, const cv::Point& min_tl, const cv::Point& max_br) {
    return cv::Rect(cv::Point(std::max(min_tl.x, tl.x), std::max(min_tl.y, tl.y)), 
                    cv::Point(std::min(br.x, max_br.x), std::min(br.y, max_br.y)));
}

inline cv::Rect neighborhood_rect(int x, int y, int n, int w, int h) {
    return clip_rect(cv::Point(x-n, y-n), cv::Point(x+n, y+n), cv::Point(0, 0), cv::Point(w-1, h-1));
}

inline double clip(double val, double min, double max) {
    return (val < min) ? min : ((val > max) ? max : val);
}

inline uint32_t border_fit(uint32_t x,  uint32_t total_size, uint32_t block_size) {
    return (total_size > x + block_size) ? x : total_size - block_size - 1;
}

inline void point2f_to_point2i(const std::vector<cv::Point2f>& pts2f, std::vector<cv::Point>& pts2i) {
    pts2i.assign(pts2f.size(), cv::Point(-1, -1));
    for (size_t k = 0; k < pts2f.size(); k++) {
        pts2i[k] = cv::Point2i((int)roundf(pts2f[k].x), (int)roundf(pts2f[k].y));
    }
}

template <typename T>
void accumulative_sum(const std::vector<T>& src, std::vector<T>& dst) {
    dst.resize(src.size());
    dst[0] = src[0];
    for (size_t i = 1; i < src.size(); i++) dst[i] = dst[i-1] + src[i];
}

namespace now {

inline static uint64_t microseconds() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}

inline static uint64_t milliseconds() {
    return microseconds() / 1000;    
}

} /* namespace now */

} /* end of namespace utils */

} /* end of namespace sonar_processing */

#endif /* end of include guard:  */
