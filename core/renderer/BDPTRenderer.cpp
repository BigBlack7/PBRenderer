#include "BDPTRenderer.hpp"
#include "utils/frame.hpp"
#include "utils/rng.hpp"
#include "sampler/spherical.hpp"
#include "light/areaLight.hpp"
#include "light/infiniteLight.hpp"

namespace pbrt
{
    namespace
    {
        float PowerHeuristic(float pdf_a, float pdf_b)
        {
            float a2 = pdf_a * pdf_a;
            float b2 = pdf_b * pdf_b;
            if (a2 + b2 == 0.f)
            {
                return 0.f;
            }
            return a2 / (a2 + b2);
        }

        constexpr float SHADOW_EPS = 1e-4f;
        constexpr float LIGHT_BACK_MULTIPLIER = 10.f;
        // LIGHT_BACK_EPS offsets a bit further along the emission direction to avoid self-intersections
        constexpr float LIGHT_BACK_EPS = SHADOW_EPS * LIGHT_BACK_MULTIPLIER;
        inline bool IsBlack(const glm::vec3 &v)
        {
            return glm::all(glm::lessThan(glm::abs(v), glm::vec3(1e-6f)));
        }

        inline bool ShouldTerminatePath(glm::vec3 &beta, float eta_scale, const RNG &rng)
        {
            glm::vec3 beta_q = beta * eta_scale;
            float q = glm::max(beta_q.r, glm::max(beta_q.g, beta_q.b));
            q = glm::min(q, 0.9f);
            if (rng.Uniform() > q)
            {
                return true;
            }
            beta /= q;
            return false;
        }

        struct SampledDirection
        {
        public:
            glm::vec3 direction_world{};
            glm::vec3 bsdf{};
            float pdf{0.f};
            float cos_theta{0.f};
            float eta_scale{1.f};
        };

        inline std::optional<SampledDirection> SampleDirection(const HitInfo &hit_info, const glm::vec3 &incoming_world, const RNG &rng)
        {
            Frame frame(hit_info.__normal__);
            glm::vec3 view_dir = frame.LocalFromWorld(incoming_world);
            if (glm::abs(view_dir.y) < 1e-6f) // grazing directions yield unstable transport; treat as invalid
            {
                return std::nullopt;
            }
            auto bsdf_info = hit_info.__material__->SampleBSDF(hit_info.__hitPoint__, view_dir, rng);
            if (!bsdf_info.has_value())
            {
                return std::nullopt;
            }

            return SampledDirection{
                .direction_world = frame.WorldFromLocal(bsdf_info->__lightDirection__),
                .bsdf = bsdf_info->__bsdf__,
                .pdf = bsdf_info->__pdf__,
                .cos_theta = glm::abs(bsdf_info->__lightDirection__.y),
                .eta_scale = bsdf_info->__etaScale__};
        }

        struct PathVertex
        {
        public:
            glm::vec3 position{};
            glm::vec3 normal{};
            const Material *material{nullptr};
            glm::vec3 beta{1.f};
            float pdfAccum{1.f};
            bool delta{false};
            glm::vec3 wi{}; // incoming world direction from the previous vertex to this vertex
        };
    }

