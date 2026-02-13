#include "dielectricMaterial.hpp"

namespace pbrt
{
    float DielectricMaterial::Fresnel(float etai_div_etat, float cos_theta_t, float &cos_theta_i) const
    {
        float sin2_theta_t = 1 - cos_theta_t * cos_theta_t;
        float sin2_theta_i = sin2_theta_t / (etai_div_etat * etai_div_etat);

        /*
            全内反射:
            当光线从较高折射率的介质进入到较低折射率的介质时, 如果入射角大于某一临界角(即光线远离法线)时,
            折射光线将会消失, 所有的入射光线将被反射而不进入低折射率的介质
        */
        if (sin2_theta_i >= 1)
        {
            return 1.f;
        }
        cos_theta_i = glm::sqrt(1 - sin2_theta_i);

        float r_parl = (cos_theta_i - etai_div_etat * cos_theta_t) / (cos_theta_i + etai_div_etat * cos_theta_t);
        float r_perp = (etai_div_etat * cos_theta_i - cos_theta_t) / (etai_div_etat * cos_theta_i + cos_theta_t);
        return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
    }

    // 约定i为光入射方向, t为透射方向对应观察方向
    std::optional<BSDFInfo> DielectricMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        if (mIOR == 1)
        {
            return BSDFInfo{
                .__bsdf__ = mAlbedoT / glm::abs(view_dir.y),
                .__pdf__ = 1.f,
                .__lightDirection__ = -view_dir
                // end
            };
        }

        float etai_div_etat = mIOR; // 约定i在物体内, t在物体外
        glm::vec3 microfacet_normal{0.f, 1.f, 0.f};
        if (!mMicrofacet.IsDeltaDistribution()) // 非Delta分布微表面模型, 则需要采样微表面法线(即足够光滑时不考虑微面元模型)
        {
            microfacet_normal = mMicrofacet.SampleVisibleNormal(view_dir, rng);
        }

        // 判断观察方向在表面上方还是下方, 约定已知观察方向(折射方向)反推光线入射方向
        float cos_theta_t = view_dir.y;
        float inverse = 1.f;
        if (cos_theta_t < 0) // 观察方向在表面下方
        {
            etai_div_etat = 1.f / mIOR;
            inverse = -1.f;
            cos_theta_t = -cos_theta_t;
        }

        float cos_theta_i; // 入射方向余弦
        float F = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

