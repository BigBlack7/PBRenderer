#pragma once
#include <cstdint>

namespace pbrt
{
    // Sobol Matrix
    static constexpr int NSobolDimensions = 1024;
    static constexpr int SobolMatrixSize = 52;
    extern const uint32_t SobolMatrices32[NSobolDimensions * SobolMatrixSize];

    extern const uint64_t VdCSobolMatrices[][SobolMatrixSize];
    extern const uint64_t VdCSobolMatricesInv[][SobolMatrixSize];
}