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

YGNodeRef CreateFlex(float width, float height, HWND hWnd)
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

    if (hWnd)
    {
        FlexData* data = calloc(1, sizeof(FlexData));
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

float ToLinear(unsigned component)
{
    float s = (float)component / 255.0f;
    if (s <= 0.04045)
    {
        return s / 12.92f;
    }

    return powf((s + 0.055f) / 1.055f, 2.4f);
}

uint32_t InvertColor(uint32_t color)
{
    unsigned r = GetRValue(color);
    unsigned g = GetGValue(color);
    unsigned b = GetBValue(color);

    float luminance = 
        (0.2126f * ToLinear(r)) +
        (0.7152f * ToLinear(g)) +
        (0.0722f * ToLinear(b));
    if (luminance > 0.179f)
    {
        return 0x00000000;
    }
    return 0x00FFFFFF;
}

void EnumNodeProperties(YGNodeConstRef node, EnumPropertiesParams* params)
{
    float fValue;
    YGValue ygValue;
    const char* valueName;
    char sValue[128];

    assert(node != NULL);
    for (Property* prop = gProperties; prop->name; prop++)
    {
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            fValue = prop->getter.f(node);
            if (fcomp(fValue, prop->default_.f))
            {
                params->onFloat(params, prop, node, fValue);
            }
            break;
        case PROPERTY_TYPE_VALUE:
            ygValue = prop->getter.v(node);
            if (vcomp(ygValue, prop->default_.v))
            {
                params->onValue(params, prop, node, ygValue);
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            if (prop->getter.a(node) != prop->default_.a)
            {
                switch (prop->getter.a(node))
                {
                ENUM_STRING_CASE(valueName, YGAlignAuto);
                ENUM_STRING_CASE(valueName, YGAlignFlexStart);
                ENUM_STRING_CASE(valueName, YGAlignCenter);
                ENUM_STRING_CASE(valueName, YGAlignFlexEnd);
                ENUM_STRING_CASE(valueName, YGAlignStretch);
                ENUM_STRING_CASE(valueName, YGAlignBaseline);
                ENUM_STRING_CASE(valueName, YGAlignSpaceBetween);
                ENUM_STRING_CASE(valueName, YGAlignSpaceAround);
                UNHANDLED_CASE();
                }
                params->onAlign(params, prop, node, prop->getter.a(node), valueName);
            }
            break;
        case PROPERTY_TYPE_POSITION:
            if (prop->getter.p(node) != prop->default_.p)
            {
                switch (prop->getter.p(node))
                {
                ENUM_STRING_CASE(valueName, YGPositionTypeStatic);
                ENUM_STRING_CASE(valueName, YGPositionTypeRelative);
                ENUM_STRING_CASE(valueName, YGPositionTypeAbsolute);
                UNHANDLED_CASE();
                }
                params->onPosition(params, prop, node, prop->getter.p(node), valueName);
            }
            break;
        case PROPERTY_TYPE_DIRECTION:
            if (prop->getter.d(node) != prop->default_.d)
            {
                switch (prop->getter.d(node))
                {
                ENUM_STRING_CASE(valueName, YGFlexDirectionColumn);
                ENUM_STRING_CASE(valueName, YGFlexDirectionColumnReverse);
                ENUM_STRING_CASE(valueName, YGFlexDirectionRow);
                ENUM_STRING_CASE(valueName, YGFlexDirectionRowReverse);
                UNHANDLED_CASE();
                }
                params->onDirection(params, prop, node, prop->getter.d(node), valueName);
            }
            break;
        case PROPERTY_TYPE_WRAP:
            if (prop->getter.w(node) != prop->default_.w)
            {
                switch (prop->getter.w(node))
                {
                ENUM_STRING_CASE(valueName, YGWrapNoWrap);
                ENUM_STRING_CASE(valueName, YGWrapWrap);
                ENUM_STRING_CASE(valueName, YGWrapWrapReverse);
                UNHANDLED_CASE();
                }
                params->onWrap(params, prop, node, prop->getter.w(node), valueName);
            }
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                ygValue = prop->getter.ev(node, i);
                if (vcomp(ygValue, prop->default_.ev[i]))
                {
                    switch (i)
                    {
                    ENUM_STRING_CASE(valueName, YGEdgeLeft);
                    ENUM_STRING_CASE(valueName, YGEdgeRight);
                    ENUM_STRING_CASE(valueName, YGEdgeTop);
                    ENUM_STRING_CASE(valueName, YGEdgeBottom);
                    UNHANDLED_CASE();
                    }

                    params->onEdgeValue(params, prop, node, i, valueName, ygValue);
                }
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                switch (i)
                {
                ENUM_STRING_CASE(valueName, YGEdgeLeft);
                ENUM_STRING_CASE(valueName, YGEdgeRight);
                ENUM_STRING_CASE(valueName, YGEdgeTop);
                ENUM_STRING_CASE(valueName, YGEdgeBottom);
                UNHANDLED_CASE();
                }

                if (fcomp(prop->getter.ef(node, i), prop->default_.ef[i]))
                {
                    params->onEdgeFloat(params, prop, node, i, valueName, prop->getter.ef(node, i));
                }
            }
            break;
        case PROPERTY_TYPE_JUSTIFY:
            if (prop->getter.j(node) != prop->default_.j)
            {
                switch (prop->getter.j(node))
                {
                ENUM_STRING_CASE(valueName, YGJustifyFlexStart);
                ENUM_STRING_CASE(valueName, YGJustifyCenter);
                ENUM_STRING_CASE(valueName, YGJustifyFlexEnd);
                ENUM_STRING_CASE(valueName, YGJustifySpaceBetween);
                ENUM_STRING_CASE(valueName, YGJustifySpaceAround);
                ENUM_STRING_CASE(valueName, YGJustifySpaceEvenly);
                UNHANDLED_CASE();
                }
                params->onJustify(params, prop, node, prop->getter.j(node), valueName);
            }
            break;
        case PROPERTY_TYPE_OVERFLOW:
            if (prop->getter.o(node) != prop->default_.o)
            {
                switch (prop->getter.o(node))
                {
                ENUM_STRING_CASE(valueName, YGOverflowVisible);
                ENUM_STRING_CASE(valueName, YGOverflowHidden);
                ENUM_STRING_CASE(valueName, YGOverflowScroll);
                UNHANDLED_CASE();
                }
                params->onOverflow(params, prop, node, prop->getter.o(node), valueName);
            }
            break;
        case PROPERTY_TYPE_DISPLAY:
            if (prop->getter.disp(node) != prop->default_.disp)
            {
                switch (prop->getter.disp(node))
                {
                ENUM_STRING_CASE(valueName, YGDisplayFlex);
                ENUM_STRING_CASE(valueName, YGDisplayNone);
                UNHANDLED_CASE();
                }
                params->onDisplay(params, prop, node, prop->getter.disp(node), valueName);
            }
            break;
        case PROPERTY_TYPE_STRING:
            prop->getter.str(node, sValue, sizeof(sValue));
            if (strcmp(sValue, prop->default_.str))
            {
                params->onString(params, prop, node, sValue);
            }
            break;
        default:
            assert(!"Unhandled case value");
        }
    }

    unsigned childCount = YGNodeGetChildCount((YGNodeRef)node);
    for (unsigned i = 0; i < childCount; i++)
    {
        YGNodeRef child = YGNodeGetChild((YGNodeRef)node, i);
        params->onChildNode(params, node, child);
    }
}

