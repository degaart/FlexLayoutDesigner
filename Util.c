#include "Util.h"

#include "Trace.h"
#include <stdlib.h>
#include <errno.h>

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

static void DumpSingleFlex(YGNodeRef flex, int indent)
{
    char prefix[32] = {0};
    strcpy(prefix, "+ ");
    for (int i = 0; i < indent; i++)
    {
        prefix[i+2] = ' ';
    }

    TRACE("%s %d %d %d %d",
          prefix,
          roundf(YGNodeLayoutGetLeft(flex)), roundf(YGNodeLayoutGetTop(flex)),
          roundf(YGNodeLayoutGetWidth(flex)), roundf(YGNodeLayoutGetHeight(flex)));

    int childCount = YGNodeGetChildCount(flex);
    for (int i = 0; i < childCount; i++)
    {
        DumpSingleFlex(YGNodeGetChild(flex, i), indent + 1);
    }
}

void DumpFlex(const char* title, YGNodeRef flex)
{
    TRACE("*** %s ***", title);
    DumpSingleFlex(flex, 0);
}

YGNodeRef CreateFlex(float width, float height, const char* name, HWND hWnd)
{
    YGNodeRef node = YGNodeNew();
    if (!isnan(width))
    {
        YGNodeStyleSetWidth(node, width);
    }
    if (!isnan(height))
    {
        YGNodeStyleSetHeight(node, height);
    }

    if (name || hWnd)
    {
        FlexData* data = calloc(1, sizeof(FlexData));
        if (name)
        {
            strcpy_s(data->label, sizeof(data->label), name);
        }
        data->hwnd = hWnd;
        YGNodeSetContext(node, data);
    }

    return node;
}

void DestroyFlex(YGNodeRef flex)
{
    if (!flex)
    {
        return;
    }

    FlexData* data = YGNodeGetContext(flex);
    if (data)
    {
        free(data);
    }

    unsigned childCount = YGNodeGetChildCount(flex);
    for (unsigned i = 0; i < childCount; i++)
    {
        DestroyFlex(YGNodeGetChild(flex, i));
    }
    YGNodeFree(flex);
}

void LayoutFlex(YGNodeRef flex, float originX, float originY)
{
    unsigned childCount = YGNodeGetChildCount(flex);
    for (unsigned i = 0; i < childCount; i++)
    {
        YGNodeRef node = YGNodeGetChild(flex, i);
        FlexData* data = YGNodeGetContext(node);
        
        float left = YGNodeLayoutGetLeft(node) + originX;
        float top = YGNodeLayoutGetTop(node) + originY;
        if (data && data->hwnd)
        {
            char className[128];
            GetClassName(data->hwnd, className, sizeof(className));

            float width = YGNodeLayoutGetWidth(node);

            float height = YGNodeLayoutGetHeight(node);
            if (!strcmp(className, "ComboBox"))
            {
                height = 128.0f;
            }
            else if (!strcmp(className, "EdgeValueView"))
            {
                height = 128.0f;
            }

            SetWindowPos(data->hwnd, NULL,
                roundf(left), roundf(top),
                roundf(width), roundf(height),
                SWP_NOZORDER);
        }

        LayoutFlex(node, left, top);
    }
}

uint32_t GenerateColor(int index)
{
    if (index == 0)
    {
        return 0xFFFFFF;
    }
    return ((uint32_t)index * 2654435761u) >> 8;
}

int fcomp(float a, float b)
{
    if (isnan(a))
    {
        if (isnan(b))
        {
            return 0;
        }
        return -1;
    }
    else if (isnan(b))
    {
        return 1;
    }

    float diff = a - b;
    if (fabsf(diff) < 1e-9)
    {
        return 0;
    }
    else if (diff > 0.0f)
    {
        return 1;
    }
    return -1;
}

int vcomp(YGValue a, YGValue b)
{
    if (a.unit != b.unit)
    {
        return a.unit - b.unit;
    }
    return fcomp(a.value, b.value);
}



