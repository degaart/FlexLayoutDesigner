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
#include "flex/flex.h"
#include "LayoutView.h"
#include "ScrollView.h"
#include "GroupBox.h"
#include "Trace.h"
#include <CommCtrl.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

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
    struct flex_item* rootFlex;
    int index;
    int blockUpdates;
} AppState;

typedef union PropertyValue
{
    float f;
    int i;
    flex_align a;
    flex_position p;
    flex_direction d;
    flex_wrap w;
} PropertyValue;

typedef union PropertyGetter
{
    float (*f)(struct flex_item*);
    int (*i)(struct flex_item*);
    flex_align (*a)(struct flex_item*);
    flex_position (*p)(struct flex_item*);
    flex_direction (*d)(struct flex_item*);
    flex_wrap (*w)(struct flex_item*);
} PropertyGetter;

typedef union PropertySetter
{
    void (*f)(struct flex_item*, float);
    void (*i)(struct flex_item*, int);
    void (*a)(struct flex_item*, flex_align);
    void (*p)(struct flex_item*, flex_position);
    void (*d)(struct flex_item*, flex_direction);
    void (*w)(struct flex_item*, flex_wrap);
} PropertySetter;

typedef enum PropertyType
{
    PROPERTY_TYPE_FLOAT,
    PROPERTY_TYPE_INT,

    PROPERTY_TYPE_ALIGN,
    PROPERTY_TYPE_POSITION,
    PROPERTY_TYPE_DIRECTION,
    PROPERTY_TYPE_WRAP,
} PropertyType;

typedef struct Property
{
    const char* name;
    PropertyType type;
    PropertyValue value;
    PropertyGetter getter;
    PropertySetter setter;
    HWND hControl;
} Property;

static Property gProperties[] =
{
    {"width", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_width, flex_item_set_width },
    {"height", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_height, flex_item_set_height },
    {"left", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_left, flex_item_set_left },
    {"right", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_right, flex_item_set_right },
    {"top", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_top, flex_item_set_top },
    {"bottom", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_bottom, flex_item_set_bottom },
    {"padding_left", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_padding_left, flex_item_set_padding_left },
    {"padding_right", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_padding_right, flex_item_set_padding_right },
    {"padding_top", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_padding_top, flex_item_set_padding_top },
    {"padding_bottom", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_padding_bottom, flex_item_set_padding_bottom },
    {"margin_left", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_margin_left, flex_item_set_margin_left },
    {"margin_right", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_margin_right, flex_item_set_margin_right },
    {"margin_top", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_margin_top, flex_item_set_margin_top },
    {"margin_bottom", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_margin_bottom, flex_item_set_margin_bottom },
    {"justify_content", PROPERTY_TYPE_ALIGN, { .a = FLEX_ALIGN_START }, { .a = flex_item_get_justify_content }, { .a = flex_item_set_justify_content } },
    {"align_content", PROPERTY_TYPE_ALIGN, { .a = FLEX_ALIGN_STRETCH }, { .a = flex_item_get_align_content }, { .a = flex_item_set_align_content } },
    {"align_items", PROPERTY_TYPE_ALIGN, { .a = FLEX_ALIGN_STRETCH }, { .a = flex_item_get_align_items }, { .a = flex_item_set_align_items } },
    {"align_self", PROPERTY_TYPE_ALIGN, { .a = FLEX_ALIGN_AUTO }, { .a = flex_item_get_align_self }, { .a = flex_item_set_align_self } },
    {"position", PROPERTY_TYPE_POSITION, { .p = FLEX_POSITION_RELATIVE }, { .p = flex_item_get_position }, { .p = flex_item_set_position } },
    {"direction", PROPERTY_TYPE_DIRECTION, { .d = FLEX_DIRECTION_COLUMN }, { .d = flex_item_get_direction }, { .d = flex_item_set_direction } },
    {"wrap", PROPERTY_TYPE_WRAP, { .w = FLEX_WRAP_NO_WRAP}, { .w = flex_item_get_wrap }, { .w = flex_item_set_wrap } },
    {"grow", PROPERTY_TYPE_FLOAT, { .f = 0.0f }, flex_item_get_grow, flex_item_set_grow },
    {"shrink", PROPERTY_TYPE_FLOAT, { .f = 1.0f }, flex_item_get_shrink, flex_item_set_shrink },
    {"order", PROPERTY_TYPE_INT, { .i = 0.0f }, { .i = flex_item_get_order }, { .i = flex_item_set_order } },
    {"basis", PROPERTY_TYPE_FLOAT, { .f = NAN }, flex_item_get_basis, flex_item_set_basis },
    {NULL},
};

