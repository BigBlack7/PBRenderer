// core
#include <material/diffuseMaterial.hpp>
#include <material/specularMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <renderer/normalRenderer.hpp>
#include <renderer/PTRenderer.hpp>
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
    pbrt::Logger::Init();
    PBRT_INFO("PBRT Init!");

    pbrt::Film film(192 * 4, 108 * 4);
    pbrt::Camera camera{film, {-12.f, 5.f, -12.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pbrt::Model model("../../../assets/models/dragon_871k.obj");
    pbrt::Plane plane{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f},10.f};

    pbrt::Scene scene;
    pbrt::RNG rng{1234};
    for (int i = 0; i < 10000; i++)
    {
        glm::vec3 random_pos{rng.Uniform() * 100.f - 50.f, rng.Uniform() * 2.f, rng.Uniform() * 100.f - 50.f};
        float u = rng.Uniform();
        if (u < 0.9)
        {
            pbrt::Material *material;
            if (u > 0.5f)
            {
                material = new pbrt::SpecularMaterial({pbrt::RGB(202, 159, 117)});
            }
            else
            {
                material = new pbrt::DiffuseMaterial({pbrt::RGB(202, 159, 117)});
            }
            scene.AddShape(model, material, random_pos, {1.f, 1.f, 1.f}, {rng.Uniform() * 360.f, rng.Uniform() * 360.f, rng.Uniform() * 360.f});
        }
        else if (u < 0.95)
        {
            scene.AddShape(sphere, new pbrt::SpecularMaterial{{rng.Uniform(), rng.Uniform(), rng.Uniform()}}, random_pos, {0.4f, 0.4f, 0.4f});
        }
        else
        {
            random_pos.y += 6.f;
            pbrt::AreaLight *area_light = new pbrt::AreaLight{sphere, {0.95f * 5.f, 0.95f * 5.f, 5.f}, false};
            scene.AddAreaLight(area_light, new pbrt::DiffuseMaterial{});
        }
    }
    scene.AddShape(plane, new pbrt::DiffuseMaterial{pbrt::RGB(120, 204, 157)}, {0.f, -0.5f, 0.f});
    scene.Build();

    pbrt::PTRenderer pt{camera, scene};
    pt.Render("../../../PathTrace.ppm", 64);

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}