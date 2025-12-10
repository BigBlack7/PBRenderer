project(EasyTracer)

file(GLOB_RECURSE MAIN CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_executable(EasyTracer ${MAIN})

target_link_libraries(EasyTracer PRIVATE core)