static void CreateProperties(HINSTANCE hInstance, HWND hParent)
{
    RECT rc;
    GetClientRect(hParent, &rc);

    int x = 10;
    int y = 10;
    for (Property* prop = gProperties; prop->name; prop++)
    {
        char label[128];
        snprintf(label, sizeof(label), "%s:", prop->name);
        HWND hLabel = CreateWindow(
            "STATIC",
            label,
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            x, y,
            90, 32,
            hParent,
            NULL,
            hInstance,
            0L);

        const char* windowClass = NULL;
        unsigned style = 0;
        int h = 32, hx = 0;
        char text[64] = {0};

        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
        case PROPERTY_TYPE_INT:
            windowClass = "EDIT";
            style = WS_BORDER|ES_RIGHT;
            h = 24;

            switch (prop->type)
            {
            case PROPERTY_TYPE_INT:
                snprintf(text, sizeof(text), "%d", prop->value.i);
                style |= ES_NUMBER;
                break;
            case PROPERTY_TYPE_FLOAT:
                if (!isnan(prop->value.f))
                {
                    snprintf(text, sizeof(text), "%0.f", prop->value.f);
                }
                break;
            default:
                assert(false);
                break;
            }
            break;
        case PROPERTY_TYPE_ALIGN:
        case PROPERTY_TYPE_POSITION:
        case PROPERTY_TYPE_DIRECTION:
        case PROPERTY_TYPE_WRAP:
            windowClass = "COMBOBOX";
            style = CBS_DROPDOWNLIST|CBS_HASSTRINGS;
            hx = 64;
            break;
        default:
            assert(false);
            break;
        }

        HWND hwnd = CreateWindow(
            windowClass,
            text,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|style,
            100, y,
            rc.right - rc.left - 90 - x - 10, h + hx,
            hParent,
            NULL,
            hInstance,
            0L);
        if (!hwnd)
        {
            MessageBox(hParent, "Failed to create control", "Error", MB_ICONERROR | MB_OK);
            ExitProcess(1);
        }

        switch (prop->type)
        {
        case PROPERTY_TYPE_ALIGN:
            ComboBox_AddString(hwnd, "AUTO");
            ComboBox_AddString(hwnd, "STRETCH");
            ComboBox_AddString(hwnd, "CENTER");
            ComboBox_AddString(hwnd, "START");
            ComboBox_AddString(hwnd, "END");
            ComboBox_AddString(hwnd, "SPACE_BETWEEN");
            ComboBox_AddString(hwnd, "SPACE_AROUND");
            ComboBox_AddString(hwnd, "SPACE_EVENLY");
            ComboBox_SetCurSel(hwnd, prop->value.a);
            break;
        case PROPERTY_TYPE_POSITION:
            ComboBox_AddString(hwnd, "RELATIVE");
            ComboBox_AddString(hwnd, "ABSOLUTE");
            ComboBox_SetCurSel(hwnd, prop->value.p);
            break;
        case PROPERTY_TYPE_DIRECTION:
            ComboBox_AddString(hwnd, "ROW");
            ComboBox_AddString(hwnd, "ROW_REVERSE");
            ComboBox_AddString(hwnd, "COLUMN");
            ComboBox_AddString(hwnd, "COLUMN_REVERSE");
            ComboBox_SetCurSel(hwnd, prop->value.d);
            break;
        case PROPERTY_TYPE_WRAP:
            ComboBox_AddString(hwnd, "NO_WRAP");
            ComboBox_AddString(hwnd, "WRAP");
            ComboBox_AddString(hwnd, "WRAP_REVERSE");
            ComboBox_SetCurSel(hwnd, prop->value.w);
            break;
        default:
            break;
        }

        prop->hControl = hwnd;
        y += 32 + 10;
    }
}

