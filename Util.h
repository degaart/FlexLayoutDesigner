#pragma once

#include <stdbool.h>
#include <Windows.h>

bool ParseFloat(const char* s, float* out);
void CenterRect(RECT* inner, const RECT* outer);