        if (rng.Uniform() <= F) // 反射
        {
            glm::vec3 light_dir = -view_dir + 2.f * glm::dot(view_dir, microfacet_normal) * microfacet_normal;
            if (mMicrofacet.IsDeltaDistribution())
            {
                return BSDFInfo{
                    .__bsdf__ = mAlbedoR / glm::abs(light_dir.y),
                    .__pdf__ = 1.f,
                    .__lightDirection__ = light_dir
                    // end
                };
            }
            glm::vec3 brdf = F * mAlbedoR * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * light_dir.y * view_dir.y);
            float pdf = F * mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
            return BSDFInfo{
                .__bsdf__ = brdf,
                .__pdf__ = pdf,
                .__lightDirection__ = light_dir
                // end
            };
        }
        else // 透射
        {
            glm::vec3 light_dir{(-view_dir / etai_div_etat) + (cos_theta_t / etai_div_etat - cos_theta_i) * microfacet_normal * inverse};

            float det_J = etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)) / glm::pow(glm::abs(glm::dot(view_dir, microfacet_normal)) - etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)), 2.f);

            glm::vec3 btdf = (1.f - F) * mAlbedoT * det_J * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) * glm::abs(glm::dot(view_dir, microfacet_normal) / (light_dir.y * view_dir.y));

            float pdf = (1.f - F) * mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) * det_J;

            return BSDFInfo{
                .__bsdf__ = btdf / (etai_div_etat * etai_div_etat),
                .__pdf__ = pdf,
                .__lightDirection__ = light_dir,
                .__etaScale__ = (etai_div_etat * etai_div_etat)
                // end
            };
        }
    }

    glm::vec3 DielectricMaterial::BSDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (IsDeltaDistribution())
        {
            return {};
        }
        float lv = light_dir.y * view_dir.y;
        if (lv == 0.f) // 观察方向与光线方向垂直, 即掠射角
        {
            return {};
        }

        float etai_div_etat = mIOR;
        float cos_theta_t = view_dir.y;
        float inverse = 1.f;
        if (cos_theta_t < 0)
        {
            etai_div_etat = 1.f / mIOR;
            inverse = -1.f;
            cos_theta_t = -cos_theta_t;
        }

        float cos_theta_i;
        float F = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

        if (lv < 0.f) // 透射, 观察方向与光线方向不位于同一个半球
        {
            glm::vec3 microfacet_normal = (light_dir + view_dir / etai_div_etat) * inverse / (cos_theta_t / etai_div_etat - cos_theta_i);
            float det_J = etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)) / glm::pow(glm::abs(glm::dot(view_dir, microfacet_normal)) - etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)), 2.f);

            glm::vec3 btdf = (1.f - F) * mAlbedoT * det_J * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) * glm::abs(glm::dot(view_dir, microfacet_normal) / lv);
            return btdf / (etai_div_etat * etai_div_etat);
        }

        // 反射, 观察方向与光线方向位于同一个半球
        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f)
        {
            microfacet_normal = -microfacet_normal;
        }
        glm::vec3 brdf = F * mAlbedoR * mMicrofacet.D(microfacet_normal) * mMicrofacet.G2(light_dir, view_dir, microfacet_normal) / glm::abs(4.f * lv);
        return brdf;
    }

    float DielectricMaterial::PDF(const glm::vec3 &hit_point, const glm::vec3 &light_dir, const glm::vec3 &view_dir) const
    {
        if (IsDeltaDistribution())
        {
            return 0.f;
        }
        float lv = light_dir.y * view_dir.y;
        if (lv == 0.f) // 观察方向与光线方向垂直, 即掠射角
        {
            return 0.f;
        }

        float etai_div_etat = mIOR;
        float cos_theta_t = view_dir.y;
        float inverse = 1.f;
        if (cos_theta_t < 0)
        {
            etai_div_etat = 1.f / mIOR;
            inverse = -1.f;
            cos_theta_t = -cos_theta_t;
        }

        float cos_theta_i;
        float F = Fresnel(etai_div_etat, cos_theta_t, cos_theta_i);

        if (lv < 0.f) // 透射, 观察方向与光线方向不位于同一个半球
        {
            glm::vec3 microfacet_normal = (light_dir + view_dir / etai_div_etat) * inverse / (cos_theta_t / etai_div_etat - cos_theta_i);
            float det_J = etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)) / glm::pow(glm::abs(glm::dot(view_dir, microfacet_normal)) - etai_div_etat * etai_div_etat * glm::abs(glm::dot(light_dir, microfacet_normal)), 2.f);

            return (1.f - F) * mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) * det_J;
        }

        // 反射, 观察方向与光线方向位于同一个半球
        glm::vec3 microfacet_normal = glm::normalize(light_dir + view_dir);
        if (microfacet_normal.y <= 0.f) // 局部坐标系内保持微表面法线位于上半球
        {
            microfacet_normal = -microfacet_normal;
        }
        return F * mMicrofacet.VisibleNormalDistribution(view_dir, microfacet_normal) / glm::abs(4.f * glm::dot(view_dir, microfacet_normal));
    }

    void DielectricMaterial::Regularize() const
    {
        float alpha_x = mMicrofacet.GetAlphaX();
        float alpha_z = mMicrofacet.GetAlphaZ();

        if (alpha_x < 0.3f)
            alpha_x = glm::clamp(2 * alpha_x, 0.1f, 0.3f);
        mMicrofacet.SetAlphaX(alpha_x);

        if (alpha_z < 0.3f)
            alpha_z = glm::clamp(2 * alpha_z, 0.1f, 0.3f);
        mMicrofacet.SetAlphaZ(alpha_z);
    }
}