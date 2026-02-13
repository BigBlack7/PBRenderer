// core
#include <light/envLight.hpp>
#include <material/diffuseMaterial.hpp>
#include <material/specularMaterial.hpp>
#include <material/conductorMaterial.hpp>
#include <material/groundMaterial.hpp>
#include <material/dielectricMaterial.hpp>
#include <presentation/film.hpp>
#include <presentation/camera.hpp>
#include <presentation/previewer.hpp>
#include <renderer/MISRenderer.hpp>
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
    pbrt::Camera camera{film, {0.f, 1.25f, -6.f}, {0.f, 1.95f, 0.f}, 45.f};
    pbrt::Scene scene{};

    // scene
    pbrt::Model model("../../../assets/models/buddha.obj");
    scene.AddShape(model, new pbrt::SpecularMaterial{pbrt::RGB(241, 191, 79)}, {-3.f, 1.75f, 0.f}, {4.f, 4.f, 4.f});
    scene.AddShape(model, new pbrt::ConductorMaterial{{0.47f, 0.47f, 0.47f}, {2.95f, 2.95f, 2.95f}, 0.8f, 0.2f}, {-1.f, 1.75f, 0.f}, {4.f, 4.f, 4.f});
    scene.AddShape(model, new pbrt::DielectricMaterial{pbrt::RGB(200, 162, 200), 1.8f, 0.1f, 0.3f}, {1.f, 1.75f, 0.f}, {4.f, 4.f, 4.f});
    scene.AddShape(model, new pbrt::DiffuseMaterial{pbrt::RGB(255, 128, 128)}, {3.f, 1.75f, 0.f}, {4.f, 4.f, 4.f});

    pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    scene.AddShape(sphere, new pbrt::SpecularMaterial{{1.f, 1.f, 1.f}}, {0.f, 3.75f, 3.f});
    pbrt::Circle ground{{0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 100.f};
    scene.AddShape(ground, new pbrt::GroundMaterial{pbrt::RGB(155, 191, 255)});

    pbrt::Image env_map("../../../assets/hdris/puresky04.exr");
    scene.AddInfiniteLight(new pbrt::EnvLight{&env_map});

    scene.Build();

    pbrt::MISRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../MIS.exr", 32);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}