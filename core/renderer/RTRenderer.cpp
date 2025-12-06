#include "RTRenderer.hpp"
#include "utils/frame.hpp"

namespace pt
{
    glm::vec3 RTRenderer::RenderPixel(const glm::ivec2 &pixel_coord)
    {
        auto ray = mCamera.GenerateRay(pixel_coord, {glm::abs(mRng.Uniform()), glm::abs(mRng.Uniform())});
        glm::vec3 beta = {1.f, 1.f, 1.f};
        glm::vec3 color = {0.f, 0.f, 0.f};
        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if(hit_info.has_value())
            {
                color += beta * hit_info->__material__->mEmission;
                beta *= hit_info->__material__->mAlbedo;
                ray.__origin__ = hit_info->__hitPoint__;
                Frame frame(hit_info->__normal__);
                glm::vec3 light_dir;
                if (hit_info->__material__->mIsSpecular)
                {
                    glm::vec3 view_dir = frame.LocalFromWorld(-ray.__direction__);
                    light_dir = {-view_dir.x, view_dir.y, -view_dir.z};
                }
                else
                {
                    do
                    {
                        light_dir = {mRng.Uniform(), mRng.Uniform(), mRng.Uniform()};
                        light_dir = light_dir * 2.f - 1.f;
                    } while (glm::length(light_dir) > 1.f);
                    if (light_dir.y < 0.f)
                    {
                        light_dir.y = -light_dir.y;
                    }
                }
                ray.__direction__ = frame.WorldFromLocal(light_dir);
            }
            else
            {
                break;
            }
        }
        return color;
    }
}