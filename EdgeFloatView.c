#include "EdgeFloatView.h"

#include "Trace.h"
#include "Util.h"

#define CLASSNAME "EdgeFloatView"

#define EDGE_MIN YGEdgeLeft
#define EDGE_MAX (YGEdgeBottom+1)

typedef struct EdgeFloatViewState
{
    YGNodeRef flex;
    struct
    {
        HWND hLabel;
        HWND hControl;
    } controls[4];
    int height;
} EdgeFloatViewState;

static void OnCreate(HWND hwnd)
{
    HMODULE hInstance = GetModuleHandle(NULL);

    EdgeFloatViewState* state = calloc(1, sizeof(EdgeFloatViewState));
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
            45, 45,
            hwnd,
            NULL,
            hInstance,
            0L);

        state->controls[i].hControl = CreateWindow(
            "EDIT",
            "",
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|ES_RIGHT|ES_AUTOHSCROLL,
            0, 0,
            32, 32,
            hwnd,
            NULL,
            hInstance,
            0L);
    }

    unsigned dpi = GetDpiForWindow(hwnd);

    YGNodeRef rootFlex = YGNodeNew();
    YGNodeStyleSetAlignContent(rootFlex, YGAlignCenter);
    YGNodeRef flex1 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex1, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex1, YGEdgeTop, MAP_PIXELS(2));
    YGNodeRef flex2 = YGNodeNew();
    YGNodeStyleSetWidth(flex2, MAP_PIXELS(45));
    SetFlexHWND(flex2, state->controls[0].hLabel);
    YGNodeInsertChild(flex1, flex2, YGNodeGetChildCount(flex1));

    YGNodeRef flex3 = YGNodeNew();
    YGNodeStyleSetHeight(flex3, MAP_PIXELS(22));
    YGNodeStyleSetFlexGrow(flex3, 1);
    YGNodeStyleSetMargin(flex3, YGEdgeLeft, MAP_PIXELS(5));
    SetFlexHWND(flex3, state->controls[0].hControl);
    YGNodeInsertChild(flex1, flex3, YGNodeGetChildCount(flex1));

    YGNodeInsertChild(rootFlex, flex1, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex4 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex4, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex4, YGEdgeTop, MAP_PIXELS(2));
    YGNodeRef flex5 = YGNodeNew();
    YGNodeStyleSetWidth(flex5, MAP_PIXELS(45));
    SetFlexHWND(flex5, state->controls[1].hLabel);
    YGNodeInsertChild(flex4, flex5, YGNodeGetChildCount(flex4));

    YGNodeRef flex6 = YGNodeNew();
    YGNodeStyleSetHeight(flex6, MAP_PIXELS(22));
    YGNodeStyleSetFlexGrow(flex6, 1);
    YGNodeStyleSetMargin(flex6, YGEdgeLeft, MAP_PIXELS(5));
    SetFlexHWND(flex6, state->controls[1].hControl);
    YGNodeInsertChild(flex4, flex6, YGNodeGetChildCount(flex4));

    YGNodeInsertChild(rootFlex, flex4, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex7 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex7, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex7, YGEdgeTop, MAP_PIXELS(2));
    YGNodeRef flex9 = YGNodeNew();
    YGNodeStyleSetWidth(flex9, MAP_PIXELS(45));
    SetFlexHWND(flex9, state->controls[2].hLabel);
    YGNodeInsertChild(flex7, flex9, YGNodeGetChildCount(flex7));

    YGNodeRef flex10 = YGNodeNew();
    YGNodeStyleSetHeight(flex10, MAP_PIXELS(22));
    YGNodeStyleSetFlexGrow(flex10, 1);
    YGNodeStyleSetMargin(flex10, YGEdgeLeft, MAP_PIXELS(5));
    SetFlexHWND(flex10, state->controls[2].hControl);
    YGNodeInsertChild(flex7, flex10, YGNodeGetChildCount(flex7));

    YGNodeInsertChild(rootFlex, flex7, YGNodeGetChildCount(rootFlex));

    YGNodeRef flex8 = YGNodeNew();
    YGNodeStyleSetFlexDirection(flex8, YGFlexDirectionRow);
    YGNodeStyleSetMargin(flex8, YGEdgeTop, MAP_PIXELS(2));
    YGNodeRef flex11 = YGNodeNew();
    YGNodeStyleSetWidth(flex11, MAP_PIXELS(45));
    SetFlexHWND(flex11, state->controls[3].hLabel);
    YGNodeInsertChild(flex8, flex11, YGNodeGetChildCount(flex8));

    YGNodeRef flex12 = YGNodeNew();
    YGNodeStyleSetHeight(flex12, MAP_PIXELS(22));
    YGNodeStyleSetFlexGrow(flex12, 1);
    YGNodeStyleSetMargin(flex12, YGEdgeLeft, MAP_PIXELS(5));
    SetFlexHWND(flex12, state->controls[3].hControl);
    YGNodeInsertChild(flex8, flex12, YGNodeGetChildCount(flex8));

    YGNodeInsertChild(rootFlex, flex8, YGNodeGetChildCount(rootFlex));


    state->flex = rootFlex;
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);

    DumpFlex("EdgeFloatView", state->flex);
    state->height = roundf(YGNodeLayoutGetHeight(state->flex));
}

