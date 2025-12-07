project(MultiDragon)

file(GLOB_RECURSE PRIMITIVE CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_executable(MultiDragon ${PRIMITIVE})

target_link_libraries(MultiDragon PRIVATE core)