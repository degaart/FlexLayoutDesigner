#pragma once

#include <Windows.h>
#include <stdbool.h>

bool ScrollView_Init(HINSTANCE hInstance);
HWND ScrollView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
void ScrollView_UpdateScroll(HWND hwnd);

