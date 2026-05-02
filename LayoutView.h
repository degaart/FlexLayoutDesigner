#pragma once

#include <stdbool.h>
#include <Windows.h>

struct flex_item;

bool LayoutView_Init(HINSTANCE hInstance);
HWND LayoutView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int width, int height);
void LayoutView_SetRootFlex(HWND hwnd, struct flex_item* rootFlex);
void LayoutView_SetSelectedFlex(HWND hwnd, struct flex_item* flex);

