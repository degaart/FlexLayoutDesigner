#include "ScrollView.h"

#include "Trace.h"

#define CLASSNAME "ScrollView"
#define SVM_UPDATESCROLL (WM_USER+1)

struct EnumChildParams
{
    HWND hParent;
    int maxY;
};

static BOOL CALLBACK EnumChildProc(HWND hChild, LPARAM lparam)
{
    struct EnumChildParams* params = (struct EnumChildParams*)lparam;

    RECT rc;
    GetWindowRect(hChild, &rc);
    MapWindowPoints(NULL, params->hParent, (LPPOINT)&rc, 2);

    if (rc.bottom > params->maxY)
    {
        params->maxY = rc.bottom;
    }

    return TRUE;
}

static void OnUpdateScroll(HWND hwnd)
{
    struct EnumChildParams params =
    {
        .hParent = hwnd,
        .maxY = 0
    };

    if (!EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&params))
    {
        MessageBox(hwnd, "Failed to enumerate child windows", "Error", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    RECT rc;
    GetClientRect(hwnd, &rc);

    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nPage = rc.bottom - rc.top;
    si.nMin = 0;
    si.nMax = params.maxY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

static void OnVScroll(HWND hwnd, int action, int pos, int multiplier)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    GetScrollInfo(hwnd, SB_VERT, &si);

    int yPos = si.nPos;
    int oldPos = yPos;

    switch (action)
    {
    case SB_LINEUP:
        yPos -= 16 * multiplier;
        break;
    case SB_LINEDOWN:
        yPos += 16 * multiplier;
        break;
    case SB_PAGEUP:
        yPos -= si.nPage;
        break;
    case SB_PAGEDOWN:
        yPos += si.nPage;
        break;
    case SB_THUMBTRACK:
        yPos = pos;
        break;
    }

    yPos = max(si.nMin, min(yPos, si.nMax - (int)si.nPage));

    si.fMask = SIF_POS;
    si.nPos = yPos;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    int delta = yPos - oldPos;
    if (delta != 0)
    {
        ScrollWindowEx(hwnd, 0, -delta, NULL, NULL, NULL, NULL,
                       SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
    }
}

static void OnMouseWheel(HWND hwnd, int distance)
{
    int amount = distance / 120;
    OnVScroll(hwnd, SB_LINEUP, 0, amount);
}

static void OnSize(HWND hwnd, int width, int height)
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, SB_VERT, &si);

    si.fMask = SIF_POS;
    si.nPos = 0;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case SVM_UPDATESCROLL:
        OnUpdateScroll(hwnd);
        return 0;
    case WM_VSCROLL:
        OnVScroll(hwnd, LOWORD(wparam), HIWORD(wparam), 1);
        return 0;
    case WM_MOUSEWHEEL:
        OnMouseWheel(hwnd, GET_WHEEL_DELTA_WPARAM(wparam));
        return 0;
    case WM_COMMAND:
        return SendMessage(GetParent(hwnd), WM_COMMAND, wparam, lparam);
    case WM_NOTIFY:
        return SendMessage(GetParent(hwnd), WM_NOTIFY, wparam, lparam);
    case WM_SIZE:
        OnSize(hwnd, LOWORD(lparam), HIWORD(lparam));
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool ScrollView_Init(HINSTANCE hInstance)
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

HWND ScrollView_Create(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h)
{
    return CreateWindowEx(
        0,
        CLASSNAME,
        "",
        WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_VISIBLE|WS_VSCROLL,
        x, y,
        w, h,
        hParent,
        NULL,
        hInstance,
        0L);
}

void ScrollView_UpdateScroll(HWND hwnd)
{
    SendMessage(hwnd, SVM_UPDATESCROLL, 0, 0);
}

