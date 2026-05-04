#include "Util.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

bool ParseFloat(const char* s, float* out)
{
    errno = 0;

    char* end;
    float val = strtof(s, &end);
    if (end == s)
    {
        return false;
    }
    if (*end != '\0')
    {
        return false;
    }
    if (errno == ERANGE)
    {
        return false;
    }
    if (out)
    {
        *out = val;
    }
    return true;
}

void CenterRect(RECT* inner, const RECT* outer)
{
    int innerW = inner->right - inner->left;
    int innerH = inner->bottom - inner->top;
    int outerW = outer->right - outer->left;
    int outerH = outer->bottom - outer->top;

    inner->left = outer->left + ((outerW - innerW) / 2);
    inner->top = outer->top + ((outerH - innerH) / 2);
    inner->right = inner->left + innerW;
    inner->bottom = inner->top + innerH;
}
