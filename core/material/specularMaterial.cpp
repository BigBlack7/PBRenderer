#include "specularMaterial.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    /*
        在路径追踪中:
            当光线以斜角入射时, 单位面积接收到的能量会减少
            这种减少遵循朗伯余弦定律: I = I₀ * cosθ
            但镜面反射是恒定的, 不应该有余弦衰减

        镜面BRDF: f_r(ω_i, ω_o) = ρ_s / cosθ_i
        其中:
            ρ_s: 镜面反射率(mAlbedo)
            cosθ_i: 入射角余弦(light_dir.y)

        L_o = ∫ f_r(ω_i, ω_o) * L_i(ω_i) * cosθ_i dω_i
            = ∫ (ρ_s / cosθ_i) * L_i(ω_i) * cosθ_i dω_i
            = ∫ ρ_s * L_i(ω_i) dω_i
    */
    std::optional<BSDFInfo> SpecularMaterial::SampleBSDF(const glm::vec3 &hit_point, const glm::vec3 &view_dir, const RNG &rng) const
    {
        // 局部坐标系下y轴为(0, 1, 0), 故该坐标系下光线方向余弦为light_dir.y
        glm::vec3 light_dir{-view_dir.x, view_dir.y, -view_dir.z};
        glm::vec3 bsdf = mAlbedo / glm::abs(light_dir.y);
        return BSDFInfo{
            .__bsdf__ = bsdf,
            .__pdf__ = 1.f,
            .__lightDirection__ = light_dir
            // end 
        };
    }
}