    glm::vec3 BDPTRenderer::RenderPixel(const glm::ivec3 &pixel_coord)
    {
        constexpr int MAX_DEPTH = 6;

        thread_local RNG rng{};
        size_t seed = static_cast<size_t>(pixel_coord.x) * 73856093ull ^
                      static_cast<size_t>(pixel_coord.y) * 19349663ull ^
                      static_cast<size_t>(pixel_coord.z) * 83492791ull;
        // mix pixel indices with distinct primes to decorrelate per-pixel RNG seeds
        rng.SetSeed(seed);

        // ---------------------------------------------------------------------
        // Generate light sub-path
        auto generateLightSubPath = [&](int max_depth) -> std::vector<PathVertex>
        {
            std::vector<PathVertex> vertices;
            const LightSampler &light_sampler = mScene.GetLightSampler(false);
            auto light_sample = light_sampler.Sample(rng.Uniform());
            if (!light_sample.has_value())
            {
                return vertices;
            }
            const Light *light = light_sample->__light__;
            float select_pdf = light_sample->__prob__;

            Ray ray{};
            glm::vec3 beta{1.f};
            float pdf_accum = 1.f;

            if (light->GetLightType() == LightType::Area)
            {
                auto *area_light = dynamic_cast<const AreaLight *>(light);
                if (!area_light)
                {
                    return vertices;
                }
                auto shape_sample = area_light->GetShape().SampleShape(rng);
                if (!shape_sample.has_value())
                {
                    return vertices;
                }

                glm::vec3 normal = shape_sample->__normal__;
                Frame light_frame(normal);
                glm::vec3 local_dir = CosineSampleHemisphere({rng.Uniform(), rng.Uniform()});
                float dir_pdf = CosineSampleHemispherePDF(local_dir);
                if (area_light->IsTwoSides())
                {
                    if (rng.Uniform() < 0.5f)
                    {
                        normal = -normal;
                        light_frame = Frame(normal);
                    }
                    dir_pdf *= 0.5f;
                }
                glm::vec3 world_dir = light_frame.WorldFromLocal(local_dir);
                if (glm::dot(world_dir, normal) <= 0.f)
                {
                    return vertices;
                }

                glm::vec3 emit_dir = world_dir;
                glm::vec3 light_surface_point = shape_sample->__point__;
                glm::vec3 origin = light_surface_point + SHADOW_EPS * emit_dir;
                glm::vec3 radiance_query_point = origin - emit_dir * LIGHT_BACK_EPS;
                glm::vec3 Le = area_light->GetRadiance(radiance_query_point, light_surface_point, normal);
                float cos_theta = glm::abs(glm::dot(normal, emit_dir));

                pdf_accum = select_pdf * shape_sample->__pdf__ * dir_pdf;
                if (pdf_accum == 0.f)
                {
                    return vertices;
                }
                beta = Le * cos_theta / pdf_accum;
                ray = {origin, emit_dir};
            }
            else
            {
                auto light_info = light->SampleLight(mScene.GetCenter(), mScene.GetRadius(), rng, false);
                if (!light_info.has_value())
                {
                    return vertices;
                }
                glm::vec3 dir = -light_info->__direction__;
                glm::vec3 origin = light_info->__lightPoint__ + SHADOW_EPS * dir;
                pdf_accum = select_pdf * light_info->__pdf__;
                if (pdf_accum == 0.f)
                {
                    return vertices;
                }
                beta = light_info->__Le__ / pdf_accum;
                ray = {origin, dir};
            }

            glm::vec3 current_beta = beta;
            float current_pdf_accum = pdf_accum;
            float eta_scale = 1.f;

            for (int depth = 0; depth < max_depth; depth++)
            {
                auto hit_info = mScene.Intersect(ray);
                if (!hit_info.has_value() || !(hit_info->__material__))
                {
                    break;
                }

                PathVertex vertex{};
                vertex.position = hit_info->__hitPoint__;
                vertex.normal = hit_info->__normal__;
                vertex.material = hit_info->__material__;
                vertex.beta = current_beta;
                vertex.pdfAccum = current_pdf_accum;
                vertex.delta = hit_info->__material__->IsDeltaDistribution();
                vertex.wi = -ray.__direction__;
                vertices.push_back(vertex);

                // Russian roulette to terminate deep paths
                if (depth >= 2 && ShouldTerminatePath(current_beta, eta_scale, rng))
                {
                    break;
                }

                auto sampled = SampleDirection(*hit_info, -ray.__direction__, rng);
                if (!sampled.has_value())
                {
                    break;
                }

                current_pdf_accum *= sampled->pdf;
                eta_scale *= sampled->eta_scale;
                current_beta *= sampled->bsdf * sampled->cos_theta / sampled->pdf;

                ray.__origin__ = hit_info->__hitPoint__;
                ray.__direction__ = sampled->direction_world;
            }

            return vertices;
        };

        // ---------------------------------------------------------------------
        // Generate camera sub-path
        auto generateCameraSubPath = [&](int max_depth, glm::vec3 &radiance) -> std::vector<PathVertex>
        {
            std::vector<PathVertex> vertices;
            glm::vec3 beta{1.f};
            float pdf_accum = 1.f;
            float eta_scale = 1.f;

            Ray ray = mCamera.GenerateRay({pixel_coord.x, pixel_coord.y}, {rng.Uniform(), rng.Uniform()});

            for (int depth = 0; depth < max_depth; depth++)
            {
                auto hit_info = mScene.Intersect(ray);
                if (!hit_info.has_value())
                {
                    glm::vec3 light_dir_delta = glm::normalize(ray.__direction__);
                    glm::vec3 light_point = ray.__origin__ + mScene.GetRadius() * 2.f * light_dir_delta;
                    for (const auto &light : mScene.GetInfiniteLights())
                    {
                        radiance += beta * light->GetRadiance(ray.__origin__, light_point, -light_dir_delta);
                    }
                    break;
                }

                if (hit_info->__material__ && hit_info->__material__->mAreaLight)
                {
                    radiance += beta * hit_info->__material__->mAreaLight->GetRadiance(ray.__origin__, hit_info->__hitPoint__, hit_info->__normal__);
                }

                if (!(hit_info->__material__))
                {
                    break;
                }

                PathVertex vertex{};
                vertex.position = hit_info->__hitPoint__;
                vertex.normal = hit_info->__normal__;
                vertex.material = hit_info->__material__;
                vertex.beta = beta;
                vertex.pdfAccum = pdf_accum;
                vertex.delta = hit_info->__material__->IsDeltaDistribution();
                vertex.wi = -ray.__direction__;
                vertices.push_back(vertex);

                // Russian roulette
                if (depth >= 2 && ShouldTerminatePath(beta, eta_scale, rng))
                {
                    break;
                }

                auto sampled = SampleDirection(*hit_info, -ray.__direction__, rng);
                if (!sampled.has_value())
                {
                    break;
                }

                pdf_accum *= sampled->pdf;
                eta_scale *= sampled->eta_scale;
                beta *= sampled->bsdf * sampled->cos_theta / sampled->pdf;

                ray.__origin__ = hit_info->__hitPoint__;
                ray.__direction__ = sampled->direction_world;
            }

            return vertices;
        };

        glm::vec3 radiance{0.f, 0.f, 0.f};
        auto light_vertices = generateLightSubPath(MAX_DEPTH);
        auto camera_vertices = generateCameraSubPath(MAX_DEPTH, radiance);

        // ---------------------------------------------------------------------
        // Connect sub-paths
        for (const auto &lv : light_vertices)
        {
            for (const auto &cv : camera_vertices)
            {
                if (lv.delta || cv.delta)
                {
                    continue;
                }

                glm::vec3 conn = cv.position - lv.position;
                float dist2 = glm::dot(conn, conn);
                if (dist2 <= 1e-6f)
                {
                    continue;
                }
                float dist = glm::sqrt(dist2);
                glm::vec3 dir = conn / dist;

                glm::vec3 shadow_origin = lv.position + dir * SHADOW_EPS;
                float shadow_tmin = SHADOW_EPS;
                float shadow_tmax = dist - SHADOW_EPS;
                if (mScene.Intersect(Ray{shadow_origin, dir}, shadow_tmin, shadow_tmax))
                {
                    continue;
                }

                Frame light_frame(lv.normal);
                Frame camera_frame(cv.normal);
                glm::vec3 wo_light_local = light_frame.LocalFromWorld(dir);
                glm::vec3 wi_light_local = light_frame.LocalFromWorld(lv.wi);
                glm::vec3 wo_camera_local = camera_frame.LocalFromWorld(-dir);
                glm::vec3 wi_camera_local = camera_frame.LocalFromWorld(cv.wi);

                float cos_light = glm::abs(wo_light_local.y);
                float cos_camera = glm::abs(wo_camera_local.y);
                if (cos_light == 0.f || cos_camera == 0.f)
                {
                    continue;
                }

                glm::vec3 f_light = lv.material->BSDF(lv.position, wo_light_local, wi_light_local);
                glm::vec3 f_camera = cv.material->BSDF(cv.position, wo_camera_local, wi_camera_local);
                if (IsBlack(f_light) || IsBlack(f_camera))
                {
                    continue;
                }

                float pdf_light = lv.material->PDF(lv.position, wo_light_local, wi_light_local);
                float pdf_camera = cv.material->PDF(cv.position, wo_camera_local, wi_camera_local);

                float p_light = lv.pdfAccum * pdf_light;
                float p_camera = cv.pdfAccum * pdf_camera;
                float weight = 1.f;
                if (p_light > 0.f || p_camera > 0.f)
                {
                    weight = PowerHeuristic(p_light, p_camera);
                }

                glm::vec3 contrib = lv.beta * cv.beta * f_light * f_camera * (cos_light * cos_camera / dist2) * weight;
                radiance += contrib;
            }
        }

        return radiance;
    }
}
