#pragma once

#include <stdbool.h>
#include <Windows.h>

bool CodeWindow_Init(HINSTANCE hInstance);
HWND CodeWindow_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
void CodeWindow_SetValue(HWND hControl, const char* value);
