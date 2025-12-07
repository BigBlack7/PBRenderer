// core
#include <material/diffuseMaterial.hpp>
#include <material/specularMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <renderer/normalRenderer.hpp>
#include <renderer/RTRenderer.hpp>
#include <renderer/debugRenderer.hpp>
#include <shape/sphere.hpp>
#include <shape/model.hpp>
#include <shape/plane.hpp>
#include <shape/scene.hpp>
#include <utils/logger.hpp>
#include <utils/rgb.hpp>

int main()
{
    pbrt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pbrt::Film film(192 * 4, 108 * 4);
    pbrt::Camera camera{film, {-3.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pbrt::Model model("../../../assets/models/dragon_871k.obj");
    pbrt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};

    pbrt::Scene scene;
    scene.AddShape(model, new pbrt::DiffuseMaterial{pbrt::RGB(202, 159, 117)}, {0.f, 0.f, 0.f});

    auto *red = new pbrt::DiffuseMaterial{pbrt::RGB(255, 128, 128)};
    red->SetEmission({1.f, 0.5f, 0.5f});
    scene.AddShape(sphere, red, {0.f, 0.f, 2.5f});
    auto *purple = new pbrt::DiffuseMaterial{pbrt::RGB(128, 128, 255)};
    purple->SetEmission({0.5f, 0.5f, 1.f});
    scene.AddShape(sphere, purple, {0.f, 0.f, -2.5f});
    scene.AddShape(sphere, new pbrt::SpecularMaterial{{1.f, 1.f, 1.f}}, {3.f, 0.5f, -2.f});
    scene.AddShape(plane, new pbrt::DiffuseMaterial{pbrt::RGB(120, 204, 157)}, {0.f, -0.5f, 0.f});
    scene.Build();

    pbrt::RTRenderer rt{camera, scene};
    rt.Render("../../../RayTracing.ppm", 64);

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}