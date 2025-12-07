#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace pbrt
{
    void Scene::AddShape(const Shape &shape, const Material *material, const glm::vec3 &position, const glm::vec3 &scale, const glm::vec3 &rotate)
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
        return __sceneBVH__.Intersect(ray, t_min, t_max);
    }
}