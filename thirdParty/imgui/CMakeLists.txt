add_library(sfmlGUI STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp
    imgui-SFML.cpp
)

target_include_directories(sfmlGUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sfmlGUI
    PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
)
