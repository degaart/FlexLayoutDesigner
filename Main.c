#include "CodeStream.h"
#include "CodeWindow.h"
#include "EdgeFloatView.h"
#include "EdgeValueView.h"
#include "GroupBox.h"
#include "LayoutView.h"
#include "Properties.h"
#include "ScrollView.h"
#include "Serialization.h"
#include "Trace.h"
#include "Util.h"
#include "ValueView.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <ShlObj.h>
#include <combaseapi.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>
#include <yoga/Yoga.h>

#define APPNAME "FlexLayoutDesigner"

enum ButtonIDs
{
    IDM_FILE_NEW = 101,
    IDM_FILE_OPEN,
    IDM_FILE_SAVE,
    IDM_FILE_SAVE_AS,
    IDM_FILE_GENERATE,
    IDM_FILE_BENCHMARK,
    IDM_FILE_EXIT,

    IDM_FLEX_ADD,
    IDM_FLEX_REMOVE,

    IDT_LAYOUT,
};

typedef struct AppState
{
    HINSTANCE hInstance;
    HFONT hFont;
    HWND hLayoutView;
    HWND hLayoutTree;
    HWND hPropertiesContainer;
    HTREEITEM hRootTreeItem;
    YGNodeRef rootFlex;
    int index;
    int blockUpdates;
    YGNodeRef propsFlex;
    YGNodeRef editorFlex;
    char filename[MAX_PATH];
} AppState;

#define DUMP_FLEX(label, flex) \
    TRACE("%s: %3d %3d %3d %3d", \
        label, \
        (int)roundf(YGNodeLayoutGetLeft(flex)), (int)roundf(YGNodeLayoutGetTop(flex)), \
        (int)roundf(YGNodeLayoutGetWidth(flex)), (int)roundf(YGNodeLayoutGetHeight(flex)))

static bool gTimerArmed = false;
static bool gResizePending = false;

void NodeGetContext(YGNodeConstRef node, char* buffer, size_t size)
{
    NodeContext* ctx = YGNodeGetContext((YGNodeRef)node);
    if (!ctx)
    {
        if (buffer && size)
        {
            *buffer = '\0';
        }
        return;
    }

    strcpy_s(buffer, size, ctx->context);
}

void NodeSetContext(YGNodeRef node, const char* buffer)
{
    NodeContext* ctx = YGNodeGetContext(node);
    if (!ctx)
    {
        ctx = calloc(1, sizeof(NodeContext));
        YGNodeSetContext(node, ctx);
    }

    strcpy_s(ctx->context, sizeof(ctx->context), buffer);
}

void NodeGetLabel(YGNodeConstRef node, char* buffer, size_t size)
{
    NodeContext* ctx = YGNodeGetContext((YGNodeRef)node);
    if (!ctx)
    {
        if (buffer && size)
        {
            *buffer = '\0';
        }
        return;
    }

    strcpy_s(buffer, size, ctx->label);
}

void NodeSetLabel(YGNodeRef node, const char* buffer)
{
    NodeContext* ctx = YGNodeGetContext(node);
    if (!ctx)
    {
        ctx = calloc(1, sizeof(NodeContext));
        YGNodeSetContext(node, ctx);
    }

    strcpy_s(ctx->label, sizeof(ctx->label), buffer);
    if (ctx->hTreeItem)
    {
        TVITEM item = {0};

        item.mask  = TVIF_TEXT;
        item.hItem = ctx->hTreeItem;
        item.pszText = ctx->label;

        TreeView_SetItem(ctx->hTree, &item);
    }
}

