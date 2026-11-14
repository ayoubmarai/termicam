#pragma once
#include <iostream>
#include <ncurses.h>

#include "utils.hpp"

extern short colorCache[256];
void n_render(SharedState* state);