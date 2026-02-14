#pragma once
#include "MISRenderer.hpp"

namespace pbrt
{
    class BDPTRenderer : public MISRenderer
    {
    public:
        BDPTRenderer(Camera &camera, const Scene &scene) : MISRenderer(camera, scene) {}
    };
}
