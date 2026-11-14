# termicam

Termicam is a C++ terminal based webcam renderer that converts live camera footage into colored ASCII art. It uses OpenCV for image processing and ncurses for terminal rendering, with an SDL2/ImGui window for adjusting image settings

## Installation

### Ubuntu

```bash
sudo apt update
sudo apt install cmake build-essential libopencv-dev libsdl2-dev libncurses-dev
```

### Arch-based Linux

```bash
sudo pacman -S cmake opencv sdl2 ncurses
```

### Windows

This project uses ncurses for terminal rendering, so it isn't compatible with Windows out of the box.

Building this on Windows requires either WSL, or some fiddling around with MSYS/MinGW that I rather not get into here.

## Build

Clone the repository and initialize the build:

```bash
git clone https://github.com/ayoubmarai/termicam.git
cd termicam

cmake -S . -B build
cmake --build build
```

The executable is generated in `dist/`

> **Note:** I made and testedt this using Clang, but other compatible C++20 compilers like GCC should work fine

## Usage

Connect a webcam and run

```bash
./dist/termicam
```

The terminal displays the live ASCII rendering of your very beautiful face. A small configuration window allows you to adjust foreground and background brightness and saturation, modify color offsets, and reset ncurses color pairs.