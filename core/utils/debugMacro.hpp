#pragma once

namespace pt
{
#ifdef WITH_DEBUG_INFO
#define DEBUG_INFO(...) __VA_ARGS__;
#else
#define DEBUG_INFO(...)
#endif
}