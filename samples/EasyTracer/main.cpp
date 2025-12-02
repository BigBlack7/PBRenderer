#include <iostream>
#include <glm/glm.hpp>
#include <rapidobj/rapidobj.hpp>
#include <spdlog/spdlog.h>
#include <logger.hpp>
#include <sfml/graphics.hpp>
#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImfInputFile.h>
#include <ImfFrameBuffer.h>
int main()
{
    core::Logger::Init();
    sf::RenderWindow window(
        sf::VideoMode({800, 600}),
        "My first Window",
        sf::Style::Resize);

    PBRT_INFO("Hello, PBRT!");

    glm::vec3 v(1.0f, 2.0f, 3.0f);
    PBRT_TRACE("v:{},{},{}", v.x, v.y, v.z);

    PBRT_INFO("Bye, PBRT!");
    return 0;
}