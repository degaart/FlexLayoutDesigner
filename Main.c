/*
 *
 * ┌──────────────────────┐ ┌─────────────────────────────────┐
 * │         Add          │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌──────────────────────┐ │                                 │
 * │    Generate Code     │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌─Layout───────────────┐ │                                 │
 * │ + root               │ │                                 │
 * │   + timerLabel       │ │        LAYOUT PREVIEW           │
 * │   + buttonContainer  │ │                                 │
 * │     + startButton    │ │                                 │
 * │     + stopButton     │ │                                 │
 * │     + resetButton    │ │                                 │
 * └──────────────────────┘ │                                 │
 * ┌─Properties───────────┐ │                                 │
 * │ Width:         [   ] │ │                                 │
 * │ Height:        [   ] │ │                                 │
 * │ Align:  [ CENTER  v] │ │                                 │
 * │ Wrap:   [ NO_WRAP v] │ │                                 │
 * └──────────────────────┘ └─────────────────────────────────┘
 *
*/
#include <Windows.h>
#include <stdlib.h>
#include <windowsx.h>
#include <stdbool.h>
#include "LayoutView.h"
#include "ScrollView.h"
#include "valueView.h"
#include "GroupBox.h"
#include "Trace.h"
#include <CommCtrl.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <yoga/Yoga.h>

#define APPNAME "FlexLayoutDesigner"

enum ButtonIDs
{
    ID_ADDBUTTON = 101,
    ID_GENERATEBUTTON,
};

typedef struct AppState
{
    HINSTANCE hInstance;
    HFONT hFont;
    HWND hLayoutView;
    HWND hLayoutTree;
    HTREEITEM hRootTreeItem;
    YGNodeRef rootFlex;
    int index;
    int blockUpdates;
    YGNodeRef propsFlex;
} AppState;

typedef struct EdgeValue
{
    YGEdge e;
    YGValue  v;
} EdgeValue;

typedef union PropertyValue
{
    float           f;
    YGValue         v;
    YGAlign         a;
    YGFlexDirection d;
    YGWrap          w;
    YGPositionType  p;
    EdgeValue       ev[4];    /* YGEdgeLeft, YGEdgeTop, YGEdgeRight, YGEdgeBottom, in that order */
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
} PropertyType;

typedef struct Property
{
    const char* name;
    PropertyType type;
    PropertyGetter getter;
    PropertySetter setter;
    PropertyValue default_;
    HWND hLabel;
    HWND hControl;
    YGNodeRef flex;
    YGNodeRef labelFlex;
    YGNodeRef controlFlex;
    float controlHeight;
    float layoutHeight;
} Property;

#define VALUE_PROP(p) { .v = YGNodeStyleGet ## p }, { .v = { .point = YGNodeStyleSet ## p, .percent = YGNodeStyleSet ## p ## Percent, .auto_ = YGNodeStyleSet ## p ## Auto } } 
#define FLOAT_PROP(p) { .f = YGNodeStyleGet ## p }, { .f = YGNodeStyleSet ## p }
#define DIRECTION_PROP(p) { .d = YGNodeStyleGet ## p }, { .d = YGNodeStyleSet ## p }
#define EDGE_VALUE_PROP(p) { .ev = YGNodeStyleGet ## p }, { .ev = { .point = YGNodeStyleSet ## p, .percent = YGNodeStyleSet ## p ## Percent, .auto_ = YGNodeStyleSet ## p ## Auto } }

static Property gProperties[] =
{
    {"width", PROPERTY_TYPE_VALUE, VALUE_PROP(Width) },
    {"height", PROPERTY_TYPE_VALUE, VALUE_PROP(Height) },
    {"grow", PROPERTY_TYPE_FLOAT, FLOAT_PROP(FlexGrow) },
    {"basis", PROPERTY_TYPE_VALUE, VALUE_PROP(FlexBasis) },
    {"flex-direction", PROPERTY_TYPE_DIRECTION, DIRECTION_PROP(FlexDirection) },
    //{ "margin", PROPERTY_TYPE_EDGE_VALUE, EDGE_VALUE_PROP(Margin) },
    {NULL},
};

