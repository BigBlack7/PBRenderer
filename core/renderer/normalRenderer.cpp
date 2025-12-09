#include "normalRenderer.hpp"
#include "utils/rgb.hpp"

namespace pbrt
{
    glm::vec3 NormalRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        auto ray = mCamera.GenerateRay(pixel_coord);
        auto hit_info = mScene.Intersect(ray);
        if (hit_info.has_value())
        {
            glm::ivec3 color = (hit_info->__normal__ * 0.5f + 0.5f) * 255.f;
            return RGB(color.x, color.y, color.z);
        }
        return {};
    }
}