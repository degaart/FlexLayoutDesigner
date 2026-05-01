#pragma once

#include <Windows.h>
#include <stdbool.h>

#define SVM_UPDATESCROLL (WM_USER+1)

bool ScrollView_Init(HINSTANCE hInstance);
HWND ScrollView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h);
