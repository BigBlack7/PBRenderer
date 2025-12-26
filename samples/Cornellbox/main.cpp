// core
#include <light/envLight.hpp>
#include <material/diffuseMaterial.hpp>
#include <material/conductorMaterial.hpp>
#include <material/groundMaterial.hpp>
#include <material/dielectricMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <presentation/previewer.hpp>
#include <renderer/MISRenderer.hpp>
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
    pbrt::Camera camera{film, {-5.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, 30.f};

    // shapes
    pbrt::Model green_left("../../../assets/models/cornellbox/left.obj");
    pbrt::Model red_right("../../../assets/models/cornellbox/right.obj");
    pbrt::Model white_back("../../../assets/models/cornellbox/back.obj");
    pbrt::Model white_top("../../../assets/models/cornellbox/top.obj");
    pbrt::Model white_bottom("../../../assets/models/cornellbox/bottom.obj");
    pbrt::Model box("../../../assets/models/cornellbox/box.obj");
    pbrt::Model bunny("../../../assets/models/bunny.obj");
    pbrt::Circle circle({0.f, 1.999f, 0.f}, {0.f, -1.f, 0.f}, 0.3f);
    pbrt::Sphere sphere({0.f, 0.f, 0.f}, 0.35f);

    // scene
    pbrt::Scene scene{};
    scene.AddShape(green_left, new pbrt::DiffuseMaterial{pbrt::RGB(0, 255, 0)}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, -90.f, 0.f});
    scene.AddShape(red_right, new pbrt::DiffuseMaterial{pbrt::RGB(255, 0, 0)}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, -90.f, 0.f});
    scene.AddShape(white_back, new pbrt::DiffuseMaterial{pbrt::RGB(255, 255, 255)}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, -90.f, 0.f});
    scene.AddShape(white_top, new pbrt::DiffuseMaterial{pbrt::RGB(255, 255, 255)}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, -90.f, 0.f});
    scene.AddShape(white_bottom, new pbrt::DiffuseMaterial{pbrt::RGB(255, 255, 255)}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}, {0.f, -90.f, 0.f});

    // original box
    // scene.AddShape(box, new pbrt::DiffuseMaterial{pbrt::RGB(255, 255, 255)}, {0.6f, 1.92f, 0.52f}, {0.018f, 0.07f, 0.04f}); // big one
    // scene.AddShape(box, new pbrt::DiffuseMaterial{pbrt::RGB(255, 255, 255)}, {-0.3f, 0.87f, 0.08f}, {0.032f, 0.032f, 0.032f}, {0.f, -54.f, 0.f}); // small one

    // glass sphere and metal box
    // scene.AddShape(box, new pbrt::ConductorMaterial{{0.1f, 1.2f, 1.8f}, {5.f, 2.5f, 2.f}, 0.5f, 0.3f}, {0.6f, 1.92f, 0.52f}, {0.018f, 0.07f, 0.04f});
    // scene.AddShape(sphere, new pbrt::DielectricMaterial{pbrt::RGB(255, 255, 255), 1.4f, 0.2f, 0.2f}, {-0.3f, 0.35f, -0.3f});

    // bunny
    scene.AddShape(bunny, new pbrt::DielectricMaterial{pbrt::RGB(221, 180, 221), 1.61f, 0.2f, 0.2f}, {0.f, 0.2f, 0.f}, {0.7f, 0.7f, 0.7f}, {0.f, -70.f, 0.f});

    // light
    pbrt::Image env_mnap("../../../assets/hdris/puresky03.exr");
    scene.AddInfiniteLight(new pbrt::EnvLight{&env_mnap});

    auto *light = new pbrt::AreaLight{circle, pbrt::RGB(255, 220, 150), false};
    scene.AddAreaLight(light, new pbrt::DiffuseMaterial{});
    scene.Build();

    pbrt::MISRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../cornellbox03.ppm", 1024);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}