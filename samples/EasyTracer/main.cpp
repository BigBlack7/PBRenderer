// core
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <renderer/normalRenderer.hpp>
#include <renderer/RTRenderer.hpp>
#include <renderer/debugRenderer.hpp>
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

    pt::Film film(192 * 4, 108 * 4);
    pt::Camera camera{film, {-3.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pt::Model model("../../../assets/models/dragon_871k.obj");
    pt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};

    pt::Scene scene;
    scene.AddShape(model, {pt::RGB(202, 159, 117)}, {0.f, 0.f, 0.f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, false, pt::RGB{255, 128, 128}}, {0.f, 0.f, 2.5f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, false, pt::RGB{128, 128, 255}}, {0.f, 0.f, -2.5f});
    scene.AddShape(sphere, {{1.f, 1.f, 1.f}, true}, {3.f, 0.5f, -2.f});
    scene.AddShape(plane, {}, {0.f, -0.5f, 0.f});

    
    pt::BTCRenderer btc{camera, scene};
    btc.Render("../../../BTC.ppm", 1);
    
    pt::TTCRenderer ttc{camera, scene};
    ttc.Render("../../../TTC.ppm", 1);
    
    pt::BDRenderer bd{camera, scene};
    bd.Render("../../../BD.ppm", 1);
    
    pt::NormalRenderer normal{camera, scene};
    normal.Render("../../../Normal.ppm", 1);

    pt::RTRenderer rt{camera, scene};
    rt.Render("../../../RayTrace.ppm", 64);
    
    PBRT_INFO("PBRT Shutdown!");
    return 0;
}