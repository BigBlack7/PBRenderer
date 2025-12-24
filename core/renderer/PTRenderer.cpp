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
        bool last_is_delta = true;

        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if (hit_info.has_value())
            {
                if (last_is_delta && hit_info->__material__ && hit_info->__material__->mAreaLight)
                {
                    radiance += beta * hit_info->__material__->mAreaLight->GetRadiance(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__);
                }

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

                    last_is_delta = hit_info->__material__->IsDeltaDistribution();
                    if (!last_is_delta)
                    {
                        auto light_sample_info = mScene.GetLightSampler(false).SampleLight(rng.Uniform());
                        if (light_sample_info.has_value())
                        {
                            auto light_info = light_sample_info->__light__->SampleLight(hit_info->__hitPoint__, mScene.GetRadius(), rng, false);
                            if (light_info.has_value() && (!mScene.Intersect({hit_info->__hitPoint__, light_info->__lightPoint__ - hit_info->__hitPoint__}, 1e-5, 1.f - 1e-5)))
                            {
                                glm::vec3 light_dir_local = frame.LocalFromWorld(light_info->__direction__);
                                radiance += beta * hit_info->__material__->BSDF(hit_info->__hitPoint__, light_dir_local, view_dir) * glm::abs(light_dir_local.y) * light_info->__Le__ / (light_info->__pdf__ * light_sample_info->__prob__);
                            }
                        }
                    }

                    auto bsdf_info = hit_info->__material__->SampleBSDF(hit_info->__hitPoint__, view_dir, rng);

                    if (!bsdf_info.has_value())
                        break;

                    beta *= bsdf_info->__bsdf__ * glm::abs(bsdf_info->__lightDirection__.y) / bsdf_info->__pdf__;
                    light_dir = bsdf_info->__lightDirection__;
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
                if (last_is_delta)
                {
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        glm::vec3 light_dir_delta = glm::normalize(ray.__direction__);
                        radiance += beta * light->GetRadiance(ray.__origin__, ray.__origin__ + mScene.GetRadius() * 2.f * ray.__direction__, -light_dir_delta);
                    }
                }

                break;
            }
        }

        return radiance;
    }
}