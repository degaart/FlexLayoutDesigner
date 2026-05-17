#include "EdgeValueView.h"

#include "Util.h"
#include "ValueView.h"
#include <assert.h>
#include <stdlib.h>

#define CLASSNAME "EdgeValueView"

#define EDGE_MIN YGEdgeLeft
#define EDGE_MAX (YGEdgeBottom+1)

typedef struct EdgeValueViewState
{
    YGNodeRef flex;
    struct
    {
        HWND hLabel;
        HWND hControl;
    } controls[4];
    int height;
} EdgeValueViewState;

static void OnCreate(HWND hwnd)
{
    HMODULE hInstance = GetModuleHandle(NULL);

    EdgeValueViewState* state = calloc(1, sizeof(EdgeValueViewState));
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)state);

    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        const char* text;
        switch (i)
        {
        case YGEdgeLeft:
            text = "left:";
            break;
        case YGEdgeTop:
            text = "top:";
            break;
        case YGEdgeRight:
            text = "right:";
            break;
        case YGEdgeBottom:
            text = "bottom:";
            break;
        default:
            assert(!"Invalid code path");
            break;
        }
        state->controls[i].hLabel = CreateWindowEx(
            0,
            "STATIC",
            text,
            WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
            0, 0,
            32, 32,
            hwnd,
            NULL,
            hInstance,
            0L);
        state->controls[i].hControl = ValueView_Create(hInstance, hwnd, 0, 0, 32, 32);
    }

    unsigned dpi = GetDpiForWindow(hwnd);

    YGNodeRef rootFlex = YGNodeNew();
    YGNodeRef flex1 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex1, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex1, YGEdgeBottom, MAP_PIXELS(1));
    YGNodeRef flex2 = YGNodeNew();
    YGNodeStyleSetWidth(flex2, MAP_PIXELS(45));
    SetFlexHWND(flex2, state->controls[0].hLabel);
    YGNodeInsertChild(flex1, flex2, YGNodeGetChildCount(flex1));

    YGNodeRef flex3 = YGNodeNew();
    YGNodeStyleSetWidth(flex3, MAP_PIXELS(45));
    YGNodeStyleSetHeight(flex3, MAP_PIXELS(24));
    YGNodeStyleSetFlexGrow(flex3, 1);
    YGNodeStyleSetMargin(flex3, YGEdgeLeft, MAP_PIXELS(1));
    SetFlexHWND(flex3, state->controls[0].hControl);
    YGNodeInsertChild(flex1, flex3, YGNodeGetChildCount(flex1));

    YGNodeInsertChild(rootFlex, flex1, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex5 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex5, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex5, YGEdgeBottom, MAP_PIXELS(1));
    YGNodeRef flex8 = YGNodeNew();
    YGNodeStyleSetWidth(flex8, MAP_PIXELS(45));
    SetFlexHWND(flex8, state->controls[1].hLabel);
    YGNodeInsertChild(flex5, flex8, YGNodeGetChildCount(flex5));

    YGNodeRef flex9 = YGNodeNew();
    YGNodeStyleSetWidth(flex9, MAP_PIXELS(45));
    YGNodeStyleSetHeight(flex9, MAP_PIXELS(24));
    YGNodeStyleSetFlexGrow(flex9, 1);
    YGNodeStyleSetMargin(flex9, YGEdgeLeft, MAP_PIXELS(1));
    SetFlexHWND(flex9, state->controls[1].hControl);
    YGNodeInsertChild(flex5, flex9, YGNodeGetChildCount(flex5));

    YGNodeInsertChild(rootFlex, flex5, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex6 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex6, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex6, YGEdgeBottom, MAP_PIXELS(1));
    YGNodeRef flex11 = YGNodeNew();
    YGNodeStyleSetWidth(flex11, MAP_PIXELS(45));
    SetFlexHWND(flex11, state->controls[2].hLabel);
    YGNodeInsertChild(flex6, flex11, YGNodeGetChildCount(flex6));

    YGNodeRef flex12 = YGNodeNew();
    YGNodeStyleSetWidth(flex12, MAP_PIXELS(45));
    YGNodeStyleSetHeight(flex12, MAP_PIXELS(24));
    YGNodeStyleSetFlexGrow(flex12, 1);
    YGNodeStyleSetMargin(flex12, YGEdgeLeft, MAP_PIXELS(1));
    SetFlexHWND(flex12, state->controls[2].hControl);
    YGNodeInsertChild(flex6, flex12, YGNodeGetChildCount(flex6));

    YGNodeInsertChild(rootFlex, flex6, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex7 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex7, YGFlexDirectionRow);
    YGNodeRef flex14 = YGNodeNew();
    YGNodeStyleSetWidth(flex14, MAP_PIXELS(45));
    SetFlexHWND(flex14, state->controls[3].hLabel);
    YGNodeInsertChild(flex7, flex14, YGNodeGetChildCount(flex7));

    YGNodeRef flex15 = YGNodeNew();
    YGNodeStyleSetWidth(flex15, MAP_PIXELS(45));
    YGNodeStyleSetHeight(flex15, MAP_PIXELS(24));
    YGNodeStyleSetFlexGrow(flex15, 1);
    YGNodeStyleSetMargin(flex15, YGEdgeLeft, MAP_PIXELS(1));
    SetFlexHWND(flex15, state->controls[3].hControl);
    YGNodeInsertChild(flex7, flex15, YGNodeGetChildCount(flex7));

    YGNodeInsertChild(rootFlex, flex7, YGNodeGetChildCount(rootFlex));


    state->flex = rootFlex;
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
    state->height = roundf(YGNodeLayoutGetHeight(state->flex));

    DumpFlex("EdgeValueView", state->flex);
}

