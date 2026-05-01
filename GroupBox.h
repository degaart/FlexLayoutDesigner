#pragma once

#include <stdbool.h>
#include <Windows.h>

bool GroupBox_Init(HINSTANCE hInstance);
HWND GroupBox_Create(HINSTANCE hInstance, HWND hParent, const char* title, int x, int y, int w, int h);
