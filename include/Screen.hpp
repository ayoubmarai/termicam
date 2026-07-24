#pragma once
#include <iostream>
#include <vector>

class Screen
{
public:
    Screen() = default;

    void init(); // init all the screen stuff
private:
    std::vector<std::vector<short>> color_cache;

    void draw_pixel(int x, int y, short c1, short c2);
};