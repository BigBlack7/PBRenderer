project(PBRT)

file(GLOB_RECURSE MAIN CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_executable(PBRT ${MAIN})

target_link_libraries(PBRT PRIVATE core)