#define DUMP_FLEX(flex) \
    TRACE("%-12s: %3d %3d %3d %3d", \
        #flex, \
        (int)roundf(YGNodeLayoutGetLeft(flex)), (int)roundf(YGNodeLayoutGetTop(flex)), \
        (int)roundf(YGNodeLayoutGetWidth(flex)), (int)roundf(YGNodeLayoutGetHeight(flex)))

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
                prop->default_.ev[edge].e = edge;
                prop->default_.ev[edge].v = prop->getter.ev(node, edge);
            }
            break;
        default:
            assert(!"Not implemented yet");
            break;
        }
    }
    YGNodeFree(node);
}

static void Layout(YGNodeRef root, float originX, float originY)
{
    int childCount = YGNodeGetChildCount(root);
    for (int i = 0; i < childCount; i++)
    {
        YGNodeRef node = YGNodeGetChild(root, i);
        HWND hwnd = YGNodeGetContext(node);

        float left = YGNodeLayoutGetLeft(node) + originX;
        float top = YGNodeLayoutGetTop(node) + originY;
        if (hwnd)
        {
            SetWindowPos(hwnd, NULL,
                roundf(left), roundf(top),
                roundf(YGNodeLayoutGetWidth(node)), roundf(YGNodeLayoutGetHeight(node)),
                SWP_NOZORDER);
        }

        Layout(node, left, top);
    }
}

static void CreateProperties(AppState* appState, HWND hParent)
{
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
        YGNodeInsertChild(appState->propsFlex, prop->flex, YGNodeGetChildCount(appState->propsFlex));

        prop->labelFlex = YGNodeNew();
        YGNodeStyleSetWidth(prop->labelFlex, 90.0f);
        YGNodeInsertChild(prop->flex, prop->labelFlex, 0);

        prop->controlFlex = YGNodeNew();
        YGNodeStyleSetFlexGrow(prop->controlFlex, 1.0f);
        YGNodeStyleSetHeight(prop->controlFlex, 24.0f);
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
        YGNodeSetContext(prop->labelFlex, prop->hLabel);

        const char* windowClass = NULL;
        unsigned style = 0;
        char text[64] = {0};

        prop->controlHeight = NAN;
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            windowClass = "EDIT";
            style = WS_BORDER|ES_RIGHT;
            if (!isnan(prop->default_.f))
            {
                snprintf(text, sizeof(text), "%0.f", prop->default_.f);
            }
            break;
        case PROPERTY_TYPE_VALUE:
            /*
             * label: [  ][   ▼]
             */
            windowClass = "ValueView";
            break;
        case PROPERTY_TYPE_ALIGN:
        case PROPERTY_TYPE_POSITION:
        case PROPERTY_TYPE_DIRECTION:
        case PROPERTY_TYPE_WRAP:
            windowClass = "COMBOBOX";
            style = CBS_DROPDOWNLIST|CBS_HASSTRINGS;
            prop->controlHeight = 128.0f;
            break;
        default:
            assert(false);
            break;
        }

        prop->hControl = CreateWindow(
            windowClass,
            text,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,
            32, 0,
            32, 32,
            hParent,
            NULL,
            appState->hInstance,
            0L);
        if (!prop->hControl)
        {
            MessageBox(hParent, "Failed to create control", "Error", MB_ICONERROR | MB_OK);
            ExitProcess(1);
        }
        YGNodeSetContext(prop->controlFlex, prop->hControl);

        switch (prop->type)
        {
        case PROPERTY_TYPE_VALUE:
            ValueView_Setvalue(prop->hControl, prop->default_.v);
            break;
        case PROPERTY_TYPE_ALIGN:
            ComboBox_AddString(prop->hControl, "AUTO");
            ComboBox_AddString(prop->hControl, "STRETCH");
            ComboBox_AddString(prop->hControl, "CENTER");
            ComboBox_AddString(prop->hControl, "START");
            ComboBox_AddString(prop->hControl, "END");
            ComboBox_AddString(prop->hControl, "SPACE_BETWEEN");
            ComboBox_AddString(prop->hControl, "SPACE_AROUND");
            ComboBox_AddString(prop->hControl, "SPACE_EVENLY");
            ComboBox_SetCurSel(prop->hControl, prop->default_.a);
            break;
        case PROPERTY_TYPE_POSITION:
            ComboBox_AddString(prop->hControl, "RELATIVE");
            ComboBox_AddString(prop->hControl, "ABSOLUTE");
            ComboBox_SetCurSel(prop->hControl, prop->default_.p);
            break;
        case PROPERTY_TYPE_DIRECTION:
            ComboBox_AddString(prop->hControl, "Column");
            ComboBox_AddString(prop->hControl, "ColumnReverse");
            ComboBox_AddString(prop->hControl, "Row");
            ComboBox_AddString(prop->hControl, "RowReverse");
            ComboBox_SetCurSel(prop->hControl, prop->default_.d);
            break;
        case PROPERTY_TYPE_WRAP:
            ComboBox_AddString(prop->hControl, "NO_WRAP");
            ComboBox_AddString(prop->hControl, "WRAP");
            ComboBox_AddString(prop->hControl, "WRAP_REVERSE");
            ComboBox_SetCurSel(prop->hControl, prop->default_.w);
            break;
        default:
            break;
        }
    }

    int y = 0;
    YGNodeCalculateLayout(appState->propsFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    Layout(appState->propsFlex, 0.0f, 0.0f);
}

