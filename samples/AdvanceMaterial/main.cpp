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
    pbrt::Camera camera{film, {-10.f, 1.5f, 0.f}, {0.f, 0.f, 0.f}, 45.f};

    // models
    pbrt::Sphere sphere{{0.f, 0.f, 0.f}, 1.f};
    pbrt::Model model("../../../assets/models/dragon_871k.obj");

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
    // Add iridescent material spheres with varying parameters
    for (int i = -3; i <= 3; i++)
    {
        // Parameters: Dinc (film thickness), eta2 (film IOR), eta3 (base IOR), kappa3 (extinction), alpha_x, alpha_z
        float dinc = 0.1f + (i + 3) * 0.05f;  // Film thickness from 0.1 to 0.4 micrometers
        scene.AddShape(sphere, new pbrt::IridescentMaterial{dinc, 2.0f, 3.0f, 0.0f, 0.01f, 0.01f}, {0.f, 4.5f, i * 2.f}, {0.8f, 0.8f, 0.8f});
    }

    scene.AddShape(model, new pbrt::DielectricMaterial{pbrt::RGB(221, 180, 221), 1.6f, 0.2f, 0.2f}, {-5.f, 0.4f, 1.5f}, {2.f, 2.f, 2.f});
    scene.AddShape(model, new pbrt::ConductorMaterial{{0.1f, 1.2f, 1.8f}, {5.f, 2.5f, 2.f}, 0.2f, 0.2f}, {-5.f, 0.4f, -1.5f}, {2.f, 2.f, 2.f});

    // light
    pbrt::Image env_mnap("../../../assets/hdris/puresky04.exr");
    scene.AddInfiniteLight(new pbrt::EnvLight{&env_mnap});
    scene.Build();

    pbrt::MISRenderer mis{camera, scene};
    pbrt::Previewer previewer(mis, 1);
    if (previewer.Preview())
    {
        mis.Render("../../../AdvanceMaterial.exr", 128);
    }

    PBRT_INFO("PBRT Shutdown!");
    return 0;
}