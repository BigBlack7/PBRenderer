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
    pbrt::Camera camera{film, {-20.f, 5.f, 0.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pbrt::Model model("../../../assets/models/teapot.obj");
    pbrt::Model bunny("../../../assets/models/bunny.obj");
    pbrt::Scene scene{};

    scene.AddShape(model, new pbrt::DielectricMaterial{pbrt::RGB(255, 255, 255), 1.4f, 0.2f, 0.2f}, {-5.f, 0.4f, 4.5f}, {1.f, 1.f, 1.f}, {0.f, -10.f, 0.f});
    scene.AddShape(bunny, new pbrt::IridescentMaterial{0.8f, 2.f, 3.f, 0.2f, 0.3f, 0.3f}, {-5.f, 0.4f, -4.5f}, {3.f, 3.f, 3.f}, {0.f, -70.f, 0.f});

    // light
    pbrt::Image env_mnap("../../../assets/hdris/puresky04.exr");
    scene.AddInfiniteLight(new pbrt::EnvLight{&env_mnap});
    scene.Build();

    pbrt::MISRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../teapot.exr", 1024);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}