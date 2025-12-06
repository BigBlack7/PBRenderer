#pragma once

#include <chrono>
#include <string>

namespace pt
{
#define PROFILE(name) Profile __profile__(name);

    struct Profile
    {
    public:
        std::string __name__;
        std::chrono::high_resolution_clock::time_point __start__;

    public:
        Profile(const std::string &name);
        ~Profile();
    };
}
