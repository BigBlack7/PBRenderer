project(core)

file(GLOB_RECURSE CORE CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(core ${CORE})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(core PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_LEFT_HANDED WITH_DEBUG_INFO)

target_link_libraries(core
    PUBLIC
    glm
    rapidobj
    spdlog::spdlog
    sfmlGUI
    OpenEXR::OpenEXR
    stb
    pcg
)