static void OnSize(EdgeFloatViewState* state, HWND hwnd, int w, int h)
{
    YGNodeStyleSetWidth(state->flex, w);
    YGNodeStyleSetHeight(state->flex, h);
    YGNodeCalculateLayout(state->flex, YGUndefined, YGUndefined, YGDirectionLTR);
    LayoutFlex(state->flex, 0.0f, 0.0f);
}

static void OnDestroy(EdgeFloatViewState* state)
{
    DestroyFlex(state->flex);
    free(state);
}

static void OnEditControlChange(EdgeFloatViewState* state, HWND hwnd, HWND hControl, WORD controlID)
{
    char buffer[128];
    GetWindowText(hControl, buffer, sizeof(buffer));

    float value;
    if (!ParseFloat(buffer, &value))
    {
        value = NAN;
    }

    for (int i = EDGE_MIN; i < EDGE_MAX; i++)
    {
        if (state->controls[i].hControl == hControl)
        {
            NMEDGEFLOATVIEW nmevv = {0};
            nmevv.hdr.code = EFVN_CHANGED;
            nmevv.hdr.hwndFrom = hwnd;
            nmevv.hdr.idFrom = GetWindowLongPtr(hwnd, GWLP_ID);
            nmevv.edge = i;
            nmevv.value = value;
            SendMessage(GetParent(hwnd), WM_NOTIFY, nmevv.hdr.idFrom, (LPARAM)&nmevv);
        }
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    EdgeFloatViewState* state;
    switch (msg)
    {
    case WM_CREATE:
        OnCreate(hwnd);
        return 0;
    case WM_SIZE:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            OnSize(state, hwnd, LOWORD(lparam), HIWORD(lparam));
        }
        return 0;
    case WM_DESTROY:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        OnDestroy(state);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    case WM_COMMAND:
        state = (EdgeFloatViewState*)GetWindowLongPtr(hwnd, GWL_USERDATA);
        if (state)
        {
            switch (HIWORD(wparam))
            {
            case EN_CHANGE:
                OnEditControlChange(state, hwnd, (HWND)lparam, LOWORD(wparam));
                break;
            }
        }
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool EdgeFloatView_Init(HINSTANCE hInstance)
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

HWND EdgeFloatView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
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

void EdgeFloatView_SetValue(HWND hControl, YGEdge edge, float value)
{
    assert(edge >= EDGE_MIN && edge <= EDGE_MAX);

    EdgeFloatViewState* state = (EdgeFloatViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);

    char text[128];
    if (isnan(value))
    {
        strcpy(text, "");
    }
    else
    {
        snprintf(text, sizeof(text), "%0.f", value);
    }
    SetWindowText(state->controls[edge].hControl, text);
}

int EdgeFloatView_GetHeight(HWND hControl)
{
    const EdgeFloatViewState* state = (const EdgeFloatViewState*)GetWindowLongPtr(hControl, GWLP_USERDATA);
    assert(state != NULL);
    return state->height;
}
