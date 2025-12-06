// core
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <renderer/normalRenderer.hpp>
#include <renderer/RTRenderer.hpp>
#include <shape/sphere.hpp>
#include <shape/model.hpp>
#include <shape/plane.hpp>
#include <shape/scene.hpp>
#include <thread/threadPool.hpp>
#include <utils/logger.hpp>
#include <utils/rgb.hpp>
#include <utils/progress.hpp>

int main()
{
    pt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pt::Film film(192, 108);
    pt::Camera camera{film, {0.f, 0.f, -3.6f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pt::Model model("../../../assets/models/simple_dragon.obj");
    pt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};

    pt::Scene scene;
    scene.AddShape(model, {pt::RGB(202, 159, 117)}, {0.f, 0.f, 0.f}, {1.f, 3.f, 2.f}, {0.f, -90.f, 0.f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, false, pt::RGB{255, 128, 128}}, {2.5f, 0.f, 0.f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, false, pt::RGB{128, 128, 255}}, {-2.5f, 0.f, 0.f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, true}, {-2.f, 0.5f, 3.f});
    scene.AddShape(plane, {}, {0.f, -0.5f, 0.f});

    pt::NormalRenderer nRenderer{camera, scene};
    nRenderer.Render("../../../NormalTest.ppm", 1);
    film.Clear();
    pt::RTRenderer rtRenderer{camera, scene};
    rtRenderer.Render("../../../RayTraceTest.ppm", 4);

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}