#include "Serialization.h"

#include "Util.h"
#include <cJSON/cJSON.h>

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
    EnumNodeProperties(child, &childParams);

    cJSON* childrenArray = cJSON_GetObjectItem(params->ctx, "children");
    if (!childrenArray)
    {
        childrenArray = cJSON_AddArrayToObject(params->ctx, "children");
    }

    cJSON_AddItemToArray(childrenArray, childObj);
}

SerializationResult SerializeNode(const char* filename, YGNodeRef node)
{
    cJSON* rootObj = cJSON_CreateObject();

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
    EnumNodeProperties(node, &params);

    char* serialized = cJSON_PrintUnformatted(rootObj);

    FILE* f = fopen(filename, "wb");
    fwrite(serialized, 1, strlen(serialized), f);
    fclose(f);

    free(serialized);
    cJSON_Delete(rootObj);

    return SERIALIZATION_OK;
}
