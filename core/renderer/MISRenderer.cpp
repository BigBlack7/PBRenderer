#include "MISRenderer.hpp"
#include "utils/frame.hpp"
#include "utils/rng.hpp"
#include "sequence/sobolSampler.hpp"

namespace pbrt
{
    float PowerHeuristic(float pdf_j, float pdf_k)
    {
        // return (pdf_j * pdf_j) / (pdf_j * pdf_j + pdf_k * pdf_k);
        if (pdf_j <= 0.f && pdf_k <= 0.f)
            return 0.f;
        if (pdf_j <= 0.f)
            return 0.f;
        if (pdf_k <= 0.f)
            return 1.f;

        float pdf_j_sq = pdf_j * pdf_j;
        float pdf_k_sq = pdf_k * pdf_k;
        return pdf_j_sq / (pdf_j_sq + pdf_k_sq + 1e-8f); // 添加小常数防止除零
    }

    glm::vec3 MISRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        // thread_local RNG rng{};
        // rng.SetSeed(static_cast<size_t>(pixel_coord.x + pixel_coord.y * 10000 + pixel_coord.z * 10000 * 10000));
        thread_local SobolSampler sobol;
        sobol.StartPixelSample(glm::ivec2(pixel_coord.x, pixel_coord.y), pixel_coord.z);
        auto ray = mCamera.GenerateRay(pixel_coord, sobol.Get2D());

        glm::vec3 beta = {1.f, 1.f, 1.f};
        glm::vec3 radiance = {0.f, 0.f, 0.f};
        bool last_is_delta = true;
        float last_bsdf_pdf = 0.f;
        float eta_scale = 1.f;
        bool MISC = true;
        const LightSampler &light_sampler = mScene.GetLightSampler(MISC);

        // Path Regularization
        bool is_regularized = false;
        bool anyNonSpecularBounces = false;

        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if (hit_info.has_value())
            {
                if (hit_info->__material__ && hit_info->__material__->mAreaLight)
                {
                    float bsdf_weight = 1.f;
                    if (!last_is_delta)
                    {
                        float light_sample_prob = light_sampler.GetProb(hit_info->__material__->mAreaLight);
                        float light_pdf = hit_info->__material__->mAreaLight->PDF(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__, MISC);
                        float bsdf_weight = PowerHeuristic(last_bsdf_pdf, light_sample_prob * light_pdf);
                    }
                    radiance += bsdf_weight * beta * hit_info->__material__->mAreaLight->GetRadiance(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__);
                }

                glm::vec3 beta_q = beta * eta_scale;
                float q = glm::max(beta_q.r, glm::max(beta_q.g, beta_q.b));
                q = glm::min(q, 0.9f);
                if (q < 1.f)
                {
                    if (sobol.Get1D() > q)
                    {
                        break;
                    }
                    beta /= q;
                }

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
                        anyNonSpecularBounces = true;
                        auto light_sample_info = light_sampler.SampleLight(sobol.Get1D());
                        if (light_sample_info.has_value())
                        {
                            auto light_info = light_sample_info->__light__->SampleLight(hit_info->__hitPoint__, mScene.GetRadius(), sobol, MISC);
                            if (light_info.has_value() && (!mScene.Intersect({hit_info->__hitPoint__, light_info->__lightPoint__ - hit_info->__hitPoint__}, 1e-5, 1.f - 1e-5)))
                            {
                                glm::vec3 light_dir_local = frame.LocalFromWorld(light_info->__direction__);

                                if (is_regularized && anyNonSpecularBounces)
                                {
                                    hit_info->__material__->Regularize();
                                }

                                float bsdf_pdf = hit_info->__material__->PDF(hit_info->__hitPoint__, light_dir_local, view_dir);
                                float light_weight = PowerHeuristic(light_info->__pdf__ * light_sample_info->__prob__, bsdf_pdf);
                                radiance += light_weight * beta * hit_info->__material__->BSDF(hit_info->__hitPoint__, light_dir_local, view_dir) * glm::abs(light_dir_local.y) * light_info->__Le__ / (light_info->__pdf__ * light_sample_info->__prob__);
                            }
                        }
                    }

                    if (is_regularized && anyNonSpecularBounces)
                    {
                        hit_info->__material__->Regularize();
                    }
                    auto bsdf_info = hit_info->__material__->SampleBSDF(hit_info->__hitPoint__, view_dir, sobol);

                    if (!bsdf_info.has_value())
                    {
                        break;
                    }
                    last_bsdf_pdf = bsdf_info->__pdf__;
                    eta_scale *= bsdf_info->__etaScale__;
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
                glm::vec3 light_dir_delta = glm::normalize(ray.__direction__);
                glm::vec3 light_point = ray.__origin__ + mScene.GetRadius() * 2.f * light_dir_delta;
                if (last_is_delta)
                {
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        radiance += beta * light->GetRadiance(ray.__origin__, light_point, -light_dir_delta);
                    }
                }
                else
                {
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        float light_sample_prob = light_sampler.GetProb(light);
                        float light_pdf = light->PDF(ray.__origin__, light_point, -light_dir_delta, MISC);
                        float bsdf_weight = PowerHeuristic(last_bsdf_pdf, light_sample_prob * light_pdf);
                        radiance += bsdf_weight * beta * light->GetRadiance(ray.__origin__, light_point, -light_dir_delta);
                    }
                }

                break;
            }
        }

        return radiance;
    }
}