static void InitProperties()
{
    YGNodeRef node = YGNodeNew();
    for (Property* prop = gProperties; prop->name; prop++)
    {
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            prop->default_.f = prop->getter.f(node);
            break;
        case PROPERTY_TYPE_VALUE:
            prop->default_.v = prop->getter.v(node);
            break;
        case PROPERTY_TYPE_DIRECTION:
            prop->default_.d = prop->getter.d(node);
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            for (int edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
            {
                prop->default_.ev[edge] = prop->getter.ev(node, edge);
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            for (int edge = YGEdgeLeft; edge <= YGEdgeBottom; edge++)
            {
                prop->default_.ef[edge] = prop->getter.ef(node, edge);
            }
            break;
        case PROPERTY_TYPE_JUSTIFY:
            prop->default_.j = prop->getter.j(node);
            break;
        case PROPERTY_TYPE_ALIGN:
            prop->default_.a = prop->getter.a(node);
            break;
        case PROPERTY_TYPE_POSITION:
            prop->default_.p = prop->getter.p(node);
            break;
        case PROPERTY_TYPE_WRAP:
            prop->default_.w = prop->getter.w(node);
            break;
        case PROPERTY_TYPE_OVERFLOW:
            prop->default_.o = prop->getter.o(node);
            break;
        case PROPERTY_TYPE_DISPLAY:
            prop->default_.disp = prop->getter.disp(node);
            break;
        case PROPERTY_TYPE_STRING:
            prop->default_.str[0] = '\0';
            break;
        default:
            assert(!"Not implemented yet");
            break;
        }
    }
    YGNodeFree(node);
}

static void CreateProperties(AppState* appState, HWND hParent)
{
    unsigned dpi = GetDpiForWindow(hParent);

    RECT rc;
    GetClientRect(hParent, &rc);

    appState->propsFlex = YGNodeNew();
    YGNodeStyleSetWidth(appState->propsFlex, rc.right - rc.left);
    YGNodeStyleSetFlexDirection(appState->propsFlex, YGFlexDirectionColumn);
    YGNodeStyleSetMargin(appState->propsFlex, YGEdgeTop, 5);
    YGNodeStyleSetMargin(appState->propsFlex, YGEdgeLeft, 5);
    YGNodeStyleSetMargin(appState->propsFlex, YGEdgeRight, 5);
    YGNodeStyleSetMargin(appState->propsFlex, YGEdgeBottom, 5);

    for (Property* prop = gProperties; prop->name; prop++)
    {
        prop->flex = YGNodeNew();
        YGNodeStyleSetFlexDirection(prop->flex, YGFlexDirectionRow);
        YGNodeStyleSetMargin(prop->flex, YGEdgeTop, MAP_PIXELS(2));
        YGNodeStyleSetAlignContent(prop->flex, YGAlignFlexStart);
        YGNodeInsertChild(appState->propsFlex, prop->flex, YGNodeGetChildCount(appState->propsFlex));

        prop->labelFlex = YGNodeNew();
        YGNodeStyleSetWidth(prop->labelFlex, MAP_PIXELS(90));
        YGNodeInsertChild(prop->flex, prop->labelFlex, 0);

        prop->controlFlex = YGNodeNew();
        YGNodeStyleSetFlexGrow(prop->controlFlex, 1.0f);
        YGNodeStyleSetHeight(prop->controlFlex, MAP_PIXELS(24));
        YGNodeInsertChild(prop->flex, prop->controlFlex, 1);

        char label[128];
        snprintf(label, sizeof(label), "%s:", prop->name);
        prop->hLabel = CreateWindow(
            "STATIC",
            label,
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            0, 0,
            32, 32,
            hParent,
            NULL,
            appState->hInstance,
            0L);
        SetFlexHWND(prop->labelFlex, prop->hLabel);

        unsigned style = 0;
        char text[64] = {0};

        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            prop->hControl = CreateWindow(
                "EDIT",
                "",
                WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|ES_RIGHT|ES_AUTOHSCROLL,
                0, 0,
                32, 32,
                hParent,
                NULL,
                appState->hInstance,
                0L);
            break;
        case PROPERTY_TYPE_VALUE:
            prop->hControl = ValueView_Create(appState->hInstance, hParent, 0, 0, 32, 32);
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            prop->hControl = EdgeValueView_Create(appState->hInstance, hParent, 0, 0, 32, 32);
            YGNodeStyleSetHeight(prop->controlFlex, EdgeValueView_GetHeight(prop->hControl));
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            prop->hControl = EdgeFloatView_Create(appState->hInstance, hParent, 0, 0, 32, 32);
            YGNodeStyleSetHeight(prop->controlFlex, EdgeFloatView_GetHeight(prop->hControl));
            break;
        case PROPERTY_TYPE_ALIGN:
        case PROPERTY_TYPE_POSITION:
        case PROPERTY_TYPE_DIRECTION:
        case PROPERTY_TYPE_WRAP:
        case PROPERTY_TYPE_JUSTIFY:
        case PROPERTY_TYPE_OVERFLOW:
        case PROPERTY_TYPE_DISPLAY:
            prop->hControl = CreateWindow(
                "COMBOBOX",
                text,
                WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|CBS_DROPDOWNLIST|CBS_HASSTRINGS|CBS_DISABLENOSCROLL,
                0, 0,
                32, 200,
                hParent,
                NULL,
                appState->hInstance,
                0L);
            break;
        case PROPERTY_TYPE_STRING:
            prop->hControl = CreateWindow(
                "EDIT",
                "",
                WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|ES_AUTOHSCROLL,
                0, 0,
                32, 32,
                hParent,
                NULL,
                appState->hInstance,
                0L);
            break;
        default:
            assert(false);
            break;
        }
                
        if (!prop->hControl)
        {
            MessageBox(hParent, "Failed to create control", "Error", MB_ICONERROR | MB_OK);
            ExitProcess(1);
        }
        SetFlexHWND(prop->controlFlex, prop->hControl);

        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            if (isnan(prop->default_.f))
            {
                strcpy(text, "");
            }
            else
            {
                snprintf(text, sizeof(text), "%0.f", prop->default_.f);
            }
            SetWindowText(prop->hControl, text);
            break;
        case PROPERTY_TYPE_VALUE:
            ValueView_Setvalue(prop->hControl, prop->default_.v);
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                EdgeValueView_SetValue(prop->hControl, i, prop->default_.ev[i]);
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                EdgeFloatView_SetValue(prop->hControl, i, prop->default_.ef[i]);
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            ComboBox_AddString(prop->hControl, "AUTO");
            ComboBox_AddString(prop->hControl, "FLEX_START");
            ComboBox_AddString(prop->hControl, "CENTER");
            ComboBox_AddString(prop->hControl, "FLEX_END");
            ComboBox_AddString(prop->hControl, "STRETCH");
            ComboBox_AddString(prop->hControl, "BASELINE");
            ComboBox_AddString(prop->hControl, "SPACE_BETWEEN");
            ComboBox_AddString(prop->hControl, "SPACE_AROUND");
            ComboBox_SetCurSel(prop->hControl, prop->default_.a);
            break;
        case PROPERTY_TYPE_POSITION:
            ComboBox_AddString(prop->hControl, "STATIC");
            ComboBox_AddString(prop->hControl, "RELATIVE");
            ComboBox_AddString(prop->hControl, "ABSOLUTE");
            ComboBox_SetCurSel(prop->hControl, prop->default_.p);
            break;
        case PROPERTY_TYPE_DIRECTION:
            ComboBox_AddString(prop->hControl, "COLUMN");
            ComboBox_AddString(prop->hControl, "COLUMN_REVERSE");
            ComboBox_AddString(prop->hControl, "ROW");
            ComboBox_AddString(prop->hControl, "ROW_REVERSE");
            ComboBox_SetCurSel(prop->hControl, prop->default_.d);
            break;
        case PROPERTY_TYPE_WRAP:
            ComboBox_AddString(prop->hControl, "NO_WRAP");
            ComboBox_AddString(prop->hControl, "WRAP");
            ComboBox_AddString(prop->hControl, "WRAP_REVERSE");
            ComboBox_SetCurSel(prop->hControl, prop->default_.w);
            break;
        case PROPERTY_TYPE_JUSTIFY:
            ComboBox_AddString(prop->hControl, "START");
            ComboBox_AddString(prop->hControl, "CENTER");
            ComboBox_AddString(prop->hControl, "END");
            ComboBox_AddString(prop->hControl, "SPACE_BETWEEN");
            ComboBox_AddString(prop->hControl, "SPACE_AROUND");
            ComboBox_AddString(prop->hControl, "SPACE_EVENLY");
            break;
        case PROPERTY_TYPE_OVERFLOW:
            ComboBox_AddString(prop->hControl, "VISIBLE");
            ComboBox_AddString(prop->hControl, "HIDDEN");
            ComboBox_AddString(prop->hControl, "SCROLL");
            break;
        case PROPERTY_TYPE_DISPLAY:
            ComboBox_AddString(prop->hControl, "FLEX");
            ComboBox_AddString(prop->hControl, "NONE");
            break;
        case PROPERTY_TYPE_STRING:
            SetWindowText(prop->hControl, prop->default_.str);
            break;
        default:
            break;
        }
    }

    YGNodeCalculateLayout(appState->propsFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(hParent, appState->propsFlex);
}