static void OnSize(EdgeValueViewState* state, HWND hwnd, int w, int h)
{
    YGNodeStyleSetWidth(state->flex, w);
    YGNodeStyleSetHeight(state->flex, h);
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
}

static void OnDestroy(EdgeValueViewState* state)
{
    DestroyFlex(state->flex);
    free(state);
}

static void OnValueViewChanged(EdgeValueViewState* state, HWND hwnd, HWND hControl, YGValue value)
{
    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        if (state->controls[i].hControl == hControl)
        {
            NMEDGEVALUEVIEW nmevv = {0};
            nmevv.hdr.code = EVVN_CHANGED;
            nmevv.hdr.hwndFrom = hwnd;
            nmevv.hdr.idFrom = GetWindowLongPtr(hwnd, GWLP_ID);
            nmevv.edge = i;
            nmevv.value = value;
            SendMessage(GetParent(hwnd), WM_NOTIFY, nmevv.hdr.idFrom, (LPARAM)&nmevv);
        }
    }
}

static void OnNotify(EdgeValueViewState* state, HWND hwnd, NMHDR* nmhdr)
{
    switch (nmhdr->code)
    {
    case VVN_CHANGED:
        {
            NM_VALUEVIEW* nmValueView = (NM_VALUEVIEW*)nmhdr;
            OnValueViewChanged(state, hwnd, nmValueView->hdr.hwndFrom, nmValueView->newValue);
        }
        break;
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    EdgeValueViewState* state;
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case WM_SIZE:
        state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            OnSize(state, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        OnDestroy(state);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    case WM_NOTIFY:
        state = (EdgeValueViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            NMHDR* nmhdr = (NMHDR*)lparam;
            OnNotify(state, hwnd, nmhdr);
        }
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool EdgeValueView_Init(HINSTANCE hInstance)
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

HWND EdgeValueView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
{
    HWND hwnd = CreateWindowEx(
        0,
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

void EdgeValueView_SetValue(HWND hControl, YGEdge edge, YGValue value)
{
    assert(edge >= EDGE_MIN && edge <= EDGE_MAX);

    EdgeValueViewState* state = (EdgeValueViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);

    ValueView_Setvalue(state->controls[edge].hControl, value);
}

int EdgeValueView_GetHeight(HWND hControl)
{
    const EdgeValueViewState* state = (const EdgeValueViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);
    return state->height;
}
