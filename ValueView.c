#include "ValueView.h"

#include "Util.h"

#include <stdlib.h>
#include <windowsx.h>
#include <assert.h>

#define CLASSNAME "ValueView"

enum ControlIDs
{
    ID_TEXT = 101,
    ID_COMBO = 102,
};

enum Messages
{
    VVM_SETVALUE = WM_USER+2,
};

enum UnitIDs
{
    UNIT_UNDEFINED = 0,
    UNIT_POINTS,
    UNIT_PERCENT,
    UNIT_AUTO
};

typedef struct EdgeValueViewState
{
    YGNodeRef rootFlex;
    HWND hEdit;
    HWND hCombo;
} EdgeValueViewState;

static void OnCreate(HWND hwnd)
{
    EdgeValueViewState* state = calloc(1, sizeof(EdgeValueViewState));

    RECT rc;
    GetClientRect(hwnd, &rc);

    state->hEdit = CreateWindow(
        "EDIT",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER|ES_RIGHT|ES_AUTOHSCROLL,
        0, 0,
        32, 32,
        hwnd,
        (HMENU)ID_TEXT,
        GetModuleHandle(NULL),
        0L);

    state->hCombo = CreateWindow(
        "COMBOBOX",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|CBS_DROPDOWNLIST|CBS_HASSTRINGS,
        0, 0,
        32, 32,
        hwnd,
        (HMENU)ID_COMBO,
        GetModuleHandle(NULL),
        0L);
    ComboBox_AddString(state->hCombo, "Undefined");
    ComboBox_AddString(state->hCombo, "Points");
    ComboBox_AddString(state->hCombo, "%");
    ComboBox_AddString(state->hCombo, "Auto");

    unsigned dpi = GetDpiForWindow(hwnd);

    YGNodeRef rootFlex = YGNodeNew();
    YGNodeStyleSetWidth(rootFlex, MAP_PIXELS(1446));
    YGNodeStyleSetHeight(rootFlex, MAP_PIXELS(928));
    YGNodeStyleSetFlexDirection(rootFlex, YGFlexDirectionRow);
    YGNodeStyleSetAlignContent(rootFlex, YGAlignAuto);
    YGNodeRef editorFlex = YGNodeNew();
    YGNodeStyleSetWidth(editorFlex, MAP_PIXELS(42));
    YGNodeStyleSetHeight(editorFlex, MAP_PIXELS(22));
    SetFlexHWND(editorFlex, state->hEdit);
    YGNodeInsertChild(rootFlex, editorFlex, YGNodeGetChildCount(rootFlex));

    YGNodeRef comboFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(comboFlex, 1);
    YGNodeStyleSetMargin(comboFlex, YGEdgeLeft, MAP_PIXELS(1));
    SetFlexHWND(comboFlex, state->hCombo);
    YGNodeInsertChild(rootFlex, comboFlex, YGNodeGetChildCount(rootFlex));

    state->rootFlex = rootFlex;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);

    YGNodeStyleSetWidth(rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(rootFlex, 0.0f, 0.0f);
}

static void OnChange(HWND hwnd)
{
    NM_VALUEVIEW nm = {0};
    nm.hdr.code = VVN_CHANGED;
    nm.hdr.hwndFrom = hwnd;
    nm.hdr.idFrom = GetDlgCtrlID(hwnd);
    
    int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, ID_COMBO));
    HWND hText = GetDlgItem(hwnd, ID_TEXT);

    char text[32];
    GetWindowText(hText, text, sizeof(text));
    bool valid = ParseFloat(text, &nm.newValue.value);
    switch (sel)
    {
    case UNIT_UNDEFINED:
        EnableWindow(hText, FALSE);
        nm.newValue.unit = YGUnitUndefined;
        valid = true;
        break;
    case UNIT_POINTS:
        EnableWindow(hText, TRUE);
        nm.newValue.unit = YGUnitPoint;
        break;
    case UNIT_PERCENT:
        EnableWindow(hText, TRUE);
        nm.newValue.unit = YGUnitPercent;
        break;
    case UNIT_AUTO:
        EnableWindow(hText, FALSE);
        nm.newValue.unit = YGUnitAuto;
        valid = true;
        break;
    default:
        assert(!"Invalid code path");
        break;
    }

    if (valid)
    {
        SendMessage(GetParent(hwnd), WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm);
    }
}

static void OnEditTextChange(HWND hwnd, HWND hControl, int id)
{
    OnChange(hwnd);
}

static void OnComboBoxSelChange(HWND hwnd, HWND hCombo, int id)
{
    OnChange(hwnd);
}

static void OnSetValue(HWND hwnd, const YGValue* value)
{
    HWND hText = GetDlgItem(hwnd, ID_TEXT);
    HWND hCombo = GetDlgItem(hwnd, ID_COMBO);

    char text[32];
    snprintf(text, sizeof(text), "%0.f", value->value);
    switch (value->unit)
    {
    case YGUnitUndefined:
        ComboBox_SetCurSel(hCombo, UNIT_UNDEFINED);
        *text = '\0';
        break;
    case YGUnitPoint:
        ComboBox_SetCurSel(hCombo, UNIT_POINTS);
        break;
    case YGUnitPercent:
        ComboBox_SetCurSel(hCombo, UNIT_PERCENT);
        break;
    case YGUnitAuto:
        ComboBox_SetCurSel(hCombo, UNIT_AUTO);
        *text = '\0';
        break;
    default:
        assert(!"Invalid code path");
        break;
    }

    SetWindowText(hText, text);
}

static void OnSize(EdgeValueViewState* state, HWND hwnd, WORD width, WORD height)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    YGNodeStyleSetWidth(state->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(state->rootFlex, rc.bottom - rc.top);
    YGNodeCalculateLayout(state->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->rootFlex, 0.0f, 0.0f);
}

static void OnDestroy(EdgeValueViewState* state, HWND hwnd)
{
    YGNodeFreeRecursive(state->rootFlex);
    free(state);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    EdgeValueViewState* state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case WM_COMMAND:
        switch (HIWORD(wparam))
        {
        case EN_CHANGE:
            OnEditTextChange(hwnd, (HWND)lparam, LOWORD(wparam));
            break;
        case CBN_SELCHANGE:
            OnComboBoxSelChange(hwnd, (HWND)lparam, LOWORD(wparam));
            break;
        }
        return 0;
    case VVM_SETVALUE:
        OnSetValue(hwnd, (const YGValue*)lparam);
        return 0;
    case WM_SIZE:
        if (state)
        {
            OnSize(state, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        OnDestroy(state, hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool ValueView_Init(HINSTANCE hInstance)
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASSNAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }
    return true;
}

HWND ValueView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
{
    HWND hwnd = CreateWindow(
        CLASSNAME,
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE,
        x, y, 
        w, h,
        hParent,
        NULL,
        hInstance,
        0L);
    return hwnd;
}

void ValueView_Setvalue(HWND hwnd, YGValue value)
{
    SendMessage(hwnd, VVM_SETVALUE, 0, (LPARAM)&value);
}