static void DisplayProperties(AppState* appState, YGNodeConstRef item)
{
    appState->blockUpdates++;
    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (!prop->hControl)
        {
            continue;
        }

        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            if (item)
            {
                float val = prop->getter.f(item);
                if (!isnan(val))
                {
                    char text[32];
                    snprintf(text, sizeof(text), "%0.f", val);
                    SetWindowText(prop->hControl, text);
                }
                else
                {
                    SetWindowText(prop->hControl, "");
                }
            }
            else
            {
                SetWindowText(prop->hControl, "");
            }
            break;
        case PROPERTY_TYPE_VALUE:
            if (item)
            {
                YGValue val = prop->getter.v(item);
                ValueView_Setvalue(prop->hControl, val);
            }
            else
            {
                ValueView_Setvalue(prop->hControl, prop->default_.v);
            }
            break;
        case PROPERTY_TYPE_EDGE_VALUE:
            if (item)
            {
                for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
                {
                    EdgeValueView_SetValue(prop->hControl, i, prop->getter.ev(item, i));
                }
            }
            else
            {
                for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
                {
                    EdgeValueView_SetValue(prop->hControl, i, prop->default_.ev[i]);
                }
            }
            break;
        case PROPERTY_TYPE_EDGE_FLOAT:
            if (item)
            {
                for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
                {
                    EdgeFloatView_SetValue(prop->hControl, i, prop->getter.ef(item, i));
                }
            }
            else
            {
                for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
                {
                    EdgeFloatView_SetValue(prop->hControl, i, prop->default_.ef[i]);
                }
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            if (prop)
            {
                int idx = prop->getter.a(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_POSITION:
            if (prop)
            {
                int idx = prop->getter.p(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_DIRECTION:
            if (prop)
            {
                int idx = prop->getter.d(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_WRAP:
            if (prop)
            {
                int idx = prop->getter.w(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_JUSTIFY:
            if (prop)
            {
                int idx = prop->getter.j(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_OVERFLOW:
            if (prop)
            {
                int idx = prop->getter.j(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_SetCurSel(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_DISPLAY:
            if (prop)
            {
                int idx = prop->getter.disp(item);
                ComboBox_SetCurSel(prop->hControl, idx);
            }
            else
            {
                ComboBox_AddString(prop->hControl, -1);
            }
            break;
        case PROPERTY_TYPE_STRING:
            if (item)
            {
                char buffer[128];
                prop->getter.str(item, buffer, sizeof(buffer));
                SetWindowText(prop->hControl, buffer);
            }
            break;
        default:
            assert(false);
            break;
        }

    }
    appState->blockUpdates--;
}

static void ApplyFont(HWND hwnd, HFONT hfont)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, TRUE);

    for (HWND hChild = GetWindow(hwnd, GW_CHILD); hChild; hChild = GetWindow(hChild, GW_HWNDNEXT))
    {
        ApplyFont(hChild, hfont);
    }
}

static HTREEITEM InsertTreeItem(
        HWND hTree,
        HTREEITEM parent,
        const char* label,
        void* data)
{
    TVINSERTSTRUCT tis = {0};
    tis.hParent = parent;
    tis.hInsertAfter = TVI_LAST;
    tis.itemex.mask = TVIF_TEXT|TVIF_PARAM;
    tis.itemex.pszText = (LPSTR)label;
    tis.itemex.lParam = (LPARAM)data;
    HTREEITEM hItem = TreeView_InsertItem(hTree, &tis);
    TreeView_Expand(hTree, parent, TVE_EXPAND);
    TreeView_SelectItem(hTree, hItem);
    return hItem;
}

static YGNodeRef GetSelectedNode(HWND hLayoutTree, HTREEITEM* treeItem)
{
    HTREEITEM sel = TreeView_GetSelection(hLayoutTree);
    if (!sel)
    {
        return NULL;
    }

    TVITEMEX tie = {0};
    tie.mask = TVIF_PARAM;
    tie.hItem = sel;
    if (!TreeView_GetItem(hLayoutTree, &tie))
    {
        return NULL;
    }

    if (treeItem)
    {
        *treeItem = sel;
    }

    return (YGNodeRef)tie.lParam;
}

static YGNodeRef CreateNode(YGNodeRef parent, int index)
{
    NodeContext* ctx = calloc(1, sizeof(NodeContext));
    ctx->color = GenerateColor(index);
    ctx->textColor = InvertColor(ctx->color);
    if (index == 0)
    {
        strcpy_s(ctx->label, sizeof(ctx->label), "rootFlex");    
    }
    else
    {
        snprintf(ctx->label, sizeof(ctx->label), "flex%d", index);
    }

    YGNodeRef node = YGNodeNew();
    YGNodeSetContext(node, ctx);

    if (parent)
    {
        YGNodeInsertChild(parent, node, YGNodeGetChildCount(parent));
    }
    return node;
}

static void DestroyNodeContext(YGNodeRef node)
{
    int childCount = YGNodeGetChildCount(node);
    for (int i = 0; i < childCount; i++)
    {
        YGNodeRef child = YGNodeGetChild(node, i);
        DestroyNodeContext(child);
    }

    NodeContext* ctx = YGNodeGetContext(node);
    if (ctx)
    {
        free(ctx);
    }
}

static void DestroyNode(YGNodeRef node)
{
    DestroyNodeContext(node);
    YGNodeFreeRecursive(node);
}

static void OnCreate(HWND hwnd, AppState* appState)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)appState);

    RECT rc;
    GetClientRect(hwnd, &rc);

    appState->hLayoutTree = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        "",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        0, 0,
        32, 32,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    appState->rootFlex = CreateNode(NULL, appState->index++);
    YGNodeStyleSetWidth(appState->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    NodeContext* rootCtx = YGNodeGetContext(appState->rootFlex);
    appState->hRootTreeItem = InsertTreeItem(appState->hLayoutTree,
            NULL,
            rootCtx->label,
            appState->rootFlex);
    rootCtx->hTreeItem = appState->hRootTreeItem;
    rootCtx->hTree = appState->hLayoutTree;

    HWND hLayoutGroupBox = GroupBox_Create(appState->hInstance, hwnd,
            "Layout",
            0, 0,
            32, 32);

    appState->hPropertiesContainer = ScrollView_Create(appState->hInstance,
                                                 hwnd,
                                                 0, 0,
                                                 32, 32);
    
    InitProperties();
    CreateProperties(appState, appState->hPropertiesContainer);
    ScrollView_UpdateScroll(appState->hPropertiesContainer);

    HWND hPropertiesGroupBox = GroupBox_Create(appState->hInstance,
            hwnd,
            "Properties",
            0, 0,
            32, 32);

    appState->hLayoutView = LayoutView_Create(appState->hInstance,
                                              hwnd,
                                              0, 0,
                                              32, 32);
    LayoutView_SetRootFlex(appState->hLayoutView, appState->rootFlex);

    NONCLIENTMETRICS ncm = {sizeof(ncm)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    appState->hFont = CreateFontIndirect(&ncm.lfMessageFont);
    ApplyFont(hwnd, appState->hFont);

    unsigned dpi = GetDpiForWindow(hwnd);

    YGNodeRef editorFlex = YGNodeNew();
    YGNodeStyleSetFlexDirection(editorFlex, YGFlexDirectionRow);
    YGNodeStyleSetPadding(editorFlex, YGEdgeLeft, MAP_PIXELS(5));
    YGNodeStyleSetPadding(editorFlex, YGEdgeTop, MAP_PIXELS(5));
    YGNodeStyleSetPadding(editorFlex, YGEdgeRight, MAP_PIXELS(5));
    YGNodeStyleSetPadding(editorFlex, YGEdgeBottom, MAP_PIXELS(5));
    YGNodeRef leftPaneFlex = YGNodeNew();
    YGNodeStyleSetMinWidth(leftPaneFlex, MAP_PIXELS(200));
    YGNodeStyleSetMaxWidth(leftPaneFlex, MAP_PIXELS(300));
    YGNodeStyleSetFlexGrow(leftPaneFlex, 1);
    YGNodeRef layoutGroupFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(layoutGroupFlex, 1);
    YGNodeStyleSetPadding(layoutGroupFlex, YGEdgeLeft, MAP_PIXELS(5));
    YGNodeStyleSetPadding(layoutGroupFlex, YGEdgeTop, MAP_PIXELS(20));
    YGNodeStyleSetPadding(layoutGroupFlex, YGEdgeRight, MAP_PIXELS(5));
    YGNodeStyleSetPadding(layoutGroupFlex, YGEdgeBottom, MAP_PIXELS(5));
    SetFlexHWND(layoutGroupFlex, hLayoutGroupBox);
    YGNodeRef layoutTreeFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(layoutTreeFlex, 1);
    SetFlexHWND(layoutTreeFlex, appState->hLayoutTree);
    YGNodeInsertChild(layoutGroupFlex, layoutTreeFlex, YGNodeGetChildCount(layoutGroupFlex));

    YGNodeInsertChild(leftPaneFlex, layoutGroupFlex, YGNodeGetChildCount(leftPaneFlex));

    YGNodeRef propsGroupFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(propsGroupFlex, 2);
    YGNodeStyleSetPadding(propsGroupFlex, YGEdgeLeft, MAP_PIXELS(5));
    YGNodeStyleSetPadding(propsGroupFlex, YGEdgeTop, MAP_PIXELS(20));
    YGNodeStyleSetPadding(propsGroupFlex, YGEdgeRight, MAP_PIXELS(5));
    YGNodeStyleSetPadding(propsGroupFlex, YGEdgeBottom, MAP_PIXELS(5));
    SetFlexHWND(propsGroupFlex, hPropertiesGroupBox);
    YGNodeRef propsFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(propsFlex, 1);
    SetFlexHWND(propsFlex, appState->hPropertiesContainer);
    YGNodeInsertChild(propsGroupFlex, propsFlex, YGNodeGetChildCount(propsGroupFlex));

    YGNodeInsertChild(leftPaneFlex, propsGroupFlex, YGNodeGetChildCount(leftPaneFlex));

    YGNodeInsertChild(editorFlex, leftPaneFlex, YGNodeGetChildCount(editorFlex));

    YGNodeRef layoutPaneFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(layoutPaneFlex, 3);
    YGNodeStyleSetMargin(layoutPaneFlex, YGEdgeLeft, MAP_PIXELS(5));
    SetFlexHWND(layoutPaneFlex, appState->hLayoutView);
    YGNodeInsertChild(editorFlex, layoutPaneFlex, YGNodeGetChildCount(editorFlex));

    appState->editorFlex = editorFlex;
    LayoutFlex(hwnd, appState->editorFlex);

    appState->blockUpdates--;
}

static void LayoutWindow(AppState* appState, HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    YGNodeStyleSetWidth(appState->editorFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->editorFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->editorFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(hwnd, appState->editorFlex);

    GetClientRect(appState->hLayoutView, &rc);
    YGNodeStyleSetWidth(appState->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (node)
    {
        DisplayProperties(appState, node);
    }

    GetClientRect(appState->hPropertiesContainer, &rc);
    YGNodeStyleSetWidth(appState->propsFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->propsFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->propsFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    LayoutFlex(hwnd, appState->propsFlex);
}

static void OnSize(AppState* appState, HWND hwnd, WORD width, WORD height)
{
    gResizePending = true;
    if (!gTimerArmed)
    {
        gTimerArmed = true;
        SetTimer(hwnd, IDT_LAYOUT, 100, NULL);
    }
}

static void OnAdd(AppState* appState, HWND hwnd, HWND hButton)
{
    HTREEITEM treeItem;
    YGNodeRef parentNode = GetSelectedNode(appState->hLayoutTree, &treeItem);
    if (!parentNode)
    {
        MessageBox(hwnd, "No parent selected", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    YGNodeRef node = CreateNode(parentNode, appState->index++);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    NodeContext* ctx = YGNodeGetContext(node);
    HTREEITEM hTreeItem = InsertTreeItem(appState->hLayoutTree,
            treeItem,
            ctx->label,
            node);
    ctx->hTreeItem = hTreeItem;
    ctx->hTree = appState->hLayoutTree;

    InvalidateRect(appState->hLayoutView, NULL, TRUE);
}

static void OnRemove(AppState* appState, HWND hwnd)
{
    HTREEITEM treeItem;
    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, &treeItem);
    if (!node)
    {
        MessageBox(hwnd, "No selected node", "Error", MB_OK|MB_ICONERROR);
        return;
    }
    if (!TreeView_GetParent(appState->hLayoutTree, treeItem))
    {
        MessageBox(hwnd, "Cannot remove root node", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    DestroyNode(node);
    TreeView_DeleteItem(appState->hLayoutTree, treeItem);

    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    InvalidateRect(appState->hLayoutView, NULL, TRUE);
}

static void GenerateFlex(CodeStream* stream, YGNodeRef flex)
{
    assert(flex != NULL);

    NodeContext* ctx = YGNodeGetContext(flex);
    assert(ctx != NULL);

    CodeStream_Write(stream, "YGNodeRef %s = YGNodeNew();", ctx->label);

    for (Property* prop = gProperties; prop->name; prop++)
    {
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
        {
            float value = prop->getter.f(flex);
            if (fcomp(value, prop->default_.f))
            {
                if (prop->scalable)
                {
                    CodeStream_Write(stream, "%s(%s, MAP_PIXELS(%d));", prop->setterName, ctx->label, (int)value);
                }
                else
                {
                    CodeStream_Write(stream, "%s(%s, %0.f);", prop->setterName, ctx->label, value);
                }
            }
            break;
        }
        case PROPERTY_TYPE_VALUE:
        {
            YGValue value = prop->getter.v(flex);
            if (vcomp(value, prop->default_.v))
            {
                switch (value.unit)
                {
                case YGUnitUndefined:
                    CodeStream_Write(stream, "%sUndefined(%s);", prop->setterName, ctx->label);
                    break;
                case YGUnitPoint:
                    if (prop->scalable)
                    {
                        CodeStream_Write(stream, "%s(%s, MAP_PIXELS(%0.f));", prop->setterName, ctx->label, value.value);
                    }
                    else
                    {
                        CodeStream_Write(stream, "%s(%s, %d);", prop->setterName, ctx->label, (int)value.value);
                    }
                    break;
                case YGUnitPercent:
                    if (prop->scalable)
                    {
                        CodeStream_Write(stream, "%sPercent(%s, MAP_PIXELS(%0.f));", prop->setterName, ctx->label, value.value);
                    }
                    else
                    {
                        CodeStream_Write(stream, "%sPercent(%s, %d);", prop->setterName, ctx->label, (int)value.value);
                    }
                    break;
                case YGUnitAuto:
                    CodeStream_Write(stream, "%sAuto(%s);", prop->setterName, ctx->label);
                    break;
                }
            }
            break;
        }
        case PROPERTY_TYPE_ALIGN:
        {
            if (prop->getter.a(flex) != prop->default_.a)
            {
                const char* alignName = NULL;
                switch (prop->getter.a(flex))
                {
                    ENUM_STRING_CASE(alignName, YGAlignAuto);
                    ENUM_STRING_CASE(alignName, YGAlignFlexStart);
                    ENUM_STRING_CASE(alignName, YGAlignCenter);
                    ENUM_STRING_CASE(alignName, YGAlignFlexEnd);
                    ENUM_STRING_CASE(alignName, YGAlignStretch);
                    ENUM_STRING_CASE(alignName, YGAlignBaseline);
                    ENUM_STRING_CASE(alignName, YGAlignSpaceBetween);
                    ENUM_STRING_CASE(alignName, YGAlignSpaceAround);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, alignName);
            }
            break;
        }
        case PROPERTY_TYPE_POSITION:
        {
            if (prop->getter.p(flex) != prop->default_.p)
            {
                const char* positionName = NULL;
                switch (prop->getter.p(flex))
                {
                    ENUM_STRING_CASE(positionName, YGPositionTypeStatic);
                    ENUM_STRING_CASE(positionName, YGPositionTypeRelative);
                    ENUM_STRING_CASE(positionName, YGPositionTypeAbsolute);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, positionName);
            }
            break;
        }
        case PROPERTY_TYPE_DIRECTION:
        {
            if (prop->getter.d(flex) != prop->default_.d)
            {
                const char* directionName = NULL;
                switch (prop->getter.d(flex))
                {
                    ENUM_STRING_CASE(directionName, YGFlexDirectionColumn);
                    ENUM_STRING_CASE(directionName, YGFlexDirectionColumnReverse);
                    ENUM_STRING_CASE(directionName, YGFlexDirectionRow);
                    ENUM_STRING_CASE(directionName, YGFlexDirectionRowReverse);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, directionName);
            }
            break;
        }
        case PROPERTY_TYPE_WRAP:
        {
            if (prop->getter.w(flex) != prop->default_.w)
            {
                const char* wrapName = NULL;
                switch (prop->getter.w(flex))
                {
                    ENUM_STRING_CASE(wrapName, YGWrapNoWrap);
                    ENUM_STRING_CASE(wrapName, YGWrapWrap);
                    ENUM_STRING_CASE(wrapName, YGWrapWrapReverse);
                }
                CodeStream_Write(stream, "%s(%s, %s)", prop->setterName, ctx->label, wrapName);
            }
            break;
        }
        case PROPERTY_TYPE_EDGE_VALUE:
        {
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                YGValue value = prop->getter.ev(flex, i);
                if (vcomp(value, prop->default_.ev[i]))
                {
                    const char* edgeName = NULL;
                    switch (i)
                    {
                        ENUM_STRING_CASE(edgeName, YGEdgeLeft);
                        ENUM_STRING_CASE(edgeName, YGEdgeRight);
                        ENUM_STRING_CASE(edgeName, YGEdgeTop);
                        ENUM_STRING_CASE(edgeName, YGEdgeBottom);
                    }

                    switch (value.unit)
                    {
                    case YGUnitUndefined:
                        CodeStream_Write(stream, "%sUndefined(%s, %s);", prop->setterName, ctx->label, edgeName);
                        break;
                    case YGUnitPoint:
                        if (prop->scalable)
                        {
                            CodeStream_Write(stream, "%s(%s, %s, MAP_PIXELS(%0.f));", prop->setterName, ctx->label, edgeName, value.value);
                        }
                        else
                        {
                            CodeStream_Write(stream, "%s(%s, %s, %d);", prop->setterName, ctx->label, edgeName, (int)value.value);
                        }
                        break;
                    case YGUnitPercent:
                        if (prop->scalable)
                        {
                            CodeStream_Write(stream, "%sPercent(%s, %s, MAP_PIXELS(%0.f));", prop->setterName, ctx->label, edgeName, value.value);
                        }
                        else
                        {
                            CodeStream_Write(stream, "%sPercent(%s, %s, %d);", prop->setterName, ctx->label, edgeName, (int)value.value);
                        }
                        break;
                    case YGUnitAuto:
                        CodeStream_Write(stream, "%sAuto(%s, %s);", prop->setterName, ctx->label, edgeName);
                        break;
                    }
                }
            }
            break;
        }
        case PROPERTY_TYPE_EDGE_FLOAT:
        {
            for (int i = YGEdgeLeft; i <= YGEdgeBottom; i++)
            {
                const char* edgeName = NULL;
                switch (i)
                {
                    ENUM_STRING_CASE(edgeName, YGEdgeLeft);
                    ENUM_STRING_CASE(edgeName, YGEdgeRight);
                    ENUM_STRING_CASE(edgeName, YGEdgeTop);
                    ENUM_STRING_CASE(edgeName, YGEdgeBottom);
                }

                if (fcomp(prop->getter.ef(flex, i), prop->default_.ef[i]))
                {
                    if (prop->scalable)
                    {
                        CodeStream_Write(stream, "%s(%s, %s, MAP_PIXELS(%d));", prop->setterName, ctx->label, edgeName, (int)prop->getter.ef(flex, i));
                    }
                    else
                    {
                        CodeStream_Write(stream, "%s(%s, %s, %0.f);", prop->setterName, ctx->label, edgeName, prop->getter.ef(flex, i));
                    }
                }
            }
            break;
        }
        case PROPERTY_TYPE_JUSTIFY:
        {
            if (prop->getter.j(flex) != prop->default_.j)
            {
                const char* justifyName = NULL;
                switch (prop->getter.j(flex))
                {
                    ENUM_STRING_CASE(justifyName, YGJustifyFlexStart);
                    ENUM_STRING_CASE(justifyName, YGJustifyCenter);
                    ENUM_STRING_CASE(justifyName, YGJustifyFlexEnd);
                    ENUM_STRING_CASE(justifyName, YGJustifySpaceBetween);
                    ENUM_STRING_CASE(justifyName, YGJustifySpaceAround);
                    ENUM_STRING_CASE(justifyName, YGJustifySpaceEvenly);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, justifyName);
            }
            break;
        }
        case PROPERTY_TYPE_OVERFLOW:
        {
            if (prop->getter.o(flex) != prop->default_.o)
            {
                const char* overflowName = NULL;
                switch (prop->getter.o(flex))
                {
                    ENUM_STRING_CASE(overflowName, YGOverflowVisible);
                    ENUM_STRING_CASE(overflowName, YGOverflowHidden);
                    ENUM_STRING_CASE(overflowName, YGOverflowScroll);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, overflowName);
            }
            break;
        }
        case PROPERTY_TYPE_DISPLAY:
        {
            if (prop->getter.disp(flex) != prop->default_.disp)
            {
                const char* displayName = NULL;
                switch (prop->getter.disp(flex))
                {
                    ENUM_STRING_CASE(displayName, YGDisplayFlex);
                    ENUM_STRING_CASE(displayName, YGDisplayNone);
                }
                CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, displayName);
            }
            break;
        }
        case PROPERTY_TYPE_STRING:
        {
            if (strcmp(prop->name, "label"))
            {
                char value[128];
                prop->getter.str(flex, value, sizeof(value));
                if (strcmp(value, prop->default_.str))
                {
                    CodeStream_Write(stream, "%s(%s, %s);", prop->setterName, ctx->label, value);                
                }
            }
            break;
        }
        }
    }

    int childCount = YGNodeGetChildCount(flex);
    for (int i = 0; i < childCount; i++)
    {
        YGNodeRef child = YGNodeGetChild(flex, i);
        GenerateFlex(stream, child);

        NodeContext* childCtx = YGNodeGetContext(child);
        assert(childCtx != NULL);

        CodeStream_Write(
            stream, 
            "YGNodeInsertChild(%s, %s, YGNodeGetChildCount(%s));",
            ctx->label,
            childCtx->label,
            ctx->label);
        CodeStream_Write(stream, "");
    }
}

static void OnGenerate(const AppState* appState, HWND hwnd)
{
    CodeStream* stream = CodeStream_Create();
    GenerateFlex(stream, appState->rootFlex);

    HWND hCodeWin = CodeWindow_Create(
        appState->hInstance,
        hwnd,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0);
    CodeWindow_SetValue(hCodeWin, CodeStream_Get(stream));
    CodeStream_Destroy(stream);

    SendMessage(hCodeWin, WM_SETFONT, (WPARAM)appState->hFont, TRUE);
}

static void OnSave(AppState* appState, HWND hwnd)
{
    if (!strlen(appState->filename))
    {
        if (!SaveFileDialog(hwnd, appState->filename, sizeof(appState->filename)))
        {
            return;
        }
    }

    if (SaveLayout(appState->filename, appState->rootFlex, appState->index) != SERIALIZATION_OK)
    {
        MessageBox(hwnd, "Save failed", "Error", MB_OK|MB_ICONERROR);
        return;
    }
}

static void OnSaveAs(AppState* appState, HWND hwnd)
{
    char filename[MAX_PATH];
    if (!SaveFileDialog(hwnd, filename, sizeof(filename)))
    {
        return;
    }

    if (SaveLayout(filename, appState->rootFlex, appState->index) != SERIALIZATION_OK)
    {
        MessageBox(hwnd, "Save failed", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    strcpy_s(appState->filename, sizeof(appState->filename), filename);

    PathStripPath(filename);
    char windowTitle[MAX_PATH + 32];
    snprintf(windowTitle, sizeof(windowTitle), "%s - %s", APPNAME, filename);
    SetWindowText(hwnd, windowTitle);
}

static HTREEITEM CreateTreeItems(HWND hTree, HTREEITEM parentTreeItem, YGNodeRef node)
{
    assert(node != NULL);

    NodeContext* ctx = YGNodeGetContext(node);
    assert(ctx != NULL);

    HTREEITEM treeItem = InsertTreeItem(hTree,
            parentTreeItem,
            ctx->label,
            node);
    ctx->hTree = hTree;
    ctx->hTreeItem = treeItem;

    unsigned childCount = YGNodeGetChildCount(node);
    for (unsigned i = 0; i < childCount; i++)
    {
        CreateTreeItems(hTree, treeItem, YGNodeGetChild(node, i));
    }

    return treeItem;
}

static void OnOpen(AppState* appState, HWND hwnd)
{
    char filename[MAX_PATH];
    if (!OpenFileDialog(hwnd, filename, sizeof(filename)))
    {
        return;
    }

    YGNodeRef node;
    if (LoadLayout(filename, &node, &appState->index) != SERIALIZATION_OK)
    {
        MessageBox(hwnd, "Open failed", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    strcpy_s(appState->filename, sizeof(appState->filename), filename);

    PathStripPath(filename);
    char windowTitle[MAX_PATH + 32];
    snprintf(windowTitle, sizeof(windowTitle), "%s - %s", APPNAME, filename);
    SetWindowText(hwnd, windowTitle);

    DestroyNode(appState->rootFlex);
    TreeView_DeleteAllItems(appState->hLayoutTree);

    appState->rootFlex = node;
    appState->hRootTreeItem = CreateTreeItems(appState->hLayoutTree, NULL, appState->rootFlex);

    RECT rc;
    GetClientRect(appState->hLayoutView, &rc);

    YGNodeStyleSetWidth(appState->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    LayoutView_SetRootFlex(appState->hLayoutView, appState->rootFlex);
    InvalidateRect(appState->hLayoutView, NULL, TRUE);
}

static void OnNew(AppState* appState, HWND hwnd)
{
    DestroyNode(appState->rootFlex);
    TreeView_DeleteAllItems(appState->hLayoutTree);

    appState->index = 0;
    appState->filename[0] = '\0';
    appState->hRootTreeItem = NULL;
    appState->rootFlex = NULL;
    SetWindowText(hwnd, APPNAME);

    appState->rootFlex = CreateNode(NULL, appState->index++);
    appState->hRootTreeItem = CreateTreeItems(appState->hLayoutTree, NULL, appState->rootFlex);

    RECT rc;
    GetClientRect(appState->hLayoutView, &rc);

    YGNodeStyleSetWidth(appState->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    LayoutView_SetRootFlex(appState->hLayoutView, appState->rootFlex);
    InvalidateRect(appState->hLayoutView, NULL, TRUE);
}

static void OnBenchmark(AppState* appState, HWND hwnd)
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    for (int i = 0; i < 5000; i++)
    {
        LayoutFlex(hwnd, appState->propsFlex);
    }

    LARGE_INTEGER stop;
    QueryPerformanceCounter(&stop);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    uint64_t index = ((stop.QuadPart - start.QuadPart) * 1000) / freq.QuadPart;

    char msg[128];
    snprintf(msg, sizeof(msg), "Performance index: %" PRIu64, index);

    MessageBox(hwnd, msg, "Info", MB_OK|MB_ICONINFORMATION);
}

static void OnCommand(AppState* appState, HWND hwnd, HWND hButton, unsigned buttonID)
{
    switch (buttonID)
    {
    case IDM_FLEX_ADD:
        OnAdd(appState, hwnd, hButton);
        break;
    case IDM_FLEX_REMOVE:
        OnRemove(appState, hwnd);
        break;
    case IDM_FILE_NEW:
        OnNew(appState, hwnd);
        break;
    case IDM_FILE_OPEN:
        OnOpen(appState, hwnd);
        break;
    case IDM_FILE_SAVE:
        OnSave(appState, hwnd);
        break;
    case IDM_FILE_SAVE_AS:
        OnSaveAs(appState, hwnd);
        break;
    case IDM_FILE_GENERATE:
        OnGenerate(appState, hwnd);
        break;
    case IDM_FILE_BENCHMARK:
        OnBenchmark(appState, hwnd);
        break;
    case IDM_FILE_EXIT:
        DestroyWindow(hwnd);
        break;
    }
}

static void OnTreeViewSelChanged(AppState* appState,
                                 HWND hwnd,
                                 HWND hTreeView,
                                 TVITEM* old,
                                 TVITEM* sel)
{
    if (sel && (sel->mask & TVIF_PARAM) && sel->lParam)
    {
        YGNodeRef node = (YGNodeRef)sel->lParam;
        LayoutView_SetSelectedFlex(appState->hLayoutView, node);
        DisplayProperties(appState, node);
    }
    else
    {
        LayoutView_SetSelectedFlex(appState->hLayoutView, NULL);
        DisplayProperties(appState, NULL);
    }
}

static void OnValueViewChanged(AppState* appState, HWND hwnd, HWND hControl, YGValue newValue)
{
    if (appState->blockUpdates)
    {
        return;
    }

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (!node)
    {
        return;
    }

    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (prop->hControl == hControl)
        {
            assert(prop->type == PROPERTY_TYPE_VALUE);
            switch (newValue.unit)
            {
            case YGUnitUndefined:
                if (prop->setter.v.point)
                {
                    prop->setter.v.point(node, NAN);
                }
                break;
            case YGUnitPoint:
                if (prop->setter.v.point)
                {
                    prop->setter.v.point(node, newValue.value);
                }
                break;
            case YGUnitPercent:
                if (prop->setter.v.percent)
                {
                    prop->setter.v.percent(node, newValue.value);
                }
                break;
            case YGUnitAuto:
                if (prop->setter.v.auto_)
                {
                    prop->setter.v.auto_(node);
                }
                break;
            default:
                assert(!"Invalid code path");
                break;
            }

            RECT rc;
            GetClientRect(hwnd, &rc);
            YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
            InvalidateRect(appState->hLayoutView, NULL, TRUE);
            break;
        }
    }
}

void OnEdgeValueViewChanged(AppState* appState, HWND hwnd, HWND hControl, YGEdge edge, YGValue value)
{
    if (appState->blockUpdates)
    {
        return;
    }

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (!node)
    {
        return;
    }
    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (prop->hControl == hControl)
        {
            assert(prop->type == PROPERTY_TYPE_EDGE_VALUE);
            switch (value.unit)
            {
            case YGUnitUndefined:
                prop->setter.ev.point(node, edge, NAN);
                break;
            case YGUnitPoint:
                prop->setter.ev.point(node, edge, value.value);
                break;
            case YGUnitPercent:
                prop->setter.ev.percent(node, edge, value.value);
                break;
            case YGUnitAuto:
                if (prop->setter.ev.auto_)
                {
                    prop->setter.ev.auto_(node, edge);
                }
                break;
            default:
                assert(!"Invalid code path");
                break;
            }

            RECT rc;
            GetClientRect(hwnd, &rc);
            YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
            InvalidateRect(appState->hLayoutView, NULL, TRUE);
            break;
        }
    }
}

void OnEdgeFloatViewChanged(AppState* appState, HWND hwnd, HWND hControl, YGEdge edge, float value)
{
    if (appState->blockUpdates)
    {
        return;
    }

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (!node)
    {
        return;
    }
    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (prop->hControl == hControl)
        {
            assert(prop->type == PROPERTY_TYPE_EDGE_FLOAT);
            prop->setter.ef(node, edge, value);

            RECT rc;
            GetClientRect(hwnd, &rc);
            YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
            InvalidateRect(appState->hLayoutView, NULL, TRUE);
            break;
        }
    }
}

static void OnNotify(AppState* appState, HWND hwnd, NMHDR* nmhdr)
{
    switch (nmhdr->code)
    {
    case TVN_SELCHANGED:
        {
            NMTREEVIEW* nmTreeView = (NMTREEVIEW*)nmhdr;
            OnTreeViewSelChanged(appState, hwnd, nmhdr->hwndFrom, &nmTreeView->itemOld, &nmTreeView->itemNew);
        }
        break;
    case VVN_CHANGED:
        {
            NM_VALUEVIEW* nmValueView = (NM_VALUEVIEW*)nmhdr;
            OnValueViewChanged(appState, hwnd, nmValueView->hdr.hwndFrom, nmValueView->newValue);
        }
        break;
    case EVVN_CHANGED:
        {
            NMEDGEVALUEVIEW* nm = (NMEDGEVALUEVIEW*)nmhdr;
            OnEdgeValueViewChanged(appState, hwnd, nm->hdr.hwndFrom, nm->edge, nm->value);
        }
        break;
    case EFVN_CHANGED:
        {
            NMEDGEFLOATVIEW* nm = (NMEDGEFLOATVIEW*)nmhdr;
            OnEdgeFloatViewChanged(appState, hwnd, nm->hdr.hwndFrom, nm->edge, nm->value);
        }
        break;
    }
}

static void OnEditControlChange(AppState* appState, HWND hwnd, HWND hControl, int id)
{
    if (appState->blockUpdates)
    {
        return;
    }

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (!node)
    {
        return;
    }

    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (prop->hControl == hControl)
        {
            char buffer[128];
            GetWindowText(hControl, buffer, sizeof(buffer));

            switch (prop->type)
            {
            case PROPERTY_TYPE_FLOAT:
                if (strlen(buffer))
                {
                    prop->setter.f(node, strtof(buffer, NULL));
                }
                else
                {
                    prop->setter.f(node, prop->default_.f);
                }
                break;
            case PROPERTY_TYPE_STRING:
                prop->setter.str(node, buffer);
                break;
            default:
                assert(false);
                break;
            }

            RECT rc;
            GetClientRect(hwnd, &rc);
            YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
            InvalidateRect(appState->hLayoutView, NULL, TRUE);
            break;
        }
    }
}

void OnComboBoxSelChange(AppState* appState, HWND hwnd, HWND hCombo, int id)
{
    if (appState->blockUpdates)
    {
        return;
    }

    YGNodeRef item = GetSelectedNode(appState->hLayoutTree, NULL);
    if (!item)
    {
        return;
    }

    for (Property* prop = gProperties; prop->name; prop++)
    {
        if (prop->hControl == hCombo)
        {
            int sel = ComboBox_GetCurSel(hCombo);
            switch (prop->type)
            {
            case PROPERTY_TYPE_ALIGN:
                if (sel != -1)
                {
                    prop->setter.a(item, sel);
                }
                break;
            case PROPERTY_TYPE_POSITION:
                if (sel != -1)
                {
                    prop->setter.p(item, sel);
                }
                break;
            case PROPERTY_TYPE_DIRECTION:
                if (sel != -1)
                {
                    prop->setter.d(item, sel);
                }
                break;
            case PROPERTY_TYPE_WRAP:
                if (sel != -1)
                {
                    prop->setter.d(item, sel);
                }
                break;
            case PROPERTY_TYPE_JUSTIFY:
                if (sel != -1)
                {
                    prop->setter.j(item, sel);
                }
                break;
            case PROPERTY_TYPE_OVERFLOW:
                if (sel != -1)
                {
                    prop->setter.o(item, sel);
                }
                break;
            case PROPERTY_TYPE_DISPLAY:
                if (sel != -1)
                {
                    prop->setter.disp(item, sel);
                }
                break;
            default:
                assert(false);
                break;
            }
            RECT rc;
            GetClientRect(hwnd, &rc);
            YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
            InvalidateRect(appState->hLayoutView, NULL, TRUE);
            break;
        }
    }
}

static void OnDestroy(AppState* appState, HWND hwnd)
{
    KillTimer(hwnd, IDT_LAYOUT);
    PostQuitMessage(0);
}

void OnTimer(AppState* appState, HWND hwnd, int timerID)
{
    if (timerID == IDT_LAYOUT)
    {
        if (!gResizePending)
        {
            KillTimer(hwnd, IDT_LAYOUT);
            gTimerArmed = false;
            return;
        }
        gResizePending = false;
        LayoutWindow(appState, hwnd);
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    AppState* appState = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd, ((CREATESTRUCTA*)lparam)->lpCreateParams);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_SIZE:
        if (appState)
        {
            OnSize(appState, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_TIMER:
        OnTimer(appState, hwnd, wparam);
        return 0;
    case WM_COMMAND:
        if (appState)
        {
            switch (HIWORD(wparam))
            {
            case EN_CHANGE:
                OnEditControlChange(appState, hwnd, (HWND)lparam, LOWORD(wparam));
                break;
            case CBN_SELCHANGE:
                OnComboBoxSelChange(appState, hwnd, (HWND)lparam, LOWORD(wparam));
                break;
            default:
                OnCommand(appState, hwnd, (HWND)lparam, LOWORD(wparam));
                break;
            }
        }
        return 0;
    case WM_NOTIFY:
        if (appState)
        {
            NMHDR* nmhdr = (NMHDR*)lparam;
            OnNotify(appState, hwnd, nmhdr);
        }
        return 0;
    case WM_DESTROY:
        OnDestroy(appState, hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static bool InitApplication(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX ics = {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_TREEVIEW_CLASSES
    };
    if (!InitCommonControlsEx(&ics))
    {
        return false;
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = APPNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    if (!LayoutView_Init(hInstance))
    {
        return false;
    }

    if (!ScrollView_Init(hInstance))
    {
        return false;
    }

    if (!ValueView_Init(hInstance))
    {
        return false;
    }

    if (!EdgeValueView_Init(hInstance))
    {
        return false;
    }

    if (!EdgeFloatView_Init(hInstance))
    {
        return false;
    }

    if (!CodeWindow_Init(hInstance))
    {
        return false;
    }
    return true;
}

static bool InitInstance(HINSTANCE hInstance, INT nShowCmd, AppState* appState)
{
    HMENU hMenuBar = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_NEW, "&New");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open...");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, "&Save");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE_AS, "S&ave As...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_GENERATE, "&Generate Code");
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_BENCHMARK, "&Benchmark");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, "&File");

    HMENU hFlexMenu = CreatePopupMenu();
    AppendMenu(hFlexMenu, MF_STRING, IDM_FLEX_ADD, "&Add Child");
    AppendMenu(hFlexMenu, MF_STRING, IDM_FLEX_REMOVE, "&Remove");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFlexMenu, "F&lex");
            
    unsigned style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

    RECT rcWindow = {
        .top = 0,
        .left = 0,
        .right = 640,
        .bottom = 520,
    };
    AdjustWindowRectEx(&rcWindow, style, TRUE, 0);

    HWND hwnd = CreateWindow(
        APPNAME,
        APPNAME,
        style,
        CW_USEDEFAULT, 0,
        rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
        NULL,
        hMenuBar,
        hInstance,
        appState);
    if (!hwnd)
    {
        return false;
    }
    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);
    return true;
}

static void UnitTest(void)
{
    YGNodeRef parent = YGNodeNew();
    YGNodeRef child1 = YGNodeNew();
    YGNodeRef child2 = YGNodeNew();

    YGNodeStyleSetFlexDirection(parent, YGFlexDirectionColumn);

    /* Child sizes */
    YGNodeStyleSetHeight(child1, 100);
    YGNodeStyleSetHeight(child2, 50);

    YGNodeInsertChild(parent, child1, 0);
    YGNodeInsertChild(parent, child2, 1);

    /* IMPORTANT:
       Do NOT set parent height.
    */

    YGNodeCalculateLayout(parent, YGUndefined, YGUndefined, YGDirectionLTR);

    float parentHeight = YGNodeLayoutGetHeight(parent);
    TRACE("Parent height: %0.f", parentHeight);
    // parentHeight == 150
}

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   INT nShowCmd)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    UnitTest();

    AppState appState = {0};
    appState.hInstance = hInstance;
    appState.index = 0;
    appState.blockUpdates = 1;

    if (!InitApplication(hInstance))
    {
        return 1;
    }
    else if (!InitInstance(hInstance, nShowCmd, &appState))
    {
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
