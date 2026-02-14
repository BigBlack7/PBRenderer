#include "envLight.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    glm::vec2 EnvLight::ImagePointFromDirection(const glm::vec3 &direction) const
    {
        /*  象限判断方法:
             1. 计算θ角: θ = acos(y), 范围为[0, 180]度
             2. 计算φ角: φ = asin(z / sin(θ)), 范围为[-90, 90]度
             3. 根据x和z的符号判断φ所在的象限:
                - 第一象限(x > 0, z > 0): φ保持不变
                - 第二象限(x <= 0, z > 0): φ = 180 - φ
                - 第三象限(x < 0, z <= 0): φ = 180 + φ
                - 第四象限(x >= 0, z < 0): φ = 360 - φ
            当y接近±1时, sin(θ)接近0, 可能导致数值不稳定, 需要特殊处理
        */
        float theta = 0;
        float phi = 0;
        glm::vec3 normalized_direction = glm::normalize(direction);
        if (glm::abs(normalized_direction.y) < 0.99999)
        {
            theta = glm::degrees(glm::acos(normalized_direction.y));
            float sin_phi = glm::abs(normalized_direction.z / glm::sqrt(1 - normalized_direction.y * normalized_direction.y));
            phi = glm::degrees(glm::asin(sin_phi));
            // 判断phi角在xz平面的象限
            if ((direction.x <= 0) && (direction.z > 0)) // 第二象限
            {
                phi = 180 - phi;
            }
            else if ((direction.x < 0) && (direction.z <= 0)) // 第三象限
            {
                phi = 180 + phi;
            }
            else if ((direction.x >= 0) && (direction.z < 0)) // 第四象限
            {
                phi = 360 - phi;
            }
        }
        else // 垂直向上或向下
        {
            theta = (normalized_direction.y > 0) ? 0 : 180;
        }

        // 根据偏置角度调整环境贴图位置
        phi += mStartPhi;
        if (phi > 360)
        {
            phi -= 360;
        }
        return glm::vec2{mImage->GetWidth() * phi / 360, mImage->GetHeight() * theta / 180};
    }

    glm::vec3 EnvLight::DirectionFromImagePoint(const glm::vec2 &image_point) const
    {
        // 环境贴图上的点得到θ和φ角度
        float theta = glm::radians(180 * image_point.y / mImage->GetHeight());
        float phi = glm::radians(360 * image_point.x / mImage->GetWidth() - mStartPhi);

        float sin_theta = glm::sin(theta);
        float cos_theta = glm::cos(theta);
        float sin_phi = glm::sin(phi);
        float cos_phi = glm::cos(phi);
        // 将θ和φ角度转换为方向向量
        return glm::vec3{sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};
    }

    glm::ivec2 EnvLight::GirdIdxFromImagePoint(const glm::vec2 &image_point) const
    {
        // 将环境贴图上的点映射到网格索引
        glm::ivec2 discrete_point{glm::clamp<int>(image_point.x, 0, mImage->GetWidth() - 1), glm::clamp<int>(image_point.y, 0, mImage->GetHeight() - 1)};
        return discrete_point / static_cast<int>(mGridSize);
    }

    EnvLight::EnvLight(const Image *image, float start_phi) : mImage(image), mStartPhi(start_phi)
    {
        mPrecomputePhi = 0;
        mGridCount = GirdIdxFromImagePoint(mImage->GetResolution()) + 1;
        std::vector<float> grids_phi(mGridCount.x * mGridCount.y);
        for (size_t y = 0; y < mImage->GetHeight(); y++)
        {
            for (size_t x = 0; x < mImage->GetWidth(); x++)
            {
                glm::vec3 radiance = mImage->GetPixel(x, y);
                // 每像素的光功率φ
                float pixel_phi = glm::max(radiance.r, glm::max(radiance.g, radiance.b)) * (glm::cos(y * PI / mImage->GetHeight()) - glm::cos((y + 1) * PI / mImage->GetHeight()));
                mPrecomputePhi += pixel_phi;

                // 网格光功率
                auto grid_idx = GirdIdxFromImagePoint({x, y});
                grids_phi[grid_idx.x + grid_idx.y * mGridCount.x] += pixel_phi;
            }
        }
        float average_phi = mPrecomputePhi / (mGridCount.x * mGridCount.y); // 每个网格的平均光功率
        mPrecomputePhi *= 2 * PI * PI / mImage->GetWidth();                 // 预计算光功率φ系数
        mAliasTable.Build(grids_phi);
        // 提高对高亮度小区域的采样概率
        mCompensated = true;
        for (float &grid_phi : grids_phi)
        {
            if (grid_phi > average_phi) // 对网格光功率进行修正
            {
                mCompensated = false;
                grid_phi -= average_phi;
            }
            else // 低于平均值的网格功率设为0
            {
                grid_phi = 0;
            }
        }
        if (!mCompensated)
        {
            mAliasTableMISC.Build(grids_phi);
        }
    }

    std::optional<LightInfo> EnvLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const
    {
        auto res = ((MISC && (!mCompensated)) ? mAliasTableMISC : mAliasTable).Sample(rng.Uniform()); // 从别名表中采样网格索引
        // 根据网格索引得到网格坐标
        size_t grid_x = res.__idx__ % mGridCount.x;
        size_t grid_y = res.__idx__ / mGridCount.x;
        // 网格尺寸(边缘网格可能不满足默认尺寸)
        float w = glm::min<float>(mGridSize, mImage->GetWidth() - grid_x * mGridSize);
        float h = glm::min<float>(mGridSize, mImage->GetHeight() - grid_y * mGridSize);

        // 从网格中随机均匀采样一个点
        glm::vec2 image_point{grid_x * mGridSize + w * rng.Uniform(), grid_y * mGridSize + h * rng.Uniform()};
        // 将环境贴图上的点映射到方向向量
        glm::vec3 light_direction = DirectionFromImagePoint(image_point);
        // 避免采样到垂直方向, 会使得pdf计算除零异常
        if (glm::abs(light_direction.y) == 1)
        {
            return std::nullopt;
        }
        return LightInfo{
            .__lightPoint__ = surface_point + 2 * scene_radius * light_direction,
            .__direction__ = light_direction,
            .__Le__ = mImage->GetPixel(image_point),
            .__pdf__ = res.__prob__ * mImage->GetHeight() * mImage->GetWidth() / (2 * PI * PI * glm::sqrt(1 - light_direction.y * light_direction.y) * w * h)
            // end
        };
    }

    glm::vec3 EnvLight::GetRadiance(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal) const
    {
        glm::vec3 light_direction = glm::normalize(light_point - surface_point);
        if (glm::abs(light_direction.y) == 1)
        {
            return {};
        }
        return mImage->GetPixel(ImagePointFromDirection(light_direction));
    }

    float EnvLight::PDF(const glm::vec3 &surface_point, const glm::vec3 &light_point, const glm::vec3 &normal, bool MISC) const
    {
        glm::vec3 light_direction = glm::normalize(light_point - surface_point);
        if (glm::abs(light_direction.y) == 1)
        {
            return 0.f;
        }
        glm::vec2 image_point = ImagePointFromDirection(light_direction);
        glm::ivec2 grid_idx = GirdIdxFromImagePoint(image_point);
        float w = glm::min<float>(mGridSize, mImage->GetWidth() - grid_idx.x * mGridSize);
        float h = glm::min<float>(mGridSize, mImage->GetHeight() - grid_idx.y * mGridSize);
        float grid_prob = ((MISC && (!mCompensated)) ? mAliasTableMISC : mAliasTable).GetProbs()[grid_idx.x + grid_idx.y * mGridCount.x];
        return grid_prob * mImage->GetWidth() * mImage->GetHeight() / (2 * PI * PI * glm::sqrt(1 - light_direction.y * light_direction.y) * w * h);
    }
}