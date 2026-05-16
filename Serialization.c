#include "Serialization.h"

#include "LayoutView.h"
#include "Util.h"
#include <cJSON/cJSON.h>
#include <sys/stat.h>

static void onFloat(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, float value)
{
    cJSON_AddNumberToObject(params->ctx, prop->name, value);
}

static void onValue(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGValue value)
{
    char str[128];

    switch (value.unit)
    {
    case YGUnitUndefined:
        break;
    case YGUnitPoint:
        cJSON_AddNumberToObject(params->ctx, prop->name, value.value);
        break;
    case YGUnitPercent:
        snprintf(str, sizeof(str), "%0.f%%", value.value);
        cJSON_AddStringToObject(params->ctx, prop->name, str);
        break;
    case YGUnitAuto:
        cJSON_AddStringToObject(params->ctx, prop->name, "auto");
        break;
    UNHANDLED_CASE();
    }
}

static void onAlign(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGAlign value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onPosition(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGPositionType value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onDirection(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGFlexDirection value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onWrap(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGWrap value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onEdgeValue(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGEdge edge, const char* edgeName, YGValue value)
{
    char str[128];
    cJSON* propObj = cJSON_GetObjectItem(params->ctx, prop->name);
    if (!propObj)
    {
        propObj = cJSON_AddObjectToObject(params->ctx, prop->name);
    }

    switch (value.unit)
    {
    case YGUnitUndefined:
        break;
    case YGUnitPoint:
        cJSON_AddNumberToObject(propObj, edgeName, value.value);
        break;
    case YGUnitPercent:
        snprintf(str, sizeof(str), "%0.f%%", value.value);
        cJSON_AddStringToObject(propObj, edgeName, str);
        break;
    case YGUnitAuto:
        cJSON_AddStringToObject(propObj, edgeName, "auto");
        break;
    UNHANDLED_CASE();
    }
}

static void onEdgeFloat(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGEdge edge, const char* edgeName, float value)
{
    cJSON* propObj = cJSON_GetObjectItem(params->ctx, prop->name);
    if (!propObj)
    {
        propObj = cJSON_AddObjectToObject(params->ctx, prop->name);
    }
    cJSON_AddNumberToObject(propObj, edgeName, value);
}

static void onJustify(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGJustify value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onOverflow(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGOverflow value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onDisplay(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, YGDisplay value, const char* valueName)
{
    cJSON_AddStringToObject(params->ctx, prop->name, valueName);
}

static void onString(EnumPropertiesParams* params, const Property* prop, YGNodeConstRef node, const char* value)
{
    cJSON_AddStringToObject(params->ctx, prop->name, value);
}

static void onChildNode(EnumPropertiesParams* params, YGNodeConstRef node, YGNodeConstRef child)
{
    cJSON* childObj = cJSON_CreateObject();

    EnumPropertiesParams childParams = {0};
    childParams.onFloat = onFloat;
    childParams.onValue = onValue;
    childParams.onAlign = onAlign;
    childParams.onPosition = onPosition;
    childParams.onDirection = onDirection;
    childParams.onWrap = onWrap;
    childParams.onEdgeValue = onEdgeValue;
    childParams.onEdgeFloat = onEdgeFloat;
    childParams.onJustify = onJustify;
    childParams.onOverflow = onOverflow;
    childParams.onDisplay = onDisplay;
    childParams.onString = onString;
    childParams.onChildNode = onChildNode;
    childParams.ctx = childObj;
    EnumNodeProperties(child, &childParams, false);

    NodeContext* ctx = YGNodeGetContext((YGNodeRef)child);
    cJSON_AddNumberToObject(childObj, "color", ctx->color);
    cJSON_AddNumberToObject(childObj, "text-color", ctx->textColor);

    cJSON* childrenArray = cJSON_GetObjectItem(params->ctx, "children");
    if (!childrenArray)
    {
        childrenArray = cJSON_AddArrayToObject(params->ctx, "children");
    }

    cJSON_AddItemToArray(childrenArray, childObj);
}

SerializationResult SaveLayout(const char* filename, YGNodeRef node, int nextIndex)
{
    cJSON* rootObj = cJSON_CreateObject();
    cJSON_AddNumberToObject(rootObj, "next-index", nextIndex);

    cJSON* layoutObj = cJSON_CreateObject();
    cJSON_AddItemToObject(rootObj, "layout", layoutObj);
    
    EnumPropertiesParams params = {0};
    params.onFloat = onFloat;
    params.onValue = onValue;
    params.onAlign = onAlign;
    params.onPosition = onPosition;
    params.onDirection = onDirection;
    params.onWrap = onWrap;
    params.onEdgeValue = onEdgeValue;
    params.onEdgeFloat = onEdgeFloat;
    params.onJustify = onJustify;
    params.onOverflow = onOverflow;
    params.onDisplay = onDisplay;
    params.onString = onString;
    params.onChildNode = onChildNode;
    params.ctx = layoutObj;
    EnumNodeProperties(node, &params, false);

    NodeContext* ctx = YGNodeGetContext(node);
    cJSON_AddNumberToObject(layoutObj, "color", ctx->color);
    cJSON_AddNumberToObject(layoutObj, "text-color", ctx->textColor);
    
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
            if(cJSON_IsNumber(value))
            {
                prop->setter.a(node, (YGAlign)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_POSITION:
            if (cJSON_IsNumber(value))
            {
                prop->setter.p(node, (YGPositionType)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_DIRECTION:
            if (cJSON_IsNumber(value))
            {
                prop->setter.d(node, (YGFlexDirection)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_WRAP:
            if (cJSON_IsNumber(value))
            {
                prop->setter.w(node, (YGWrap)cJSON_GetNumberValue(value));
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
            if (cJSON_IsNumber(value))
            {
                prop->setter.j(node, (YGJustify)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_OVERFLOW:
            if (cJSON_IsNumber(value))
            {
                prop->setter.o(node, (YGOverflow)cJSON_GetNumberValue(value));
            }
            break;
        case PROPERTY_TYPE_DISPLAY:
            if (cJSON_IsNumber(value))
            {
                prop->setter.disp(node, (YGDisplay)cJSON_GetNumberValue(value));
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
