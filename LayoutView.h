#pragma once

#include <stdbool.h>
#include <Windows.h>

bool LayoutView_Init(HINSTANCE hInstance);
HWND LayoutView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int width, int height);
