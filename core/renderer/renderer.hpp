#pragma once
#include "presentation/camera.hpp"
#include "shape/scene.hpp"

namespace pbrt
{
#define DEFINE_RENDERER(Name)                                                           \
    class Name##Renderer : public Renderer                                              \
    {                                                                                   \
    private:                                                                            \
        glm::vec3 RenderPixel(const glm::ivec3 &pixel_coord) override;                  \
                                                                                        \
    public:                                                                             \
        Name##Renderer(Camera &camera, const Scene &scene) : Renderer(camera, scene) {} \
    };

    class Renderer
    {
        friend class Previewer;

    protected:
        Camera &mCamera;
        const Scene &mScene;

    public:
        Renderer(Camera &camera, const Scene &scene) : mCamera(camera), mScene(scene) {}

        void Render(const std::filesystem::path &filename, size_t spp);

        virtual glm::vec3 RenderPixel(const glm::ivec3 &pixel_coord) = 0;
    };
}