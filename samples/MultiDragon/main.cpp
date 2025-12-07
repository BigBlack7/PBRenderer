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
#include <utils/logger.hpp>
#include <utils/rgb.hpp>
#include <utils/rng.hpp>

int main()
{
    pt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pt::Film film(192 * 4, 108 * 4);
    pt::Camera camera{film, {-12.f, 5.f, -12.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pt::Model model("../../../assets/models/dragon_871k.obj");
    pt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};

    pt::Scene scene;
    pt::RNG rng{1234};
    for (int i = 0; i < 10000; i++)
    {
        glm::vec3 random_pos{rng.Uniform() * 100.f - 50.f, rng.Uniform() * 2.f, rng.Uniform() * 100.f - 50.f};
        float u = rng.Uniform();
        if (u < 0.9)
        {
            scene.AddShape(model, {pt::RGB(202, 159, 117), rng.Uniform() > 0.5f}, random_pos, {1, 1, 1}, {rng.Uniform() * 360.f, rng.Uniform() * 360.f, rng.Uniform() * 360.f});
        }
        else if(u<0.95)
        {
            scene.AddShape(sphere, {{rng.Uniform(), rng.Uniform(), rng.Uniform()}, true}, random_pos,{0.4f,0.4f,0.4f});
        }
        else
        {
            random_pos.y += 6.f;
            scene.AddShape(sphere, {{1.f, 1.f, 1.f}, false, {rng.Uniform() * 4.f, rng.Uniform() * 4.f, rng.Uniform() * 4.f}}, random_pos);
        }
    }
    scene.AddShape(plane, {pt::RGB(120, 204, 157)}, {0.f, -0.5f, 0.f});
    scene.Build();

    pt::BTCRenderer btc{camera, scene};
    btc.Render("../../../BTC.ppm", 1);

    pt::TTCRenderer ttc{camera, scene};
    ttc.Render("../../../TTC.ppm", 1);

    pt::NormalRenderer normal{camera, scene};
    normal.Render("../../../Normal.ppm", 1);

    pt::RTRenderer rt{camera, scene};
    rt.Render("../../../RayTrace.ppm", 64);

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}