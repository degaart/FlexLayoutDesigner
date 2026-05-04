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

static void OnCreate(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    CreateWindow(
        "EDIT",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER|ES_RIGHT,
        0, 0,
        32, 24,
        hwnd,
        (HMENU)ID_TEXT,
        GetModuleHandle(NULL),
        0L);
    HWND hCombo = CreateWindow(
        "COMBOBOX",
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|CBS_DROPDOWNLIST|CBS_HASSTRINGS,
        32+10, 0,
        rc.right - rc.left - 32 - 10, 32+64,
        hwnd,
        (HMENU)ID_COMBO,
        GetModuleHandle(NULL),
        0L);
    ComboBox_AddString(hCombo, "Undefined");
    ComboBox_AddString(hCombo, "Points");
    ComboBox_AddString(hCombo, "%");
    ComboBox_AddString(hCombo, "Auto");
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

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
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
