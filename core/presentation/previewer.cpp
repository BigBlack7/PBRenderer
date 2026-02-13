#include "previewer.hpp"
#include "renderer/normalRenderer.hpp"
#include "renderer/debugRenderer.hpp"
#include "thread/threadPool.hpp"
#include "utils/logger.hpp"

namespace pbrt
{
    Previewer::Previewer(Renderer &renderer, float fps) : mRenderer(renderer), mFPS(fps)
    {
        auto &film = mRenderer.mCamera.GetFilm();
        mFilmResolution = {film.GetWidth(), film.GetHeight()};

        mRenderModes.push_back(&mRenderer);
        mRenderModes.push_back(new NormalRenderer(mRenderer.mCamera, mRenderer.mScene));
        DEBUG_INFO(mRenderModes.push_back(new BTCRenderer(mRenderer.mCamera, mRenderer.mScene)));
        DEBUG_INFO(mRenderModes.push_back(new TTCRenderer(mRenderer.mCamera, mRenderer.mScene)));

        mScale = 1.f;
    }

    bool Previewer::Preview()
    {
        // 创建窗口
        mWindow = std::make_shared<sf::RenderWindow>(
            sf::VideoMode(sf::Vector2u(mFilmResolution.x, mFilmResolution.y)), // 窗口大小
            "PBRT",                                                            // 窗口标题
            sf::Style::Titlebar,                                               // 窗口样式
            sf::State::Windowed);                                              // 窗口状态: 无边框窗口化

        mTexture = std::make_shared<sf::Texture>(sf::Vector2u(mFilmResolution.x, mFilmResolution.y)); // 纹理大小
        mTexture->setSmooth(true);                                                                    // 在低分辨率下会更加平滑
        mSprite = std::make_shared<sf::Sprite>(*mTexture);                                            // 让精灵持有纹理

        SetResolution(0.1f); // 默认1/10的分辨率
        auto &camera = mRenderer.mCamera;
        auto &film = camera.GetFilm();
        // 窗口事件
        bool mouse_grabbed = false;                          // 鼠标是否被捕获
        sf::Vector2i window_center{mWindow->getSize() / 2u}; // 窗口中心
        float dt = 0.f;
        bool render_final_result = false;
        // 窗口循环
        while (mWindow->isOpen())
        {
            while (auto event = mWindow->pollEvent())
            {
                // 键盘事件
                if (auto key_released = event->getIf<sf::Event::KeyReleased>())
                {
                    if (key_released->scancode == sf::Keyboard::Scancode::Escape) // keyboard: ESC 退出
                    {
                        mWindow->close();
                    }
                    else if (key_released->scancode == sf::Keyboard::Scancode::Enter) // keyboard: ENTER 渲染
                    {
                        render_final_result = true; // 渲染最终结果
                        mWindow->close();
                    }
                    else if (key_released->scancode == sf::Keyboard::Scancode::Tab) // keyboard: TAB 切换渲染模式
                    {
                        mRenderModeIndex = (mRenderModeIndex + 1) % mRenderModes.size(); // 切换渲染模式
                        mCurrentSPP = 0;                                                 // 重置采样次数
                    }
                    else if (key_released->scancode == sf::Keyboard::Scancode::NumpadPlus) // keypad: + 增加fps
                    {
                        mFPS += 1;
                        PBRT_INFO("FPS: {}", mFPS);
                    }
                    else if (key_released->scancode == sf::Keyboard::Scancode::NumpadMinus) // keypad: - 减少fps
                    {
                        // 限制 mFPS 不小于 1
                        if (mFPS > 1)
                        {
                            mFPS -= 1;
                            PBRT_INFO("FPS: {}", mFPS);
                        }
                        else
                        {
                            PBRT_WARN("FPS cannot be less than 1!");
                        }
                    }
                    else if (key_released->scancode == sf::Keyboard::Scancode::CapsLock) // keyboard: CapsLock 切换鼠标捕获
                    {
                        mouse_grabbed = !mouse_grabbed;                  // 切换鼠标捕获
                        mWindow->setMouseCursorGrabbed(mouse_grabbed);   // 设置鼠标捕获
                        mWindow->setMouseCursorVisible(!mouse_grabbed);  // 设置鼠标被捕获后的可见性
                        sf::Mouse::setPosition(window_center, *mWindow); // 设置鼠标位置
                    }
                }
                // 鼠标事件
                else if (auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
                {
                    if (!mouse_grabbed) // 鼠标未被捕获, 不处理
                        continue;
                    auto delta = mouseMoved->position - window_center; // 鼠标移动量
                    if (delta.x == 0 && delta.y == 0)                  // 鼠标未移动, 不处理
                        continue;
                    camera.Turn({delta.x, delta.y});
                    mCurrentSPP = 0;                                 // 重置采样次数, 移动鼠标后在下一帧重新渲染新的图像
                    sf::Mouse::setPosition(window_center, *mWindow); // 设置鼠标位置
                }
                else if (auto *mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) // 鼠标滚轮事件
                {
                    if (!mouse_grabbed) // 鼠标未被捕获, 不处理
                        continue;
                    camera.Zoom(mouseWheel->delta); // 滚轮滚动量
                    mCurrentSPP = 0;                // 重置采样次数, 移动鼠标后在下一帧重新渲染新的图像
                }
            }
            if (mouse_grabbed)
            {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
                {
                    camera.Move(dt, Direction::Forward);
                    mCurrentSPP = 0;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
                {
                    camera.Move(dt, Direction::Backward);
                    mCurrentSPP = 0;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                {
                    camera.Move(dt, Direction::Left);
                    mCurrentSPP = 0;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                {
                    camera.Move(dt, Direction::Right);
                    mCurrentSPP = 0;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
                {
                    camera.Move(dt, Direction::Up);
                    mCurrentSPP = 0;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
                {
                    camera.Move(dt, Direction::Down);
                    mCurrentSPP = 0;
                }
            }

            auto start = std::chrono::high_resolution_clock::now();            // 记录开始时间
            RendererFrame();                                                   // 渲染一帧
            auto duration = std::chrono::high_resolution_clock::now() - start; // 计算耗时

            auto buffer = film.GenerateRGBABuffer(); // 生成RGBABuffer
            mTexture->update(buffer.data());         // 更新纹理

            mWindow->clear();
            mWindow->draw(*mSprite);
            mWindow->display();
            // 在当前帧绘制完成后再调整分辨率绘制新帧
            dt = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() * 0.001f;
            AutoAdjustResolution(dt);
        }
        film.SetResolution(mFilmResolution.x, mFilmResolution.y); // 恢复原始分辨率
        return render_final_result;
    }

    void Previewer::RendererFrame()
    {
        auto *renderer = mRenderModes[mRenderModeIndex];
        size_t render_spp = mRenderModeIndex == 0 ? 4 : 1; // 只有路径追踪模式需要多采样
        auto &film = mRenderer.mCamera.GetFilm();
        if (mCurrentSPP == 0) // 第一次渲染
        {
            film.Clear(); // 清空
        }
        MasterThreadPool.ParallelFor(film.GetWidth(), film.GetHeight(), [&](size_t x, size_t y)
                                     {
                                         for (size_t i = mCurrentSPP; i < mCurrentSPP + render_spp; i++)
                                         {
                                             film.AddSample(x, y, renderer->RenderPixel({x, y, i}));
                                         }
                                         // end
                                     });
        MasterThreadPool.Wait();
        mCurrentSPP += render_spp;
    }

    void Previewer::SetResolution(float scale)
    {
        scale = glm::clamp(scale, 0.f, 1.f);
        if (scale == mScale)
        {
            return;
        }
        mScale = scale;
        // 调整分辨率
        glm::ivec2 new_resolution = glm::vec2(mFilmResolution) * mScale;
        if (new_resolution.x == 0)
            new_resolution.x = 1;
        if (new_resolution.y == 0)
            new_resolution.y = 1;
        mRenderer.mCamera.GetFilm().SetResolution(new_resolution.x, new_resolution.y);
        auto res = mTexture->resize(sf::Vector2u(new_resolution.x, new_resolution.y));
        // 调整精灵, 精灵持有的纹理此时小于窗口大小, 所以需要缩放
        mSprite->setTexture(*mTexture);
        mSprite->setScale(sf::Vector2f(static_cast<float>(mFilmResolution.x) / new_resolution.x, static_cast<float>(mFilmResolution.y) / new_resolution.y));
        mCurrentSPP = 0; // 重置采样次数, 清空胶片
    }

    void Previewer::AutoAdjustResolution(float dt)
    {
        // 根据当前渲染一帧的时间和当前fps的差值动态调整分辨率
        float expected_dt = 1.f / mFPS;
        if (glm::abs(expected_dt - dt) / expected_dt > 0.4f)
        {
            float new_scale = mScale * (1.f + 0.1f * (glm::sqrt(expected_dt / dt) - 1.f));
            SetResolution(new_scale);
        }
    }
}