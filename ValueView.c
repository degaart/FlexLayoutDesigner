#include "ValueView.h"

#include "Util.h"
#include <windowsx.h>

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

typedef struct ControlState
{
    YGNodeRef rootFlex;
    YGNodeRef labelFlex;
    YGNodeRef comboFlex;
    HWND hLabel;
    HWND hCombo;
} ControlState;

static void Layout(ControlState* state, int width, int height)
{
    YGNodeStyleSetWidth(state->rootFlex, width);
    YGNodeStyleSetHeight(state->rootFlex, height);
    YGNodeCalculateLayout(state->rootFlex, YGUndefined, YGUndefined, YGDirectionLTR);

    int x = roundf(YGNodeLayoutGetLeft(state->labelFlex));
    int y = roundf(YGNodeLayoutGetTop(state->labelFlex));
    int w = roundf(YGNodeLayoutGetWidth(state->labelFlex));
    int h = roundf(YGNodeLayoutGetHeight(state->labelFlex));
    SetWindowPos(state->hLabel,
        NULL,
        x, y,
        w, h,
        SWP_NOZORDER);

    x = roundf(YGNodeLayoutGetLeft(state->comboFlex));
    y = roundf(YGNodeLayoutGetTop(state->comboFlex));
    w = roundf(YGNodeLayoutGetWidth(state->comboFlex));
    h = 32 + 64;
    SetWindowPos(state->hCombo,
        NULL,
        x, y,
        w, h,
        SWP_NOZORDER);
}

static void OnCreate(HWND hwnd)
{
    ControlState* state = calloc(1, sizeof(ControlState));

    RECT rc;
    GetClientRect(hwnd, &rc);

    state->hLabel = CreateWindow(
        "EDIT",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER|ES_RIGHT,
        0, 0,
        32, 24,
        hwnd,
        (HMENU)ID_TEXT,
        GetModuleHandle(NULL),
        0L);

    int width = rc.right - rc.left - 32 - 10;
    int height = 32 + 64;
    state->hCombo = CreateWindow(
        "COMBOBOX",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|CBS_DROPDOWNLIST|CBS_HASSTRINGS,
        32+10, 0,
        width, height,
        hwnd,
        (HMENU)ID_COMBO,
        GetModuleHandle(NULL),
        0L);
    ComboBox_AddString(state->hCombo, "Undefined");
    ComboBox_AddString(state->hCombo, "Points");
    ComboBox_AddString(state->hCombo, "%");
    ComboBox_AddString(state->hCombo, "Auto");

    state->rootFlex = YGNodeNew();
    YGNodeStyleSetWidth(state->rootFlex, rc.right - rc.left);
    YGNodeStyleSetHeight(state->rootFlex, rc.bottom - rc.top);
    YGNodeStyleSetFlexDirection(state->rootFlex, YGFlexDirectionRow);

    state->labelFlex = YGNodeNew();
    YGNodeStyleSetWidth(state->labelFlex, 32.0f);
    YGNodeStyleSetHeight(state->labelFlex, 22.0f);
    YGNodeInsertChild(state->rootFlex, state->labelFlex, 0);

    state->comboFlex = YGNodeNew();
    YGNodeStyleSetFlexGrow(state->comboFlex, 1.0f);
    YGNodeStyleSetHeight(state->comboFlex, 32.0f + 64.0f);
    YGNodeInsertChild(state->rootFlex, state->comboFlex, 1);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)state);

    Layout(state, rc.right - rc.left, rc.bottom - rc.top);
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

static void OnSize(ControlState* state, HWND hwnd, WORD width, WORD height)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    Layout(state, rc.right - rc.left, rc.bottom - rc.top);
}

static void OnDestroy(ControlState* state, HWND hwnd)
{
    YGNodeFreeRecursive(state->rootFlex);
    free(state);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ControlState* state = (ControlState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

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
