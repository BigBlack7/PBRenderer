project(Cornellbox)

file(GLOB_RECURSE MAIN CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_executable(Cornellbox ${MAIN})

target_link_libraries(Cornellbox PRIVATE core)