static void DisplayProperties(AppState* appState, YGNodeConstRef item)
{
    appState->blockUpdates++;
    for (Property* prop = gProperties; prop->name; prop++)
    {
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

static void OnCreate(HWND hwnd, AppState* appState)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)appState);

    RECT rc;
    GetClientRect(hwnd, &rc);

    HWND addButton = CreateWindow(
        "BUTTON",
        "Add",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 0,
        300, 32,
        hwnd,
        (HMENU)ID_ADDBUTTON,
        appState->hInstance,
        0L);

    HWND generateCodeButton = CreateWindow(
        "BUTTON",
        "Generate Code",
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS,
        0, 32+10,
        300, 32,
        hwnd,
        (HMENU)ID_GENERATEBUTTON,
        appState->hInstance,
        0L);

    appState->hLayoutTree = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEW,
        "",
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        10, 32 + 10 + 32 + 20 + 20,
        280, 170,
        hwnd,
        NULL,
        appState->hInstance,
        0L);

    appState->rootFlex = YGNodeNew();
    YGNodeStyleSetWidth(appState->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(appState->rootFlex, rc.bottom - rc.top);
    YGNodeSetContext(appState->rootFlex, NULL);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    appState->hRootTreeItem = InsertTreeItem(appState->hLayoutTree,
            NULL,
            "root",
            appState->rootFlex);

    GroupBox_Create(appState->hInstance, hwnd,
            "Layout",
            0, 32+10+32+20,
            300, 200);

    HWND propertiesContainer = ScrollView_Create(appState->hInstance,
                                                 hwnd,
                                                 10, 32 + 10 + 32 + 10 + 200 + 20 + 20,
                                                 280, 170);

    InitProperties();
    CreateProperties(appState, propertiesContainer);
    ScrollView_UpdateScroll(propertiesContainer);

    GroupBox_Create(appState->hInstance,
            hwnd,
            "Properties",
            0, 32+10+32+10+200+20,
            300, 200);

    appState->hLayoutView = LayoutView_Create(appState->hInstance,
                                              hwnd,
                                              310, 10,
                                              rc.right - rc.left - 200 - 20, rc.bottom - rc.top - 20);
    LayoutView_SetRootFlex(appState->hLayoutView, appState->rootFlex);

    NONCLIENTMETRICS ncm = {sizeof(ncm)};
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    appState->hFont = CreateFontIndirect(&ncm.lfMessageFont);
    ApplyFont(hwnd, appState->hFont);

    appState->blockUpdates--;
}

static void OnSize(AppState* appState, HWND hwnd, WORD width, WORD height)
{
    RECT rc;
    GetWindowRect(appState->hLayoutView, &rc);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);

    int layoutViewWidth = rc.right - rc.left;
    if (width > rc.left)
    {
        layoutViewWidth = width - rc.left - 10;
    }

    int layoutViewHeight = rc.bottom - rc.top;
    if (height > rc.top)
    {
        layoutViewHeight = height - rc.top - 10;
    }

    YGNodeStyleSetWidth(appState->rootFlex, layoutViewWidth);
    YGNodeStyleSetHeight(appState->rootFlex, layoutViewHeight);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    YGNodeRef node = GetSelectedNode(appState->hLayoutTree, NULL);
    if (node)
    {
        DisplayProperties(appState, node);
    }

    SetWindowPos(appState->hLayoutView,
                 NULL,
                 rc.left, rc.right,
                 layoutViewWidth, layoutViewHeight,
                 SWP_NOMOVE | SWP_NOZORDER);
    InvalidateRect(appState->hLayoutView, NULL, FALSE);
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

    char label[32];
    snprintf(label, sizeof(label), "%d", ++appState->index);

    YGNodeRef node = YGNodeNew();
    YGNodeSetContext(node, (void*)appState->index);
    YGNodeInsertChild(parentNode, node, YGNodeGetChildCount(parentNode));

    RECT rc;
    GetClientRect(hwnd, &rc);
    YGNodeCalculateLayout(appState->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    InsertTreeItem(appState->hLayoutTree,
            treeItem,
            label,
            node);
    InvalidateRect(appState->hLayoutView, NULL, FALSE);
}

