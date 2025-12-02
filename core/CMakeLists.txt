project(core)

file(GLOB_RECURSE CORE CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(core ${CORE})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(core
    PUBLIC
    glm
    rapidobj
    spdlog::spdlog
    sfml-graphics
    OpenEXR::OpenEXR
)