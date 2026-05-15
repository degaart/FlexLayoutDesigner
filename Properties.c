#include "Properties.h"

#define VALUE_PROP(P)       { .v = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .v = { .point = YGNodeStyleSet ## P, .percent = YGNodeStyleSet ## P ## Percent, .auto_ = YGNodeStyleSet ## P ## Auto } } 
#define FLOAT_PROP(P)       { .f = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .f = YGNodeStyleSet ## P }
#define DIRECTION_PROP(P)   { .d = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .d = YGNodeStyleSet ## P }
#define EDGE_VALUE_PROP(P)  { .ev = YGNodeStyleGet   ## P }, "YGNodeStyleSet" #P, { .ev = { .point = YGNodeStyleSet ## P, .percent = YGNodeStyleSet ## P ## Percent, .auto_ = YGNodeStyleSet ## P ## Auto } }
#define EDGE_FLOAT_PROP(P)  { .ef = YGNodeStyleGet   ## P }, "YGNodeStyleSet" #P, { .ef = YGNodeStyleSet ## P }
#define JUSTIFY_PROP(P)     { .j = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .j = YGNodeStyleSet ## P }
#define ALIGN_PROP(P)       { .a = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .a = YGNodeStyleSet ## P }
#define POSITION_PROP(P)    { .p = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .p = YGNodeStyleSet ## P }
#define WRAP_PROP(P)        { .w = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .w = YGNodeStyleSet ## P }
#define OVERFLOW_PROP(P)    { .o = YGNodeStyleGet    ## P }, "YGNodeStyleSet" #P, { .o = YGNodeStyleSet ## P }
#define DISPLAY_PROP(P)     { .disp = YGNodeStyleGet ## P }, "YGNodeStyleSet" #P, { .disp = YGNodeStyleSet ## P }

Property gProperties[] =
{
    { "width", PROPERTY_TYPE_VALUE, VALUE_PROP(Width) },
    { "height", PROPERTY_TYPE_VALUE, VALUE_PROP(Height) },
    { "min-width", PROPERTY_TYPE_VALUE, { .v = YGNodeStyleGetMinWidth }, "YGNodeStyleSetMinWidth", { .v = { .point =  YGNodeStyleSetMinWidth, .percent = YGNodeStyleSetMinWidthPercent, .auto_ = NULL } } },
    { "min-height", PROPERTY_TYPE_VALUE, { .v = YGNodeStyleGetMinHeight }, "YGNodeStyleSetMinHeight", { .v = { .point =  YGNodeStyleSetMinHeight, .percent = YGNodeStyleSetMinHeightPercent, .auto_ = NULL } } },
    { "max-width", PROPERTY_TYPE_VALUE, { .v = YGNodeStyleGetMaxWidth }, "YGNodeStyleSetMaxWidth", { .v = { .point =  YGNodeStyleSetMaxWidth, .percent = YGNodeStyleSetMaxWidthPercent, .auto_ = NULL } } },
    { "max-height", PROPERTY_TYPE_VALUE, { .v = YGNodeStyleGetMaxHeight }, "YGNodeStyleSetMaxHeight", { .v = { .point =  YGNodeStyleSetMaxHeight, .percent = YGNodeStyleSetMaxHeightPercent, .auto_ = NULL } } },
    { "flex", PROPERTY_TYPE_FLOAT, FLOAT_PROP(Flex) },
    { "flex-grow", PROPERTY_TYPE_FLOAT, FLOAT_PROP(FlexGrow) },
    { "flex-shrink", PROPERTY_TYPE_FLOAT, FLOAT_PROP(FlexShrink) },
    { "basis", PROPERTY_TYPE_VALUE, VALUE_PROP(FlexBasis) },
    { "position", PROPERTY_TYPE_EDGE_VALUE, { .ev = YGNodeStyleGetPosition }, "YGNodeStyleSetPosition", { .ev = { .point = YGNodeStyleSetPosition, .percent = YGNodeStyleSetPositionPercent, .auto_ = NULL } } },
    { "flex-direction", PROPERTY_TYPE_DIRECTION, DIRECTION_PROP(FlexDirection) },
    { "margin", PROPERTY_TYPE_EDGE_VALUE, EDGE_VALUE_PROP(Margin) },
    { "padding", PROPERTY_TYPE_EDGE_VALUE, { .ev = YGNodeStyleGetPadding }, "YGNodeStyleSetPadding", { .ev = { .point = YGNodeStyleSetPadding, .percent = YGNodeStyleSetPaddingPercent, .auto_ = NULL } } },
    { "border", PROPERTY_TYPE_EDGE_FLOAT, EDGE_FLOAT_PROP(Border) },
    { "justify-content", PROPERTY_TYPE_JUSTIFY, JUSTIFY_PROP(JustifyContent) },
    { "align-content", PROPERTY_TYPE_ALIGN, ALIGN_PROP(AlignContent) },
    { "align-items", PROPERTY_TYPE_ALIGN, ALIGN_PROP(AlignItems) },
    { "align-self", PROPERTY_TYPE_ALIGN, ALIGN_PROP(AlignSelf) },
    { "position-type", PROPERTY_TYPE_POSITION, POSITION_PROP(PositionType) },
    { "flex-wrap", PROPERTY_TYPE_WRAP, WRAP_PROP(FlexWrap) },
    { "overflow", PROPERTY_TYPE_OVERFLOW, OVERFLOW_PROP(Overflow) },
    { "display", PROPERTY_TYPE_DISPLAY, DISPLAY_PROP(Display) },
    { "aspect-ratio", PROPERTY_TYPE_FLOAT, FLOAT_PROP(AspectRatio) },
    { "HWND", PROPERTY_TYPE_STRING, { .str = NodeGetContext }, "SetFlexHWND",  { .str = NodeSetContext } },
    { "label", PROPERTY_TYPE_STRING, { .str = NodeGetLabel }, "SetFlexLabel",  { .str = NodeSetLabel } },
    { NULL },
};