static void OnCommand(AppState* appState, HWND hwnd, HWND hButton, unsigned buttonID)
{
    switch (buttonID)
    {
    case ID_ADDBUTTON:
        OnAdd(appState, hwnd, hButton);
        break;
    case ID_GENERATEBUTTON:
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
                prop->setter.v.point(node, NAN);
                break;
            case YGUnitPoint:
                prop->setter.v.point(node, newValue.value);
                break;
            case YGUnitPercent:
                prop->setter.v.percent(node, newValue.value);
                break;
            case YGUnitAuto:
                prop->setter.v.auto_(node);
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
            char buffer[32];
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
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        PostQuitMessage(0);
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
    return true;
}

static bool InitInstance(HINSTANCE hInstance, INT nShowCmd, AppState* appState)
{
    unsigned style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

    RECT rcWindow = {
        .top = 0,
        .left = 0,
        .right = 640,
        .bottom = 520,
    };
    AdjustWindowRectEx(&rcWindow, style, FALSE, 0);

    HWND hwnd = CreateWindow(
        APPNAME,
        APPNAME,
        style,
        CW_USEDEFAULT, 0,
        rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
        NULL,
        NULL,
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
    AppState state;

    state.propsFlex = YGNodeNew();
    YGNodeStyleSetWidth(state.propsFlex, 300.0f);
    YGNodeStyleSetFlexDirection(state.propsFlex, YGFlexDirectionColumn);
    YGNodeStyleSetMargin(state.propsFlex, YGEdgeTop, 5);
    YGNodeStyleSetMargin(state.propsFlex, YGEdgeLeft, 5);
    YGNodeStyleSetMargin(state.propsFlex, YGEdgeRight, 5);
    YGNodeStyleSetMargin(state.propsFlex, YGEdgeBottom, 5);

    YGNodeRef prop[2];
    YGNodeRef label[2];
    YGNodeRef control[2];

    prop[0] = YGNodeNew();
    YGNodeStyleSetFlexDirection(prop[0], YGFlexDirectionRow);
    YGNodeInsertChild(state.propsFlex, prop[0], YGNodeGetChildCount(state.propsFlex));

    label[0] = YGNodeNew();
    YGNodeStyleSetWidth(label[0], 90.0f);
    YGNodeInsertChild(prop[0], label[0], YGNodeGetChildCount(prop[0]));

    control[0] = YGNodeNew();
    YGNodeStyleSetFlexGrow(control[0], 1.0f);
    YGNodeStyleSetHeight(control[0], 24.0f);
    YGNodeInsertChild(prop[0], control[0], YGNodeGetChildCount(prop[0]));

    prop[1] = YGNodeNew();
    YGNodeStyleSetFlexDirection(prop[1], YGFlexDirectionRow);
    YGNodeInsertChild(state.propsFlex, prop[1], YGNodeGetChildCount(state.propsFlex));

    YGNodeRef label2 = YGNodeNew();
    YGNodeStyleSetWidth(label2, 90.0f);
    YGNodeInsertChild(prop[1], label2, YGNodeGetChildCount(prop[1]));

    YGNodeRef control2 = YGNodeNew();
    YGNodeStyleSetFlexGrow(control2, 1.0f);
    YGNodeStyleSetHeight(control2, 24.0f);
    YGNodeInsertChild(prop[1], control2, YGNodeGetChildCount(prop[1]));

    YGNodeCalculateLayout(state.propsFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    DUMP_FLEX(prop[0]);
    DUMP_FLEX(label[0]);
    DUMP_FLEX(control[0]);
    DUMP_FLEX(prop[1]);
    DUMP_FLEX(label2);
    DUMP_FLEX(control2);
}

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   INT nShowCmd)
{
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
