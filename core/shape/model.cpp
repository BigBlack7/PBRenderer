#include "model.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace pt
{
    Model::Model(const std::filesystem::path &filename)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;

        std::ifstream file(filename);
        if (!file.good())
        {
            PBRT_ERROR("Model file not found at {}", std::filesystem::absolute(filename).string());
            return;
        }

        std::string line;
        char trash;
        while (!file.eof())
        {
            std::getline(file, line);
            std::istringstream iss(line);

            if (line.compare(0, 2, "v ") == 0)
            {
                glm::vec3 position;
                iss >> trash >> position.x >> position.y >> position.z;
                positions.push_back(position);
            }
            if (line.compare(0, 3, "vn ") == 0)
            {
                glm::vec3 normal;
                iss >> trash >> trash >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            }
            else if (line.compare(0, 2, "f ") == 0)
            {
                glm::ivec3 idx_v, idx_vn;
                iss >> trash;
                iss >> idx_v.x >> trash >> trash >> idx_vn.x;
                iss >> idx_v.y >> trash >> trash >> idx_vn.y;
                iss >> idx_v.z >> trash >> trash >> idx_vn.z;
                mTriangles.push_back(Triangle(
                    positions[idx_v.x - 1], positions[idx_v.y - 1], positions[idx_v.z - 1],
                    normals[idx_vn.x - 1], normals[idx_vn.y - 1], normals[idx_vn.z - 1]));
            }
        }

        if (mTriangles.empty())
        {
            PBRT_ERROR("Model file {} is empty", std::filesystem::absolute(filename).string());
            return;
        }
        else
        {
            PBRT_INFO("Model file {} loaded with {} triangles", std::filesystem::absolute(filename).string(), mTriangles.size());
        }
    }

    std::optional<HitInfo> Model::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info;
        for (const auto &triangle : mTriangles)
        {
            if (auto hit_info = triangle.Intersect(ray, t_min, t_max); hit_info.has_value())
            {
                t_max = hit_info->__t__;
                closest_hit_info = hit_info;
            }
        }
        return closest_hit_info;
    }
}