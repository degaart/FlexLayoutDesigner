#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <stdbool.h>
#include <yoga/Yoga.h>

typedef struct NodeContext
{
    char label[128];
    uint32_t color;
    uint32_t textColor;
    char context[128];
    HTREEITEM hTreeItem;
    HWND hTree;
} NodeContext;

bool LayoutView_Init(HINSTANCE hInstance);
HWND LayoutView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int width, int height);
void LayoutView_SetRootFlex(HWND hwnd, YGNodeRef rootFlex);
void LayoutView_SetSelectedFlex(HWND hwnd, YGNodeRef flex);

