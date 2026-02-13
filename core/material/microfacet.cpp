#include "microfacet.hpp"
#include "sampler/spherical.hpp"
#include "utils/frame.hpp"

namespace pbrt
{

    Microfacet::Microfacet(float alpha_x, float alpha_z)
    {
        // 人眼感知的粗糙度是非线性的, 平方处理以获得更好的视觉效果
        mAlphaX = glm::clamp(alpha_x * alpha_x, 1e-3f, 1.f);
        mAlphaZ = glm::clamp(alpha_z * alpha_z, 1e-3f, 1.f);
    }

    float Microfacet::D(const glm::vec3 &microfacet_normal) const
    {
        // 根据微表面法线得到微表面的斜率
        glm::vec2 slope{-microfacet_normal.x / microfacet_normal.y, -microfacet_normal.z / microfacet_normal.y};
        // 将拉伸后的斜率转换回拉伸前的斜率
        slope.x /= mAlphaX;
        slope.y /= mAlphaZ;
        // 使用拉伸前的斜率得到斜率分布函数值
        float slope_distribution = SlopeDistribution(slope) / (mAlphaX * mAlphaZ);
        // 根据jakob行列式将斜率分布函数转换为微表面法线分布函数
        return slope_distribution / glm::pow(microfacet_normal.y, 4);
    }

    float Microfacet::G1(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        // O+: 将观察方向转换到微表面上方
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        // X+: 如果观察方向在微表面下方, 则X+函数值0
        if (glm::dot(view_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        return 1.f / (1.f + Lambda(view_dir_up));
    }

    float Microfacet::G2(const glm::vec3 &light_dir, const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        // O+: 将光线方向转换到微表面上方 | X+: 如果入射方向在微表面下方, 则X+函数值0
        glm::vec3 light_dir_up = light_dir.y > 0 ? light_dir : -light_dir;
        if (glm::dot(light_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        if (glm::dot(view_dir_up, microfacet_normal) <= 0.f)
            return 0.f;

        return 1.f / (1.f + Lambda(light_dir_up) + Lambda(view_dir_up));
    }

    bool Microfacet::IsDeltaDistribution() const
    {
        return glm::max(mAlphaX, mAlphaZ) == 1e-3f;
    }

    float Microfacet::VisibleNormalDistribution(const glm::vec3 &view_dir, const glm::vec3 &microfacet_normal) const
    {
        // 确保观察方向在微表面上方
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        float cos_theta_o = glm::dot(view_dir_up, microfacet_normal);
        if (cos_theta_o <= 0.f)
            return 0.f;

        return D(microfacet_normal) * cos_theta_o * G1(view_dir, microfacet_normal) / glm::abs(view_dir.y);
    }

    glm::vec3 Microfacet::SampleVisibleNormal(const glm::vec3 &view_dir, const RNG &rng) const
    {
        // 根据伸缩不变性将观察方向从椭球坐标系转换到球坐标系下
        glm::vec3 view_dir_up = view_dir.y > 0 ? view_dir : -view_dir;
        glm::vec3 view_dir_hemi = glm::normalize(glm::vec3(mAlphaX * view_dir_up.x, view_dir_up.y, mAlphaZ * view_dir_up.z));

        // 均匀采样单位圆, 并根据投影关系将采样点映射到对应观察方向的投影面积中
        glm::vec2 sample = UniformSampleUnitDisk({rng.Uniform(), rng.Uniform()});
        float h = glm::sqrt(1.f - sample.x * sample.x);
        float t = 0.5f * (1.f + view_dir_hemi.y);
        sample.y = t * sample.y + (1.f - t) * h;
        
        // 以球体坐标系下的观察方向为Y轴构建正交坐标系
        Frame frame{view_dir_hemi};
        // 将采样点对应的微表面法线从局部坐标系转换到球体坐标系
        glm::vec3 microfacet_normal_hemi = frame.WorldFromLocal({sample.x, glm::sqrt(1.f - sample.x * sample.x - sample.y * sample.y), sample.y});
        // 将球体坐标系下的微表面法线转换回椭球坐标系(对于法线的变换应该是拉伸不变性的逆变换)
        return glm::normalize(glm::vec3(mAlphaX * microfacet_normal_hemi.x, microfacet_normal_hemi.y, mAlphaZ * microfacet_normal_hemi.z));
    }

    // 斜率分布函数, 拉伸前的形状分布函数
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
        if (length_2 == 0.f) // 从微表面正上方观察, 则Lambda函数值为0
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