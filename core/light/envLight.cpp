#include "envLight.hpp"
#include "sampler/spherical.hpp"

namespace pbrt
{
    glm::vec2 EnvLight::ImagePointFromDirection(const glm::vec3 &direction) const
    {
        float theta = 0;
        float phi = 0;
        glm::vec3 normalized_direction = glm::normalize(direction);
        if (glm::abs(normalized_direction.y) < 0.99999)
        {
            theta = glm::degrees(glm::acos(normalized_direction.y));
            float sin_phi = glm::abs(normalized_direction.z / glm::sqrt(1 - normalized_direction.y * normalized_direction.y));
            phi = glm::degrees(glm::asin(sin_phi));
            if ((direction.x <= 0) && (direction.z > 0))
            {
                phi = 180 - phi;
            }
            else if ((direction.x < 0) && (direction.z <= 0))
            {
                phi = 180 + phi;
            }
            else if ((direction.x >= 0) && (direction.z < 0))
            {
                phi = 360 - phi;
            }
        }
        else
        {
            theta = (normalized_direction.y > 0) ? 0 : 180;
        }

        phi += mStartPhi;
        if (phi > 360)
        {
            phi -= 360;
        }
        return {mImage->GetWidth() * phi / 360, mImage->GetHeight() * theta / 180};
    }

    glm::vec3 EnvLight::DirectionFromImagePoint(const glm::vec2 &image_point) const
    {
        float theta = glm::radians(180 * image_point.y / mImage->GetHeight());
        float phi = glm::radians(360 * image_point.x / mImage->GetWidth());

        phi -= glm::radians(mStartPhi);
        if (phi > 360)
        {
            phi -= 360;
        }

        float sin_theta = glm::sin(theta);
        float cos_theta = glm::cos(theta);
        float sin_phi = glm::sin(phi);
        float cos_phi = glm::cos(phi);
        return {sin_theta * cos_phi, cos_theta, sin_theta * sin_phi};
    }

    glm::ivec2 EnvLight::GirdIdxFromImagePoint(const glm::vec2 &image_point) const
    {
        glm::ivec2 point_discrete{glm::clamp<int>(image_point.x, 0, mImage->GetWidth() - 1), glm::clamp<int>(image_point.y, 0, mImage->GetHeight() - 1)};
        return point_discrete / static_cast<int>(mGridSize);
    }

    EnvLight::EnvLight(const Image *image, float start_phi) : mImage(image), mStartPhi(start_phi)
    {
        mPrecomputePhi = 0;
        mGridCount = GirdIdxFromImagePoint(mImage->GetResolution()) + 1;
        std::vector<float> grids_phi(mGridCount.x * mGridCount.y);
        for (size_t x = 0; x < mImage->GetWidth(); x++)
        {
            for (size_t y = 0; y < mImage->GetHeight(); y++)
            {
                glm::vec3 radiance = mImage->GetPixel(x, y);
                float pixel_phi = glm::max(radiance.r, glm::max(radiance.g, radiance.b)) * (glm::cos(y * PI / mImage->GetHeight()) - glm::cos((y + 1) * PI / mImage->GetHeight()));
                mPrecomputePhi += pixel_phi;

                auto grid_idx = GirdIdxFromImagePoint({x, y});
                grids_phi[grid_idx.x + grid_idx.y * mGridCount.x] += pixel_phi;
            }
        }
        float average_phi = mPrecomputePhi / (mGridCount.x * mGridCount.y);
        mPrecomputePhi *= 2 * PI * PI / mImage->GetWidth();
        mAliasTable.Build(grids_phi);
        for (float &grid_phi : grids_phi)
        {
            if (grid_phi > average_phi)
            {
                grid_phi -= average_phi;
            }
            else
            {
                grid_phi = 0;
            }
        }
        mAliasTableMISC.Build(grids_phi);
    }

    std::optional<LightInfo> EnvLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, const RNG &rng, bool MISC) const
    {
        auto res = (MISC ? mAliasTableMISC : mAliasTable).Sample(rng.Uniform());
        size_t grid_x = res.__idx__ % mGridCount.x;
        size_t grid_y = res.__idx__ / mGridCount.x;
        float w = glm::min<float>(mGridSize, mImage->GetWidth() - grid_x * mGridSize);
        float h = glm::min<float>(mGridSize, mImage->GetHeight() - grid_y * mGridSize);

        glm::vec2 image_point{grid_x * mGridSize + w * rng.Uniform(), grid_y * mGridSize + h * rng.Uniform()};
        glm::vec3 light_direction = DirectionFromImagePoint(image_point);
        if (glm::abs(light_direction.y) == 1)
        {
            return {};
        }
        return LightInfo{
            surface_point + 2 * scene_radius * light_direction,
            light_direction,
            mImage->GetPixel(image_point),
            res.__prob__ * mImage->GetHeight() * mImage->GetWidth() / (2 * PI * PI * glm::sqrt(1 - light_direction.y * light_direction.y) * w * h)};
    }

    // 参数化采样接口：使用u_select选择网格，u_surface在网格内采样
    std::optional<LightInfo> EnvLight::SampleLight(const glm::vec3 &surface_point, float scene_radius, float u_select, const glm::vec2 &u_surface, bool MISC) const
    {
        auto res = (MISC ? mAliasTableMISC : mAliasTable).Sample(u_select);
        size_t grid_x = res.__idx__ % mGridCount.x;
        size_t grid_y = res.__idx__ / mGridCount.x;
        float w = glm::min<float>(mGridSize, mImage->GetWidth() - grid_x * mGridSize);
        float h = glm::min<float>(mGridSize, mImage->GetHeight() - grid_y * mGridSize);

        glm::vec2 image_point{grid_x * mGridSize + w * u_surface.x, grid_y * mGridSize + h * u_surface.y};
        glm::vec3 light_direction = DirectionFromImagePoint(image_point);
        if (glm::abs(light_direction.y) == 1)
        {
            return {};
        }
        return LightInfo{
            surface_point + 2 * scene_radius * light_direction,
            light_direction,
            mImage->GetPixel(image_point),
            res.__prob__ * mImage->GetHeight() * mImage->GetWidth() / (2 * PI * PI * glm::sqrt(1 - light_direction.y * light_direction.y) * w * h)};
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
            return {};
        }
        glm::vec2 image_point = ImagePointFromDirection(light_direction);
        glm::ivec2 grid_idx = GirdIdxFromImagePoint(image_point);
        float w = glm::min<float>(mGridSize, mImage->GetWidth() - grid_idx.x * mGridSize);
        float h = glm::min<float>(mGridSize, mImage->GetHeight() - grid_idx.y * mGridSize);
        float grid_prob = (MISC ? mAliasTableMISC : mAliasTable).GetProbs()[grid_idx.x + grid_idx.y * mGridCount.x];
        return grid_prob * mImage->GetWidth() * mImage->GetHeight() / (2 * PI * PI * glm::sqrt(1 - light_direction.y * light_direction.y) * w * h);
    }
}