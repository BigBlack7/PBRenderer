#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace pt
{
    void Scene::AddShape(const Shape &shape, const Material &material, const glm::vec3 &position, const glm::vec3 &scale, const glm::vec3 &rotate)
    {
        // 对象空间到世界空间的变换矩阵
        // 约定向量都为列向量, 矩阵乘法即变换顺序从右往左
        glm::mat4 world_from_object =
            glm::translate(glm::mat4(1.f), position) *
            glm::rotate(glm::mat4(1.f), glm::radians(rotate.z), {0.f, 0.f, 1.f}) *
            glm::rotate(glm::mat4(1.f), glm::radians(rotate.y), {0.f, 1.f, 0.f}) *
            glm::rotate(glm::mat4(1.f), glm::radians(rotate.x), {1.f, 0.f, 0.f}) *
            glm::scale(glm::mat4(1.f), scale);

        __shapeInfos__.push_back(ShapeInfo{shape, material, world_from_object, glm::inverse(world_from_object)});
    }

    std::optional<HitInfo> Scene::Intersect(const Ray &ray, float t_min, float t_max) const
    {
        std::optional<HitInfo> closest_hit_info = {};
        const ShapeInfo *closest_shape_info = nullptr;
        for (const auto &info : __shapeInfos__)
        {
            auto ray_object = ray.ObjectFromWorld(info.__objectFromWorld__);
            auto hit_info = info.__shape__.Intersect(ray_object, t_min, t_max);
            if (hit_info.has_value())
            {
                closest_hit_info = hit_info;
                t_max = hit_info->__t__;
                closest_shape_info = &info;
            }
        }

        if (closest_shape_info)
        {
            closest_hit_info->__hitPoint__ = closest_shape_info->__worldFromObject__ * glm::vec4(closest_hit_info->__hitPoint__, 1.f);
            closest_hit_info->__normal__ = glm::normalize(glm::vec3(glm::transpose(closest_shape_info->__objectFromWorld__) * glm::vec4(closest_hit_info->__normal__, 0.f)));
            closest_hit_info->__material__ = &closest_shape_info->__material__;
        }
        return closest_hit_info;
    }
}