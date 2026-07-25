cmake_minimum_required(VERSION 3.16)

project(program LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put the executable in ./dist
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    highgui
    imgcodecs
    videoio
)
pkg_check_modules(NCURSES REQUIRED ncurses)

add_executable(program
    src/main.cpp
)

target_include_directories(program PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${NCURSES_INCLUDE_DIRS}
)

target_link_libraries(program PRIVATE
    ${OpenCV_LIBS}
    ${NCURSES_LIBRARIES}
)

target_compile_options(program PRIVATE
    ${NCURSES_CFLAGS_OTHER}
)