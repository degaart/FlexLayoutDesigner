#include "Serialization.h"

#include "LayoutView.h"
#include "Util.h"
#include <cJSON/cJSON.h>
#include <sys/stat.h>

#define STRING_CASE(A, B, C)    \
    do                          \
    {                           \
        if (!strcmp(A, C))      \
        {                       \
            return B;           \
        }                       \
    } while (false)
    

YGAlign AlignFromString(const char* align)
{
    STRING_CASE(align, YGAlignAuto, "auto");
    STRING_CASE(align, YGAlignFlexStart, "flex-start");
    STRING_CASE(align, YGAlignCenter, "center");
    STRING_CASE(align, YGAlignFlexEnd, "flex-end");
    STRING_CASE(align, YGAlignStretch, "stretch");
    STRING_CASE(align, YGAlignBaseline, "baseline");
    STRING_CASE(align, YGAlignSpaceBetween, "space-between");
    STRING_CASE(align, YGAlignSpaceAround, "space-around");
    return -1;
}

YGDimension DimensionFromString(const char* dimension)
{
    STRING_CASE(dimension, YGDimensionWidth, "width");
    STRING_CASE(dimension, YGDimensionHeight, "height");
    return -1;
}

YGDisplay DisplayFromString(const char* display)
{
    STRING_CASE(display, YGDisplayFlex, "flex");
    STRING_CASE(display, YGDisplayNone, "none");
    return -1;
}

YGEdge EdgeFromString(const char* edge)
{
    STRING_CASE(edge, YGEdgeLeft, "left");
    STRING_CASE(edge, YGEdgeTop, "top");
    STRING_CASE(edge, YGEdgeRight, "right");
    STRING_CASE(edge, YGEdgeBottom, "bottom");
    STRING_CASE(edge, YGEdgeStart, "start");
    STRING_CASE(edge, YGEdgeEnd, "end");
    STRING_CASE(edge, YGEdgeHorizontal, "horizontal");
    STRING_CASE(edge, YGEdgeVertical, "vertical");
    STRING_CASE(edge, YGEdgeAll, "all");
    return -1;
}

YGFlexDirection FlexDirectionFromString(const char* flexDirection)
{
    STRING_CASE(flexDirection, YGFlexDirectionColumn, "column");
    STRING_CASE(flexDirection, YGFlexDirectionColumnReverse, "column-reverse");
    STRING_CASE(flexDirection, YGFlexDirectionRow, "row");
    STRING_CASE(flexDirection, YGFlexDirectionRowReverse, "row-reverse");
    return -1;
}

YGGutter GutterFromString(const char* gutter)
{
    STRING_CASE(gutter, YGGutterColumn, "column");
    STRING_CASE(gutter, YGGutterRow, "row");
    STRING_CASE(gutter, YGGutterAll, "all");
    return -1;
}

YGJustify JustifyFromString(const char* justify)
{
    STRING_CASE(justify, YGJustifyFlexStart, "flex-start");
    STRING_CASE(justify, YGJustifyCenter, "center");
    STRING_CASE(justify, YGJustifyFlexEnd, "flex-end");
    STRING_CASE(justify, YGJustifySpaceBetween, "space-between");
    STRING_CASE(justify, YGJustifySpaceAround, "space-around");
    STRING_CASE(justify, YGJustifySpaceEvenly, "space-evenly");
    return -1;
}

YGOverflow OverflowFromString(const char* overflow)
{
    STRING_CASE(overflow, YGOverflowVisible, "visible");
    STRING_CASE(overflow, YGOverflowHidden, "hidden");
    STRING_CASE(overflow, YGOverflowScroll, "scroll");
    return -1;
}

YGPositionType PositionTypeFromString(const char* positionType)
{
    STRING_CASE(positionType, YGPositionTypeStatic, "static");
    STRING_CASE(positionType, YGPositionTypeRelative, "relative");
    STRING_CASE(positionType, YGPositionTypeAbsolute, "absolute");
    return -1;
}

YGWrap WrapFromString(const char* wrap)
{
    STRING_CASE(wrap, YGWrapNoWrap, "no-wrap");
    STRING_CASE(wrap, YGWrapWrap, "wrap");
    STRING_CASE(wrap, YGWrapWrapReverse, "wrap-reverse");
    return -1;
}

