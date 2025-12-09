#include "PTRenderer.hpp"
#include "utils/frame.hpp"
#include "utils/rng.hpp"

namespace pbrt
{
    glm::vec3 PTRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        thread_local RNG rng{};
        rng.SetSeed(static_cast<size_t>(pixel_coord.x + pixel_coord.y * 10000 + pixel_coord.z * 10000 * 10000));
        auto ray = mCamera.GenerateRay(pixel_coord, {rng.Uniform(), rng.Uniform()});
        glm::vec3 beta = {1.f, 1.f, 1.f};
        glm::vec3 radiance = {0.f, 0.f, 0.f};
        float q = 0.9f;
        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if (hit_info.has_value())
            {
                radiance += beta * hit_info->__material__->mEmission;
                if (rng.Uniform() > q)
                {
                    break;
                }
                beta /= q;
                Frame frame(hit_info->__normal__);
                glm::vec3 light_dir;

                if (hit_info->__material__)
                {
                    glm::vec3 view_dir = frame.LocalFromWorld(-ray.__direction__);

                    if (view_dir.y == 0)
                    {
                        ray.__origin__ = hit_info->__hitPoint__;
                        continue;
                    }

                    auto bsdf_sample = hit_info->__material__->SampleBSDF(hit_info->__hitPoint__, view_dir, rng);

                    if (!bsdf_sample.has_value())
                        break;

                    beta *= bsdf_sample->__bsdf__ * glm::abs(bsdf_sample->__lightDirection__.y) / bsdf_sample->__pdf__;
                    light_dir = bsdf_sample->__lightDirection__;
                }
                else
                {
                    break;
                }

                ray.__origin__ = hit_info->__hitPoint__;
                ray.__direction__ = frame.WorldFromLocal(light_dir);
            }
            else
            {
                break;
            }
        }

        return radiance;
    }
}