#include "microfacet.hpp"
#include "sampler/spherical.hpp"
#include "utils/frame.hpp"

namespace pbrt
{

    Microfacet::Microfacet(float alpha_x, float alpha_z)
    {
        mAlphaX = glm::clamp(alpha_x * alpha_x, 1e-3f, 1.f);
        mAlphaZ = glm::clamp(alpha_z * alpha_z, 1e-3f, 1.f);
    }

    float Microfacet::D(const glm::vec3 &microfacet_normal) const
    {
        glm::vec2 slope{-microfacet_normal.x / microfacet_normal.y, -microfacet_normal.z / microfacet_normal.y};
        slope.x /= mAlphaX;
        slope.y /= mAlphaZ;
        float slope_distribution = SlopeDistribution(slope) / (mAlphaX * mAlphaZ);
        return slope_distribution / glm::pow(microfacet_normal.y, 4);
    }

    float Microfacet::G1(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        if (glm::dot(view_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        return 1.f / (1.f + Lambda(view_dir_up));
    }

    float Microfacet::G2(const glm::vec3 &light_dir, const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        glm::vec3 light_dir_up = light_dir.y > 0 ? light_dir : -light_dir;
        if (glm::dot(light_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        if (glm::dot(view_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        return 1.f / (1.f + Lambda(light_dir_up) + Lambda(view_dir_up));
    }

    float Microfacet::IsDeltaDistribution() const
    {
        return glm::max(mAlphaX, mAlphaZ) == 1e-3f;
    }

    float Microfacet::VisibleNormalDistribution(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        float cos_theta_o = glm::dot(view_dir_up, microfacet_normal);
        if (cos_theta_o <= 0.f)
            return 0.f;

        return D(microfacet_normal) * cos_theta_o * G1(view_dir, microfacet_normal) / glm::abs(view_dir.y);
    }

    glm::vec3 Microfacet::SampleVisibleNormal(const glm::vec3 &view_dir, const RNG &rng) const
    {
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        glm::vec3 view_dir_hemi = glm::normalize(glm::vec3(mAlphaX * view_dir_up.x, view_dir_up.y, mAlphaZ * view_dir_up.z));

        glm::vec2 sample = UniformSampleUnitDisk({rng.Uniform(), rng.Uniform()});
        float h = glm::sqrt(1.f - sample.x * sample.x);
        float t = 0.5f * (1.f + view_dir_hemi.y);
        sample.y = t * sample.y + (1.f - t) * h;

        Frame frame{view_dir_hemi};
        glm::vec3 microfacet_normal_hemi = frame.WorldFromLocal({sample.x, glm::sqrt(1.f - sample.x * sample.x - sample.y * sample.y), sample.y});
        return glm::normalize(glm::vec3(mAlphaX * microfacet_normal_hemi.x, microfacet_normal_hemi.y, mAlphaZ * microfacet_normal_hemi.z));
    }

    float Microfacet::SlopeDistribution(const glm::vec2 &slope) const
    {
        return INV_PI / (glm::pow(1.f + slope.x * slope.x + slope.y * slope.y, 2));
    }

    float Microfacet::Lambda(const glm::vec3 &dir_up) const
    {
        if (dir_up.y == 0.f)
        {
            return std::numeric_limits<float>::infinity();
        }
        float length_2 = dir_up.x * dir_up.x + dir_up.z * dir_up.z;
        if (length_2 == 0.f)
        {
            return 0.f;
        }
        float cos2_phi = dir_up.x * dir_up.x / length_2;
        float sin2_phi = dir_up.z * dir_up.z / length_2;
        float tan2_theta = length_2 / (dir_up.y * dir_up.y);
        float alpha0_2 = mAlphaX * mAlphaX * cos2_phi + mAlphaZ * mAlphaZ * sin2_phi;
        return 0.5f * (glm::sqrt(1.f + alpha0_2 * tan2_theta) - 1.f);
    }
}