#include "debugRenderer.hpp"
#include "utils/rgb.hpp"

namespace pbrt
{
        glm::vec3 BTCRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
        {
#ifdef WITH_DEBUG_INFO
                auto ray = mCamera.GenerateRay(pixel_coord);
                mScene.Intersect(ray);
                return RGB::GenerateHeatMap(ray.__boundsTestCount__ / 150.f);
#else
                return {};
#endif
        }

        glm::vec3 TTCRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
        {
#ifdef WITH_DEBUG_INFO
                auto ray = mCamera.GenerateRay(pixel_coord);
                mScene.Intersect(ray);
                return RGB::GenerateHeatMap(ray.__triangleTestCount__ / 7.f);
#else
                return {};
#endif
        }
}
