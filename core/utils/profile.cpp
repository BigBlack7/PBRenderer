#include "profile.hpp"
#include "utils/logger.hpp"

namespace pbrt
{
    Profile::Profile(const std::string &name) : __name__(name), __start__(std::chrono::high_resolution_clock::now()) {}

    Profile::~Profile()
    {
        auto duration = std::chrono::high_resolution_clock::now() - __start__;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        PBRT_DEBUG("Profile {} cost {} ms", __name__, ms);
    }
}