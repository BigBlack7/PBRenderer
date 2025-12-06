#include "model.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <rapidobj/rapidobj.hpp>
namespace pt
{
    Model::Model(const std::filesystem::path &filename, bool byMyself)
    {
        std::vector<Triangle> triangles;
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
                triangles.push_back(Triangle(
                    positions[idx_v.x - 1], positions[idx_v.y - 1], positions[idx_v.z - 1],
                    normals[idx_vn.x - 1], normals[idx_vn.y - 1], normals[idx_vn.z - 1]));
            }
        }

        if (triangles.empty())
        {
            PBRT_ERROR("Model file {} is empty", std::filesystem::absolute(filename).string());
            return;
        }
        else
        {
            PBRT_INFO("Model file {} loaded with {} triangles", std::filesystem::absolute(filename).string(), triangles.size());
        }
        mBVH.Build(std::move(triangles));
    }

    Model::Model(const std::filesystem::path &filename)
    {
        auto result = rapidobj::ParseFile(filename, rapidobj::MaterialLibrary::Ignore());
        if (result.error)
        {
            PBRT_ERROR("Model file not found at {}", std::filesystem::absolute(filename).string());
            return;
        }
        std::vector<Triangle> triangles;
        for (const auto &shape : result.shapes)
        {
            size_t idx_offset = 0;
            for (size_t num_face_vertex : shape.mesh.num_face_vertices)
            {
                if (num_face_vertex == 3)
                {
                    auto idx = shape.mesh.indices[idx_offset];
                    glm::vec3 pos0{
                        result.attributes.positions[idx.position_index * 3 + 0],
                        result.attributes.positions[idx.position_index * 3 + 1],
                        result.attributes.positions[idx.position_index * 3 + 2]};

                    idx = shape.mesh.indices[idx_offset + 1];
                    glm::vec3 pos1{
                        result.attributes.positions[idx.position_index * 3 + 0],
                        result.attributes.positions[idx.position_index * 3 + 1],
                        result.attributes.positions[idx.position_index * 3 + 2]};

                    idx = shape.mesh.indices[idx_offset + 2];
                    glm::vec3 pos2{
                        result.attributes.positions[idx.position_index * 3 + 0],
                        result.attributes.positions[idx.position_index * 3 + 1],
                        result.attributes.positions[idx.position_index * 3 + 2]};

                    if (idx.normal_index >= 0)
                    {
                        idx = shape.mesh.indices[idx_offset];
                        glm::vec3 normal0{
                            result.attributes.normals[idx.normal_index * 3 + 0],
                            result.attributes.normals[idx.normal_index * 3 + 1],
                            result.attributes.normals[idx.normal_index * 3 + 2]};

                        idx = shape.mesh.indices[idx_offset + 1];
                        glm::vec3 normal1{
                            result.attributes.normals[idx.normal_index * 3 + 0],
                            result.attributes.normals[idx.normal_index * 3 + 1],
                            result.attributes.normals[idx.normal_index * 3 + 2]};

                        idx = shape.mesh.indices[idx_offset + 2];
                        glm::vec3 normal2{
                            result.attributes.normals[idx.normal_index * 3 + 0],
                            result.attributes.normals[idx.normal_index * 3 + 1],
                            result.attributes.normals[idx.normal_index * 3 + 2]};

                        triangles.push_back(Triangle(pos0, pos1, pos2, normal0, normal1, normal2));
                    }
                    else
                    {
                        triangles.push_back(Triangle(pos0, pos1, pos2));
                    }
                }
                idx_offset += num_face_vertex;
            }
        }

        if (triangles.empty())
        {
            PBRT_ERROR("Model file {} is empty", std::filesystem::absolute(filename).string());
            return;
        }
        else
        {
            PBRT_INFO("Model file {} loaded with {} triangles", std::filesystem::absolute(filename).string(), triangles.size());
        }

        mBVH.Build(std::move(triangles));
    }

    std::optional<HitInfo> Model::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        return mBVH.Intersect(ray, t_min, t_max);
    }
}