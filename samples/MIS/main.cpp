// core
#include <material/diffuseMaterial.hpp>
#include <material/conductorMaterial.hpp>
#include <material/groundMaterial.hpp>
#include <material/dielectricMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <presentation/previewer.hpp>
#include <renderer/BDPTRenderer.hpp>
#include <renderer/PTRenderer.hpp>
#include <shape/sphere.hpp>
#include <shape/model.hpp>
#include <shape/circle.hpp>
#include <shape/scene.hpp>
#include <utils/logger.hpp>
#include <utils/rgb.hpp>
#include <utils/rng.hpp>

int main()
{
    pbrt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pbrt::Film film(1920, 1080);
    glm::vec3 camera_pos = {0, 37.f, -61.f};
    pbrt::Camera camera{film, camera_pos, {0.f, 8.f, 0.f}, 16.f};
    pbrt::Scene scene{};

    // scene
    pbrt::Triangle triangles[] = {
        {{-17.f, 0.f, -1.5f}, {-17.f, 0.f, 1.5f}, {17.f, 0.f, 1.5f}},
        {{-17.f, 0.f, -1.5f}, {17.f, 0.f, 1.5f}, {17.f, 0.f, -1.5f}},
    };
    pbrt::Sphere light_sphere_1{{-15.f + 00.f / 3.f, 12.f, 8.f}, 2.f};
    pbrt::Sphere light_sphere_2{{-15.f + 30.f / 3.f, 12.f, 8.f}, 1.f};
    pbrt::Sphere light_sphere_3{{-15.f + 60.f / 3.f, 12.f, 8.f}, 0.5f};
    pbrt::Sphere light_sphere_4{{-15.f + 90.f / 3.f, 12.f, 8.f}, 0.1f};
    pbrt::AreaLight area_light_1{light_sphere_1, {1.f, 1.f, 1.f}, false};
    pbrt::AreaLight area_light_2{light_sphere_2, {4.f, 4.f, 4.f}, false};
    pbrt::AreaLight area_light_3{light_sphere_3, {16.f, 16.f, 16.f}, false};
    pbrt::AreaLight area_light_4{light_sphere_4, {400.f, 400.f, 400.f}, false};
    scene.AddAreaLight(&area_light_1, new pbrt::DiffuseMaterial{});
    scene.AddAreaLight(&area_light_2, new pbrt::DiffuseMaterial{});
    scene.AddAreaLight(&area_light_3, new pbrt::DiffuseMaterial{});
    scene.AddAreaLight(&area_light_4, new pbrt::DiffuseMaterial{});

    glm::vec3 light_pos_center = {0.f, 12.f, 8.f};
    float alphas[] = {0.4f, 0.25f, 0.16f, 0.04f};
    for (size_t i = 0; i < 4; i++)
    {
        float theta = glm::radians(i * 15.f);
        glm::vec3 center{0.f, 17.f * (1.f - glm::cos(theta)), 17.f * glm::sin(theta)};
        glm::vec3 normal = glm::normalize(glm::normalize(light_pos_center - center) + glm::normalize(camera_pos - center));
        float rotate_x = -glm::degrees(glm::acos(normal.y));
        pbrt::ConductorMaterial *surface_material = new pbrt::ConductorMaterial{{2.f, 2.f, 1.f}, {3.f, 3.f, 15.f}, alphas[i], alphas[i]};
        scene.AddShape(triangles[0], surface_material, center, {1.f, 1.f, 1.f}, {rotate_x, 0.f, 0.f});
        scene.AddShape(triangles[1], surface_material, center, {1.f, 1.f, 1.f}, {rotate_x, 0.f, 0.f});
    }
    pbrt::Circle ground{{0.f, -0.5f, 0.f}, {0.f, 1.f, 0.f}, 100.f};
    pbrt::Circle wall{{0.f, 0.f, 15.f}, {0.f, 0.f, -1.f}, 100.f};
    scene.AddShape(ground, new pbrt::GroundMaterial{{1.f, 1.f, 1.f}});
    scene.AddShape(wall, new pbrt::DiffuseMaterial{{1.f, 1.f, 1.f}});
    scene.AddInfiniteLight(new pbrt::InfiniteLight{{0.5f, 0.5f, 0.5f}});

    scene.Build();

    pbrt::BDPTRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../MIS.ppm", 64);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}
