// core
#include <light/envLight.hpp>
#include <material/diffuseMaterial.hpp>
#include <material/conductorMaterial.hpp>
#include <material/groundMaterial.hpp>
#include <material/dielectricMaterial.hpp>
#include <material/iridescentMaterial.hpp>
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
    pbrt::Camera camera{film, {-20.f, 7.f, 0.f}, {0.f, 2.f, 0.f}, 45.f};

    // models
    pbrt::Model teapot("../../../assets/models/teapot.obj");
    pbrt::Model ajax("../../../assets/models/ajax.obj");
    pbrt::Circle ground{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 100.f};
    pbrt::Scene scene{};

    scene.AddShape(teapot, new pbrt::DielectricMaterial{pbrt::RGB(255, 255, 255), 1.2f, 0.1f, 0.1f}, {-5.f, 0.4f, 4.5f}, {1.f, 1.f, 1.f}, {0.f, -10.f, 0.f});
    scene.AddShape(teapot, new pbrt::ConductorMaterial{{0.1f, 1.2f, 1.8f}, {5.f, 2.5f, 2.f}, 0.2f, 0.2f}, {-5.f, 0.4f, -4.5f}, {1.f, 1.f, 1.f});
    // scene.AddShape(ajax, new pbrt::IridescentMaterial{0.8f, 2.f, 3.f, 0.2f, 0.3f, 0.3f}, {-5.f, 0.4f, -4.5f}, {7.f, 7.f, 7.f}, {0.f, 90.f, 0.f});
    scene.AddShape(ground, new pbrt::GroundMaterial{pbrt::RGB(225, 225, 225)});

    // light
    pbrt::Image env_map("../../../assets/hdris/puresky04.exr");
    scene.AddInfiniteLight(new pbrt::EnvLight{&env_map});
    scene.Build();

    pbrt::MISRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../teapot.exr", 64);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}