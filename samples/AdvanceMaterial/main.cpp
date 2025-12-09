// core
#include <material/diffuseMaterial.hpp>
#include <material/conductorMaterial.hpp>
#include <material/groundMaterial.hpp>
#include <material/dielectricMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <renderer/PTRenderer.hpp>
#include <shape/sphere.hpp>
#include <shape/model.hpp>
#include <shape/plane.hpp>
#include <shape/scene.hpp>
#include <utils/logger.hpp>
#include <utils/rgb.hpp>
#include <utils/rng.hpp>

int main()
{
    pbrt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pbrt::Film film(192 * 5, 108 * 5);
    pbrt::Camera camera{film, {-10.f, 1.5f, 0.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pbrt::Model model("../../../assets/models/dragon_871k.obj");
    pbrt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};

    pbrt::Scene scene;
    pbrt::RNG rng{1234};
    for (int i = -3; i <= 3; i++)
    {
        scene.AddShape(sphere, new pbrt::DielectricMaterial{{1.f, 1.f, 1.f}, 1.f + 0.2f * (i + 3), (3.f - i) / 18.f, (3.f - i) / 6.f}, {0.f, 0.5f, i * 2.f}, {0.8f, 0.8f, 0.8f});
    }
    for (int i = -3; i <= 3; i++)
    {
        glm::vec3 c = pbrt::RGB::GenerateHeatMap((i + 3.f) / 6.f);
        scene.AddShape(sphere, new pbrt::ConductorMaterial{glm::vec3(2.f - c * 2.f), glm::vec3(2.f + c * 3.f), (3.f - i) / 6.f, (3.f - i) / 18.f}, {0.f, 2.5f, i * 2.f}, {0.8f, 0.8f, 0.8f});
    }

    scene.AddShape(model, new pbrt::DielectricMaterial{pbrt::RGB(128, 191, 131), 1.8f, 0.5f, 0.5f}, {-5.f, 0.4f, 1.5f}, {2.f, 2.f, 2.f});
    scene.AddShape(model, new pbrt::ConductorMaterial{{0.1f, 1.2f, 1.8f}, {5.f, 2.5f, 2.f}, 0.f, 0.f}, {-5.f, 0.4f, -1.5f}, {2.f, 2.f, 2.f});

    scene.AddShape(plane, new pbrt::GroundMaterial{pbrt::RGB(120, 204, 157)}, {0.f, -0.5f, 0.f});
    auto *light = new pbrt::DiffuseMaterial{{0.2f, 1.f, 1.f}};
    light->SetEmission({0.95f, 0.95f, 1.f});
    scene.AddShape(plane, light, {0.f, 10.f, 0.f});
    scene.Build();

    pbrt::PTRenderer pt{camera, scene};
    pt.Render("../../../AdvanceMaterial.ppm", 64);

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}