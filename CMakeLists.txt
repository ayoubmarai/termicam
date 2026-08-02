cmake_minimum_required(VERSION 3.16)

project(program LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)

file(GLOB SRCS CONFIGURE_DEPENDS "src/*.cpp")
foreach(SRC IN LISTS SRCS)
    message("SOURCE: ${SRC}")
endforeach()

file(GLOB HEADERS CONFIGURE_DEPENDS "include/*.hpp")
foreach(SRC IN LISTS HEADERS)
    message("HEADERS: ${SRC}")
endforeach()

file(GLOB IMGUISRCS "imgui/*.cpp" "imgui/*.h")
foreach(SRC IN LISTS IMGUISRCS)
    message("IMGUISOURCE: ${SRC}")
endforeach()

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    highgui
    imgcodecs
    videoio
)
pkg_check_modules(NCURSES REQUIRED ncurses)


add_executable(program
    ${SRCS}
    ${HEADERS}
    ${IMGUISRCS}
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(program PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${NCURSES_INCLUDE_DIRS}
    "include"
    "imgui/backends"
    "imgui"
)

target_link_libraries(program PRIVATE
    ${OpenCV_LIBS}
    ${NCURSES_LIBRARIES}
    SDL2::SDL2
    SDL2::SDL2main
)

target_compile_options(program PRIVATE
    ${NCURSES_CFLAGS_OTHER}
)
