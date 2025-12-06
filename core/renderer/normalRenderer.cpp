#include "normalRenderer.hpp"

namespace pt
{
    glm::vec3 NormalRenderer::RenderPixel(const glm::ivec2 &pixel_coord)
    {
        auto ray = mCamera.GenerateRay(pixel_coord);
        auto hit_info = mScene.Intersect(ray);
        if (hit_info.has_value())
        {
            return hit_info->__normal__ * 0.5f + 0.5f;
        }
        return {};
    }
}