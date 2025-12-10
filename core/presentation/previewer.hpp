#pragma once
#include "renderer/renderer.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>

namespace pbrt
{
    class Previewer
    {

    private:
        Renderer &mRenderer;
        std::vector<Renderer *> mRenderModes;
        size_t mRenderModeIndex = 0;

        float mScale;
        float mFPS;
        glm::ivec2 mFilmResolution;

        size_t mCurrentSPP = 0;

        std::shared_ptr<sf::RenderWindow> mWindow;
        std::shared_ptr<sf::Texture> mTexture;
        std::shared_ptr<sf::Sprite> mSprite;

    private:
        void RendererFrame();
        void SetResolution(float scale);
        void AutoAdjustResolution(float dt);

    public:
        Previewer(Renderer &renderer, float fps = 30);
        ~Previewer();
        bool Preview();

        sf::RenderWindow &GetWindow() const
        {
            return *mWindow;
        }
    };
}