static cJSON* SerializeNode(YGNodeConstRef node)
{
    char str[128];
    YGValue ygValue;
    cJSON* propObj;
    float fValue;

    cJSON* result = cJSON_CreateObject();
    for (Property* prop = gProperties; prop->name; prop++)
    {
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            fValue = prop->getter.f(node);
            if (!isnan(fValue))
            {
                cJSON_AddNumberToObject(result, prop->name, fValue);
            }
            break;
        case PROPERTY_TYPE_VALUE:
            ygValue = prop->getter.v(node);
            switch (ygValue.unit)
            {
            case YGUnitUndefined:
                break;
            case YGUnitPoint:
                if (!isnan(ygValue.value))
                {
                    cJSON_AddNumberToObject(result, prop->name, ygValue.value);
                }
                break;
            case YGUnitPercent:
                snprintf(str, sizeof(str), "%0.f%%", ygValue.value);
                cJSON_AddStringToObject(result, prop->name, str);
                break;
            case YGUnitAuto:
                cJSON_AddStringToObject(result, prop->name, "auto");
                break;
            UNHANDLED_CASE();
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            cJSON_AddStringToObject(result, prop->name, YGAlignToString(prop->getter.a(node)));
            break;
        case PROPERTY_TYPE_POSITION:
            cJSON_AddStringToObject(result, prop->name, YGPositionTypeToString(prop->getter.p(node)));
            break;
        case PROPERTY_TYPE_DIRECTION:
            cJSON_AddStringToObject(result, prop->name, YGFlexDirectionToString(prop->getter.d(node)));
            break;
        case PROPERTY_TYPE_WRAP:
            cJSON_AddStringToObject(result, prop->name, YGWrapToString(prop->getter.w(node)));
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            propObj = cJSON_GetObjectItem(result, prop->name);
            if (!propObj)
            {
                propObj = cJSON_AddObjectToObject(result, prop->name);
            }

            for (YGEdge edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
            {
                ygValue = prop->getter.ev(node, edge);
                switch (ygValue.unit)
                {
                case YGUnitUndefined:
                    break;
                case YGUnitPoint:
                    if (!isnan(ygValue.value))
                    {
                        cJSON_AddNumberToObject(propObj, YGEdgeToString(edge), ygValue.value);
                    }
                    break;
                case YGUnitPercent:
                    snprintf(str, sizeof(str), "%0.f%%", ygValue.value);
                    cJSON_AddStringToObject(propObj, YGEdgeToString(edge), str);
                    break;
                case YGUnitAuto:
                    cJSON_AddStringToObject(propObj, YGEdgeToString(edge), "auto");
                    break;
                UNHANDLED_CASE();
                }
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            propObj = cJSON_GetObjectItem(result, prop->name);
            if (!propObj)
            {
                propObj = cJSON_AddObjectToObject(result, prop->name);
            }

            for (YGEdge edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
            {
                fValue = prop->getter.ef(node, edge);
                if (!isnan(fValue))
                {
                    cJSON_AddNumberToObject(propObj, YGEdgeToString(edge), fValue);
                }
            }
            break;
        case PROPERTY_TYPE_JUSTIFY:
            cJSON_AddStringToObject(result, prop->name, YGJustifyToString(prop->getter.j(node)));
            break;
        case PROPERTY_TYPE_OVERFLOW:
            cJSON_AddStringToObject(result, prop->name, YGOverflowToString(prop->getter.o(node)));
            break;
        case PROPERTY_TYPE_DISPLAY:
            cJSON_AddStringToObject(result, prop->name, YGDisplayToString(prop->getter.disp(node)));
            break;
        case PROPERTY_TYPE_STRING:
            prop->getter.str(node, str, sizeof(str));
            if (strlen(str))
            {
                cJSON_AddStringToObject(result, prop->name, str);
            }
            break;
        UNHANDLED_CASE();
        }
    }

    cJSON* childrenArray = cJSON_GetObjectItem(result, "children");
    if (!childrenArray)
    {
        childrenArray = cJSON_AddArrayToObject(result, "children");
    }

    unsigned n = YGNodeGetChildCount((YGNodeRef)node);
    for (unsigned i = 0; i < n; i++)
    {
        cJSON* childObj = SerializeNode(YGNodeGetChild((YGNodeRef)node, i));
        cJSON_AddItemToArray(childrenArray, childObj);
    }

    NodeContext* ctx = YGNodeGetContext((YGNodeRef)node);
    if (ctx)
    {
        cJSON_AddNumberToObject(result, "color", ctx->color);
        cJSON_AddNumberToObject(result, "text-color", ctx->textColor);
    }
    return result;
}

SerializationResult SaveLayout(const char* filename, YGNodeRef node, int nextIndex)
{
    cJSON* rootObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(rootObj, "next-index", nextIndex);

    cJSON* obj = SerializeNode(node);
    cJSON_AddItemToObject(rootObj, "layout", obj);

    char* serialized = cJSON_PrintUnformatted(rootObj);

    FILE* f = fopen(filename, "wb");
    fwrite(serialized, 1, strlen(serialized), f);
    fclose(f);

    free(serialized);
    cJSON_Delete(rootObj);

    return SERIALIZATION_OK;
}

static YGNodeRef DeserializeObject(cJSON* obj)
{
    char* strValue;
    cJSON* subObj;
    cJSON* childrenArr;
    int childCount;
    int i;
    YGNodeRef child;

    YGNodeRef node = YGNodeNew();
    NodeContext* ctx = calloc(1, sizeof(NodeContext));
    if (cJSON_IsNumber(cJSON_GetObjectItem(obj, "color")))
    {
        ctx->color = cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "color"));
    }
    if (cJSON_IsNumber(cJSON_GetObjectItem(obj, "text-color")))
    {
        ctx->textColor = cJSON_GetNumberValue(cJSON_GetObjectItem(obj, "text-color"));
    }
    YGNodeSetContext(node, ctx);

    for (Property* prop = gProperties; prop->name; prop++)
    {
        cJSON* value = cJSON_GetObjectItem(obj, prop->name);
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            if (cJSON_IsNumber(value))
            {
                prop->setter.f(node, (float)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_VALUE:
            if (cJSON_IsNumber(value))
            {
                prop->setter.v.point(node, (float)cJSON_GetNumberValue(value));
            }
            else if (cJSON_IsString(value))
            {
                strValue = cJSON_GetStringValue(value);
                if (!strcmp(strValue, "auto"))
                {
                    if (prop->setter.v.auto_)
                    {
                        prop->setter.v.auto_(node);
                    }
                }
                else if (strlen(strValue) && strValue[strlen(strValue) - 1] == '%')
                {
                    prop->setter.v.percent(node, atof(strValue));
                }
                free(strValue);
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            if(cJSON_IsString(value))
            {
                prop->setter.a(node, AlignFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_POSITION:
            if (cJSON_IsString(value))
            {
                prop->setter.p(node, PositionTypeFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_DIRECTION:
            if (cJSON_IsString(value))
            {
                prop->setter.d(node, FlexDirectionFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_WRAP:
            if (cJSON_IsString(value))
            {
                prop->setter.w(node, WrapFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            if (cJSON_IsObject(value))
            {
                for (YGEdge edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
                {
                    subObj = cJSON_GetObjectItem(value, YGEdgeToString(edge));
                    if (cJSON_IsNumber(subObj) && prop->setter.ev.point)
                    {
                        prop->setter.ev.point(node, edge, (float)cJSON_GetNumberValue(subObj));
                    }
                    else if (cJSON_IsString(subObj))
                    {
                        strValue = cJSON_GetStringValue(subObj);
                        if (strlen(strValue) && strValue[strlen(strValue) - 1] == '%' && prop->setter.ev.percent)
                        {
                            prop->setter.ev.percent(node, edge, (float)atof(strValue));
                        }
                        else if (!strcmp(strValue, "auto") && prop->setter.ev.auto_)
                        {
                            prop->setter.ev.auto_(node, edge);
                        }
                    }
                }
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            if (cJSON_IsObject(value))
            {
                for (YGEdge edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
                {
                    subObj = cJSON_GetObjectItem(value, YGEdgeToString(edge));
                    if (cJSON_IsNumber(subObj))
                    {
                        prop->setter.ef(node, edge, cJSON_GetNumberValue(subObj));
                    }
                }
            }
            break;
        case PROPERTY_TYPE_JUSTIFY:
            if (cJSON_IsString(value))
            {
                prop->setter.j(node, JustifyFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_OVERFLOW:
            if (cJSON_IsString(value))
            {
                prop->setter.o(node, OverflowFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_DISPLAY:
            if (cJSON_IsString(value))
            {
                prop->setter.disp(node, DisplayFromString(cJSON_GetStringValue(value)));
            }
            break;
        case PROPERTY_TYPE_STRING:
            if (cJSON_IsString(value))
            {
                strValue = cJSON_GetStringValue(value);
                prop->setter.str(node, strValue);
                free(strValue);
            }
            break;
        UNHANDLED_CASE();
        }
    }

    childrenArr = cJSON_GetObjectItem(obj, "children");
    if (cJSON_IsArray(childrenArr))
    {
        childCount = cJSON_GetArraySize(childrenArr);
        for (i = 0; i < childCount; i++)
        {
            subObj = cJSON_GetArrayItem(childrenArr, i);
            if (cJSON_IsObject(subObj))
            {
                child = DeserializeObject(subObj);
                if (!child)
                {
                    continue;
                }

                YGNodeInsertChild(node, child, YGNodeGetChildCount(node));
            }
        }
    }

    return node;
}

SerializationResult LoadLayout(const char* filename, YGNodeRef* node, int* nextIndex)
{
    struct stat st;
    int ret = stat(filename, &st);
    if (ret == -1)
    {
        return SERIALIZATION_OPEN_FAILED;
    }

    char* buffer = malloc(st.st_size + 1);

    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        free(buffer);
        return SERIALIZATION_OPEN_FAILED;
    }

    size_t readBytes = fread(buffer, 1, st.st_size, f);
    if (readBytes != st.st_size)
    {
        return SERIALIZATION_READ_FAILED;
    }
    buffer[readBytes] = '\0';

    fclose(f);


    cJSON* rootObj = cJSON_Parse(buffer);
    free(buffer);
    if (!rootObj)
    {
        return SERIALIZATION_PARSE_FAILED;
    }

    if (!cJSON_IsNumber(cJSON_GetObjectItem(rootObj, "next-index")))
    {
        cJSON_Delete(rootObj);
        return SERIALIZATION_INVALID_OBJECT;
    }
    *nextIndex = cJSON_GetNumberValue(cJSON_GetObjectItem(rootObj, "next-index"));

    cJSON* layoutObj = cJSON_GetObjectItem(rootObj, "layout");
    if (!layoutObj)
    {
        cJSON_Delete(rootObj);
        return SERIALIZATION_INVALID_OBJECT;
    }

    *node = DeserializeObject(layoutObj);

    return SERIALIZATION_OK;
}
