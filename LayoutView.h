#pragma once

#include <stdbool.h>
#include <Windows.h>
#include <yoga/Yoga.h>

bool LayoutView_Init(HINSTANCE hInstance);
HWND LayoutView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int width, int height);
void LayoutView_SetRootFlex(HWND hwnd, YGNodeRef rootFlex);
void LayoutView_SetSelectedFlex(HWND hwnd, YGNodeRef flex);

