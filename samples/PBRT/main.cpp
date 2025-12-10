#include <SFML/Graphics.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({800, 600}),
        "ImGui + SFML 3 test");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock delta_clock;

    while (window.isOpen())
    {
        // ✅ SFML 3 事件循环
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, delta_clock.restart());

        ImGui::Begin("Hello ImGui (SFML 3)");
        ImGui::Text("SFML 3 + ImGui-SFML is working.");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        window.clear(sf::Color{30, 30, 30});
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
