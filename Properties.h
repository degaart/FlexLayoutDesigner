#pragma once

#include <yoga/Yoga.h>
#include <Windows.h>

typedef union PropertyValue
{
    float           f;
    YGValue         v;
    YGAlign         a;
    YGFlexDirection d;
    YGWrap          w;
    YGPositionType  p;
    YGValue         ev[4];    /* YGEdgeLeft, YGEdgeTop, YGEdgeRight, YGEdgeBottom, in that order */
    float           ef[4];    /* YGEdgeLeft, YGEdgeTop, YGEdgeRight, YGEdgeBottom, in that order */
    YGJustify       j;
    YGOverflow      o;
    YGDisplay       disp;
    char            str[128];
} PropertyValue;

typedef union PropertyGetter
{
    float           (*f)(YGNodeConstRef);
    YGValue         (*v)(YGNodeConstRef);
    YGAlign         (*a)(YGNodeConstRef);
    YGFlexDirection (*d)(YGNodeConstRef);
    YGWrap          (*w)(YGNodeConstRef);
    YGPositionType  (*p)(YGNodeConstRef);
    YGValue         (*ev)(YGNodeConstRef, YGEdge);
    float           (*ef)(YGNodeConstRef, YGEdge);
    YGJustify       (*j)(YGNodeConstRef);
    YGOverflow      (*o)(YGNodeConstRef);
    YGDisplay       (*disp)(YGNodeConstRef);
    void            (*str)(YGNodeConstRef, char*, size_t);
} PropertyGetter;

typedef union PropertySetter
{
    void (*f)(YGNodeRef, float);
    struct
    {
        void (*point)(YGNodeRef, float);
        void (*percent)(YGNodeRef, float);
        void (*auto_)(YGNodeRef);
    } v;
    void (*a)(YGNodeRef, YGAlign);
    void (*d)(YGNodeRef, YGFlexDirection);
    void (*w)(YGNodeRef, YGWrap);
    void (*p)(YGNodeRef, YGPositionType);
    struct
    {
        void (*point)(YGNodeRef, YGEdge, float);
        void (*percent)(YGNodeRef, YGEdge, float);
        void (*auto_)(YGNodeRef, YGEdge);
    } ev;
    void (*ef)(YGNodeRef, YGEdge, float);
    void (*j)(YGNodeRef, YGJustify);
    void (*o)(YGNodeRef, YGOverflow);
    void (*disp)(YGNodeRef, YGDisplay);
    void (*str)(YGNodeRef, const char*);
} PropertySetter;

typedef enum PropertyType
{
    PROPERTY_TYPE_FLOAT,
    PROPERTY_TYPE_VALUE,
    PROPERTY_TYPE_ALIGN,
    PROPERTY_TYPE_POSITION,
    PROPERTY_TYPE_DIRECTION,
    PROPERTY_TYPE_WRAP,
    PROPERTY_TYPE_EDGE_VALUE,
    PROPERTY_TYPE_EDGE_FLOAT,
    PROPERTY_TYPE_JUSTIFY,
    PROPERTY_TYPE_OVERFLOW,
    PROPERTY_TYPE_DISPLAY,
    PROPERTY_TYPE_STRING,
} PropertyType;

typedef struct Property
{
    const char* name;
    PropertyType type;
    PropertyGetter getter;
    const char* setterName;
    PropertySetter setter;
    PropertyValue default_;
    HWND hLabel;
    HWND hControl;
    YGNodeRef flex;
    YGNodeRef labelFlex;
    YGNodeRef controlFlex;
} Property;

extern Property gProperties[];

void NodeGetContext(YGNodeConstRef, char*, size_t);
void NodeSetContext(YGNodeRef, const char*);
void NodeGetLabel(YGNodeConstRef, char*, size_t);
void NodeSetLabel(YGNodeRef, const char*);
