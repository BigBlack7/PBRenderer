#include "MISRenderer.hpp"
#include "utils/frame.hpp"
#include "utils/rng.hpp"

namespace pbrt
{
    float PowerHeuristic(float pdf_j, float pdf_k)
    {
        return (pdf_j * pdf_j) / (pdf_j * pdf_j + pdf_k * pdf_k);
    }

    glm::vec3 MISRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        thread_local RNG rng{};
        rng.SetSeed(static_cast<size_t>(pixel_coord.x + pixel_coord.y * 10000 + pixel_coord.z * 10000 * 10000));
        auto ray = mCamera.GenerateRay(pixel_coord, {rng.Uniform(), rng.Uniform()});

        glm::vec3 beta = {1.f, 1.f, 1.f};     // i = 1, β = 1; i > 1, β = Π( f_i * |cosθ_i| / pdf_i )
        glm::vec3 radiance = {0.f, 0.f, 0.f}; // L_o = L_e + ∫(f_i * L_i * |cosθ_i|) = Σ (β * L_e) -> RR -> Σ (β / q_r * L_e)
        bool last_is_delta = true;            // 上一次反射是否为Delta分布(初始认为从相机到像素的第一根光线为Delta分布)
        float last_bsdf_pdf = 0.f;
        float eta_scale = 1.f;
        /*
            多重重要性采样补偿, 如果向光源采样的光源面积非常大,
            那么采样效果不会很好, 此时可以将采样它的概率限制为0, 减小方差
        */
        bool MISC = true;
        const LightSampler &light_sampler = mScene.GetLightSampler(MISC);

        // Path Regularization
        bool is_regularized = false;
        bool anyNonSpecularBounces = false;

        while (true)
        {
            auto hit_info = mScene.Intersect(ray);
            if (hit_info.has_value()) // 与场景相交
            {
                // 与光源相交, 直接对结果产生贡献(如果在RR后会导致光源上有黑色噪点)
                if (hit_info->__material__ && hit_info->__material__->mAreaLight)
                {
                    float bsdf_weight = 1.f;
                    if (!last_is_delta) // 上一次反射不是Delta分布
                    {
                        float light_sample_prob = light_sampler.GetProb(hit_info->__material__->mAreaLight);
                        float light_pdf = hit_info->__material__->mAreaLight->PDF(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__, MISC);
                        float bsdf_weight = PowerHeuristic(last_bsdf_pdf, light_sample_prob * light_pdf);
                    }
                    radiance += bsdf_weight * beta * hit_info->__material__->mAreaLight->GetRadiance(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__);
                }

                // Russian Roulette
                glm::vec3 beta_q = beta * eta_scale;
                float q = glm::max(beta_q.r, glm::max(beta_q.g, beta_q.b)); // 高贡献路径更不容易被结束
                q = glm::min(q, 0.9f); // 防止q过大导致路径追踪死循环(如反照率为1的镜面反射)
                if (q < 1.f)
                {
                    if (rng.Uniform() > q)
                    {
                        break; // 终止该路径
                    }
                    beta /= q;
                }

                Frame frame(hit_info->__normal__);
                glm::vec3 light_dir;

                if (hit_info->__material__) // 在物体局部坐标系处理
                {
                    glm::vec3 view_dir = frame.LocalFromWorld(-ray.__direction__);

                    // 掠射角处理, 光线刚好掠过物体表面
                    if (view_dir.y == 0)
                    {
                        ray.__origin__ = hit_info->__hitPoint__;
                        continue;
                    }

                    last_is_delta = hit_info->__material__->IsDeltaDistribution();
                    if (!last_is_delta) // 非Delta分布, 向光源采样
                    {
                        auto light_sample_info = light_sampler.Sample(rng.Uniform()); // 采样光源及其概率
                        if (light_sample_info.has_value())
                        {
                            auto light_info = light_sample_info->__light__->SampleLight(hit_info->__hitPoint__, mScene.GetRadius(), rng, MISC); // 光源具体信息
                            /*
                                可见性测试, 判断从表面点到光源之间是否有遮挡
                                最小距离防止光线与发出该光线的表面自身相交
                                最大距离防止光线与目标光源本身相交
                            */
                            if (light_info.has_value() && (!mScene.Intersect(Ray{hit_info->__hitPoint__, light_info->__lightPoint__ - hit_info->__hitPoint__}, 1e-5, 1.f - 1e-5)))
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

                    // PBRT-v4 Light Transport Ⅰ Path Regularization
                    if (is_regularized && anyNonSpecularBounces)
                    {
                        hit_info->__material__->Regularize();
                    }

                    auto bsdf_info = hit_info->__material__->SampleBSDF(hit_info->__hitPoint__, view_dir, rng);
                    if (!bsdf_info.has_value())
                    {
                        break;
                    }
                    if (!hit_info->__material__->IsDeltaDistribution())
                    {
                        anyNonSpecularBounces = true;
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
                if (last_is_delta) // 上一次反射为Delta分布且当前未命中任何物体, 处理无限光源
                {
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        radiance += beta * light->GetRadiance(ray.__origin__, light_point, -light_dir_delta);
                    }
                }
                else // 非Delta分布且当前未命中任何物体, 处理无限光源
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