static void DisplayProperties(AppState* appState, struct flex_item* item)
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
        case PROPERTY_TYPE_INT:
            if (item)
            {
                int val = prop->getter.i(item);
                char text[32];
                snprintf(text, sizeof(text), "%d", val);
                SetWindowText(prop->hControl, text);
            }
            else
            {
                SetWindowText(prop->hControl, "");
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

static void ApplyProperties(struct flex_item* item)
{
    for (Property* prop = gProperties; prop->name; prop++)
    {
        switch (prop->type)
        {
        case PROPERTY_TYPE_FLOAT:
            {
                char text[32];
                GetWindowText(prop->hControl, text, sizeof(text));
                float val = strtof(text, NULL);
                if (strlen(text) && !isnan(val))
                {
                    prop->setter.f(item, strtof(text, NULL));
                }
                else
                {
                    prop->setter.f(item, prop->value.f);
                }
            }
            break;
        case PROPERTY_TYPE_INT:
            {
                char text[32];
                GetWindowText(prop->hControl, text, sizeof(text));
                if (strlen(text))
                {
                    int val = atoi(text);
                    prop->setter.i(item, val);
                }
                else
                {
                    prop->setter.i(item, prop->value.i);
                }
            }
            break;
        case PROPERTY_TYPE_ALIGN:
            {
                int sel = ComboBox_GetCurSel(prop->hControl);
                if (sel != -1)
                {
                    prop->setter.a(item, sel);
                }
                else
                {
                    prop->setter.a(item, prop->value.a);
                }
            }
            break;
        case PROPERTY_TYPE_POSITION:
            {
                int sel = ComboBox_GetCurSel(prop->hControl);
                if (sel != -1)
                {
                    prop->setter.p(item, sel);
                }
                else
                {
                    prop->setter.p(item, prop->value.p);
                }
            }
            break;
        case PROPERTY_TYPE_DIRECTION:
            {
                int sel = ComboBox_GetCurSel(prop->hControl);
                if (sel != -1)
                {
                    prop->setter.d(item, sel);
                }
                else
                {
                    prop->setter.d(item, prop->value.d);
                }
            }
            break;
        case PROPERTY_TYPE_WRAP:
            {
                int sel = ComboBox_GetCurSel(prop->hControl);
                if (sel != -1)
                {
                    prop->setter.w(item, sel);
                }
                else
                {
                    prop->setter.w(item, prop->value.w);
                }
            }
            break;
        default:
            assert(false);
            break;
        }
    }
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

    appState->rootFlex = flex_item_new();
    flex_item_set_left(appState->rootFlex, 0.0f);
    flex_item_set_top(appState->rootFlex, 0.0f);
    flex_item_set_width(appState->rootFlex, rc.right - rc.left);
    flex_item_set_height(appState->rootFlex, rc.bottom - rc.top);
    flex_item_set_managed_ptr(appState->rootFlex, 0);
    flex_layout(appState->rootFlex);

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

    CreateProperties(appState->hInstance, propertiesContainer);
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

    flex_item_set_width(appState->rootFlex, layoutViewWidth);
    flex_item_set_height(appState->rootFlex, layoutViewHeight);
    flex_layout(appState->rootFlex);

    SetWindowPos(appState->hLayoutView,
                 NULL,
                 rc.left, rc.right,
                 layoutViewWidth, layoutViewHeight,
                 SWP_NOMOVE | SWP_NOZORDER);
    InvalidateRect(appState->hLayoutView, NULL, FALSE);
}

static void OnAdd(AppState* appState, HWND hwnd, HWND hButton)
{
    HTREEITEM parent = TreeView_GetSelection(appState->hLayoutTree);
    if (!parent)
    {
        MessageBox(hwnd, "No parent selected", "Error", MB_OK|MB_ICONERROR);
        return;
    }

    TVITEMEX tie = {0};
    tie.mask = TVIF_PARAM;
    tie.hItem = parent;
    if (!TreeView_GetItem(appState->hLayoutTree, &tie))
    {
        TRACE("TreeView_GetItem failed");
    }

    char label[32];
    snprintf(label, sizeof(label), "%d", ++appState->index);

    struct flex_item* flexItem = flex_item_new();
    flex_item_set_managed_ptr(flexItem, (void*)(uintptr_t)appState->index);
    flex_item_set_basis(flexItem, 32.0f);
    flex_item_add((struct flex_item*)tie.lParam, flexItem);
    flex_layout(appState->rootFlex);

    InsertTreeItem(appState->hLayoutTree,
            parent,
            label,
            flexItem);

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
        struct flex_item* flex = (struct flex_item*)sel->lParam;
        LayoutView_SetSelectedFlex(appState->hLayoutView, flex);
        DisplayProperties(appState, flex);
    }
    else
    {
        LayoutView_SetSelectedFlex(appState->hLayoutView, NULL);
        DisplayProperties(appState, NULL);
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
    }
}

static void OnEditControlChange(AppState* appState, HWND hwnd, HWND hControl, int id)
{
    if (appState->blockUpdates)
    {
        return;
    }

    HTREEITEM sel = TreeView_GetSelection(appState->hLayoutTree);
    if (!sel)
    {
        return;
    }

    TVITEMEX tie = {0};
    tie.mask = TVIF_PARAM;
    tie.hItem = sel;
    if (!TreeView_GetItem(appState->hLayoutTree, &tie))
    {
        TRACE("TreeView_GetItem failed");
        return;
    }

    struct flex_item* item = (struct flex_item*)tie.lParam;
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
                    prop->setter.f(item, atof(buffer));
                }
                else
                {
                    prop->setter.f(item, prop->value.f);
                }
                break;
            case PROPERTY_TYPE_INT:
                if (strlen(buffer))
                {
                    prop->setter.f(item, atoi(buffer));
                }
                else
                {
                    prop->setter.i(item, prop->value.i);
                }
                break;
            default:
                assert(false);
                break;
            }

            flex_layout(appState->rootFlex);
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

    HTREEITEM sel = TreeView_GetSelection(appState->hLayoutTree);
    if (!sel)
    {
        return;
    }

    TVITEMEX tie = {0};
    tie.mask = TVIF_PARAM;
    tie.hItem = sel;
    if (!TreeView_GetItem(appState->hLayoutTree, &tie))
    {
        TRACE("TreeView_GetItem failed");
        return;
    }

    struct flex_item* item = (struct flex_item*)tie.lParam;
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
            flex_layout(appState->rootFlex);
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

INT WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   INT nShowCmd)
{
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
