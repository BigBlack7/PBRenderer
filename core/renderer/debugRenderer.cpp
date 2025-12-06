#include "debugRenderer.hpp"
#include "utils/rgb.hpp"

namespace pt
{
    glm::vec3 BTCRenderer::RenderPixel(const glm::ivec2 &pixel_coord)
    {
#ifdef WITH_DEBUG_INFO
        auto ray = mCamera.GenerateRay(pixel_coord);
        auto hit_info = mScene.Intersect(ray);
        if(hit_info.has_value())
        {
            return RGB::GenerateHeatMap(hit_info->__boundsTestCount__ / 150.f);
        }
        return {};
#else
        return {};
#endif
    }

    glm::vec3 TTCRenderer::RenderPixel(const glm::ivec2 &pixel_coord)
    {
#ifdef WITH_DEBUG_INFO
        auto ray = mCamera.GenerateRay(pixel_coord);
        auto hit_info = mScene.Intersect(ray);
        if (hit_info.has_value())
        {
            return RGB::GenerateHeatMap(hit_info->__triangleTestCount__ / 7.f);
        }
        return {};
#else
        return {};
#endif
    }

    glm::vec3 BDRenderer::RenderPixel(const glm::ivec2 &pixel_coord)
    {
#ifdef WITH_DEBUG_INFO
        auto ray = mCamera.GenerateRay(pixel_coord);
        auto hit_info = mScene.Intersect(ray);
        if (hit_info.has_value())
        {
            return RGB::GenerateHeatMap(hit_info->__boundsDepth__ / 32.f);
        }
        return {};
#else
        return {};
#endif
    }
}
