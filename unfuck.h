/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Compatibility layer for old NT OSes                                   *
 * Written by Raymond Gillibert in 2021                                  *
 * THIS FILE IS NOT UNDER GPL but under the WTFPL.                       *
 * DO WHAT THE FUCK YOU WANT WITH THIS CODE!                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _UNFUCK_NT_
#define _UNFUCK_NT_

#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include "nanolibc.h"

/* Invalid pointer with which we initialize
 * all dynamically imported functions */
#define IPTR ((void*)(-1))

#define QWORD unsigned long long
#ifdef WIN64
    #define DorQWORD QWORD
    #define HIWORDPTR(ll)   ((DWORD) (((QWORD) (ll) >> 32) & 0xFFFFFFFF))
    #define LOWORDPTR(ll)   ((DWORD) (ll))
    #define MAKELONGPTR(lo, hi) ((QWORD) (((DWORD) (lo)) | ((QWORD) ((DWORD) (hi))) << 32))
#else
    #define DorQWORD unsigned long
    #define HIWORDPTR(l)   ((WORD) (((DWORD) (l) >> 16) & 0xFFFF))
    #define LOWORDPTR(l)   ((WORD) (l))
    #define MAKELONGPTR(lo, hi) ((DWORD) (((WORD) (lo)) | ((DWORD) ((WORD) (hi))) << 16))
#endif

#ifndef LOBYTE
#define LOBYTE(w) ((BYTE)(w))
#endif

#ifndef HIBYTE
#define HIBYTE(w) ( (BYTE)(((WORD) (w) >> 8) & 0xFF) )
#endif

#define ARR_SZ(x) (sizeof(x) / sizeof((x)[0]))
#define IDAPPLY 0x3021

#ifndef NIIF_USER
#define NIIF_USER 0x00000004
#endif

#ifndef SUBCLASSPROC
typedef LRESULT (CALLBACK *SUBCLASSPROC)
    (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
    , UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
#endif

#ifndef LOG_STUFF
#define LOG_STUFF 0
#endif
#define LOGA(X, ...) {DWORD err=GetLastError(); FILE *LOG=fopen("ad.log", "a"); fprintf(LOG, X, ##__VA_ARGS__); fprintf(LOG,", LastError=%lu\n",err); fclose(LOG); SetLastError(0); }
#define LOG(X, ...) if(LOG_STUFF) LOGA(X, ##__VA_ARGS__)

/* Stuff missing in MinGW */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
CLSID my_CLSID_MMDeviceEnumerator= {0xBCDE0395,0xE52F,0x467C,{0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E}};
GUID  my_IID_IMMDeviceEnumerator = {0xA95664D2,0x9614,0x4F35,{0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6}};
GUID  my_IID_IAudioEndpointVolume= {0x5CDF2C82,0x841E,0x4546,{0x97,0x22,0x0C,0xF7,0x40,0x78,0x22,0x9A}};

/* USER32.DLL */
static BOOL (WINAPI *mySetLayeredWindowAttributes)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags) = IPTR;
static BOOL (WINAPI *myGetLayeredWindowAttributes)(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags) = IPTR;
static HWND (WINAPI *myGetAncestor)(HWND hwnd, UINT gaFlags) = IPTR;
static BOOL (WINAPI *myEnumDisplayMonitors)(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData) = IPTR;
static BOOL (WINAPI *myGetMonitorInfoW)(HMONITOR hMonitor, LPMONITORINFO lpmi) = IPTR;
static HMONITOR (WINAPI *myMonitorFromPoint)(POINT pt, DWORD dwFlags) = IPTR;
static HMONITOR (WINAPI *myMonitorFromWindow)(HWND hwnd, DWORD dwFlags) = IPTR;

/* DWMAPI.DLL */
static HRESULT (WINAPI *myDwmGetWindowAttribute)(HWND hwnd, DWORD a, PVOID b, DWORD c) = IPTR;
static HRESULT (WINAPI *myDwmIsCompositionEnabled)(BOOL *pfEnabled) = IPTR;

/* NTDLL.DLL */
static LONG (NTAPI *myNtSuspendProcess)(HANDLE ProcessHandle) = IPTR;
static LONG (NTAPI *myNtResumeProcess )(HANDLE ProcessHandle) = IPTR;

/* OLE32.DLL */
HRESULT (WINAPI *myCoInitialize)(LPVOID pvReserved) = IPTR;
VOID (WINAPI *myCoUninitialize)( ) = IPTR;
HRESULT (WINAPI *myCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv) = IPTR;

/* Removes the trailing file name from a path */
static BOOL PathRemoveFileSpecL(LPTSTR p)
{
    int i=0;
    if (!p) return FALSE;

    while(p[++i] != '\0');
    while(i > 0 && p[i] != '\\') i--;
    p[i]='\0';

    return TRUE;
}

/* Removes the path and keeps only the file name */
static void PathStripPathL(LPTSTR p)
{
    int i=0, j;
    if (!p) return;

    while(p[++i] != '\0');
    while(i >= 0 && p[i] != '\\') i--;
    i++;
    for(j=0; p[i+j] != '\0'; j++) p[j]=p[i+j];
    p[j]= '\0';
}

static BOOL HaveProc(char *DLLname, char *PROCname)
{
    HINSTANCE hdll = LoadLibraryA(DLLname);
    BOOL ret = FALSE;
    if (hdll) {
        if(GetProcAddress(hdll, PROCname)) {
            ret = TRUE ;
        }
        FreeLibrary(hdll);
    }
    return ret;
}

static void *LoadDLLProc(char *DLLname, char *PROCname)
{
    HINSTANCE hdll;
    void *ret = NULL;

    if((hdll = GetModuleHandleA(DLLname))) {
        return GetProcAddress(hdll, PROCname);
    }
    hdll = LoadLibraryA(DLLname);
    if (hdll) {
        ret = GetProcAddress(hdll, PROCname);
        if (!ret) FreeLibrary(hdll);
    }
    return ret;
}
static BOOL FreeDLLByName(char *DLLname)
{
    HINSTANCE hdll;
    if((hdll = GetModuleHandleA(DLLname)))
        return FreeLibrary(hdll);
    return FALSE;
}
static HWND GetAncestorL(HWND hwnd, UINT gaFlags)
{
    HWND hlast, hprevious;
    LONG wlong;
    if(!hwnd) return NULL;
    hprevious = hwnd;

    if (myGetAncestor == IPTR) {
        myGetAncestor = LoadDLLProc("USER32.DLL", "GetAncestor");
    }
    if(myGetAncestor) { /* We know we have the function */
        return myGetAncestor(hwnd, gaFlags);
    }
    /* Fallback */
    while ( (hlast = GetParent(hprevious)) != NULL ){
        wlong=GetWindowLong(hprevious, GWL_STYLE);
        if(wlong&(WS_POPUP)) break;
        hprevious=hlast;
    }
    return hprevious;
}
#define GetAncestor GetAncestorL

static BOOL GetLayeredWindowAttributesL(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
    if (myGetLayeredWindowAttributes == IPTR) {
        myGetLayeredWindowAttributes=LoadDLLProc("USER32.DLL", "GetLayeredWindowAttributes");
    }
    if (myGetLayeredWindowAttributes) { /* We know we have the function */
        return myGetLayeredWindowAttributes(hwnd, pcrKey, pbAlpha, pdwFlags);
    }
    return FALSE;
}
#define GetLayeredWindowAttributes GetLayeredWindowAttributesL

static BOOL SetLayeredWindowAttributesL(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
    if (mySetLayeredWindowAttributes == IPTR) { /* First time */
        mySetLayeredWindowAttributes=LoadDLLProc("USER32.DLL", "SetLayeredWindowAttributes");
    }
    if(mySetLayeredWindowAttributes) { /* We know we have the function */
        return mySetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
    }
    return FALSE;
}
#define SetLayeredWindowAttributes SetLayeredWindowAttributesL

static BOOL GetMonitorInfoL(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
    if (myGetMonitorInfoW == IPTR) { /* First time */
        myGetMonitorInfoW=LoadDLLProc("USER32.DLL", "GetMonitorInfoW");
    }
    if(myGetMonitorInfoW) { /* We know we have the function */
        if(hMonitor) return myGetMonitorInfoW(hMonitor, lpmi);
    }
    /* Fallback for NT4 */
    GetClientRect(GetDesktopWindow(), &lpmi->rcMonitor);
    SystemParametersInfo(SPI_GETWORKAREA, 0, &lpmi->rcWork, 0);
    lpmi->dwFlags = MONITORINFOF_PRIMARY;

    return TRUE;
}
#undef GetMonitorInfo
#define GetMonitorInfo GetMonitorInfoL

static BOOL EnumDisplayMonitorsL(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
    MONITORINFO mi;

    if (myEnumDisplayMonitors == IPTR) { /* First time */
        myEnumDisplayMonitors=LoadDLLProc("USER32.DLL", "EnumDisplayMonitors");
    }
    if (myEnumDisplayMonitors) { /* We know we have the function */
        return myEnumDisplayMonitors(hdc, lprcClip, lpfnEnum, dwData);
    }

    /* Fallbak */
    GetMonitorInfoL(NULL, &mi);
    lpfnEnum(NULL, NULL, &mi.rcMonitor, 0); /* Callback function */

    return TRUE;
}
#define EnumDisplayMonitors EnumDisplayMonitorsL

static HMONITOR MonitorFromPointL(POINT pt, DWORD dwFlags)
{
    if (myMonitorFromPoint == IPTR) { /* First time */
        myMonitorFromPoint=LoadDLLProc("USER32.DLL", "MonitorFromPoint");
    }
    if (myMonitorFromPoint) { /* We know we have the function */
        return myMonitorFromPoint(pt, dwFlags);
    }
    return NULL;
}
#define MonitorFromPoint MonitorFromPointL

static HMONITOR MonitorFromWindowL(HWND hwnd, DWORD dwFlags)
{
    if (myMonitorFromWindow == IPTR) { /* First time */
        myMonitorFromWindow=LoadDLLProc("USER32.DLL", "MonitorFromWindow");
    }
    if (myMonitorFromWindow) { /* We know we have the function */
        return myMonitorFromWindow(hwnd, dwFlags);
    }
    return NULL;
}
#define MonitorFromWindow MonitorFromWindowL

static HRESULT DwmGetWindowAttributeL(HWND hwnd, DWORD a, PVOID b, DWORD c)
{
    if (myDwmGetWindowAttribute == IPTR) { /* First time */
        myDwmGetWindowAttribute=LoadDLLProc("DWMAPI.DLL", "DwmGetWindowAttribute");
    }
    if (myDwmGetWindowAttribute) { /* We know we have the function */
        return myDwmGetWindowAttribute(hwnd, a, b, c);
    }
    /* DwmGetWindowAttribute return 0 on sucess ! */
    return 666; /* Here we FAIL with 666 error    */
}
/* #define DwmGetWindowAttribute DwmGetWindowAttributeL */

static void SubRect(RECT *__restrict__ frame, const RECT *rect)
{
    frame->left -= rect->left;
    frame->top  -= rect->top;
    frame->right = rect->right - frame->right;
    frame->bottom = rect->bottom - frame->bottom;
}
static void InflateRectBorder(RECT *__restrict__ rc, const RECT *bd)
{
    rc->left   -= bd->left;
    rc->top    -= bd->top;
    rc->right  += bd->right;
    rc->bottom += bd->bottom;
}
static void DeflateRectBorder(RECT *__restrict__ rc, const RECT *bd)
{
    rc->left   += bd->left;
    rc->top    += bd->top;
    rc->right  -= bd->right;
    rc->bottom -= bd->bottom;
}

static void FixDWMRect(HWND hwnd, RECT *bbb)
{
    RECT rect, frame;

    if(S_OK == DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT))
       && GetWindowRect(hwnd, &rect)){
        SubRect(&frame, &rect);
        CopyRect(bbb, &frame);
    } else {
        SetRectEmpty(bbb);
    }
}

/* This function is here because under Windows 10, the GetWindowRect function
 * includes invisible borders, if those borders were visible it would make
 * sense, this is the case on Windows 7 and 8.x for example
 * We use DWM api when available in order to get the REAL client area
 */
static BOOL GetWindowRectL(HWND hwnd, RECT *rect)
{
    HRESULT ret = DwmGetWindowAttributeL(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, rect, sizeof(RECT));
    if( ret == S_OK) {
        ret = TRUE;
    } else {
        ret = GetWindowRect(hwnd, rect); /* Fallback to normal */
    }
    return ret;
}
/* Under Win8 and later a window can be cloaked
 * This falg can be obtained with this function
 * 1 The window was cloaked by its owner application.
 * 2 The window was cloaked by the Shell.
 * 4 The cloak value was inherited from its owner window.
 * For windows that are supposed to be logically "visible", in addition to WS_VISIBLE.
 */
static int IsWindowCloaked(HWND hwnd)
{
    int cloaked=0;
    DwmGetWindowAttributeL(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    return cloaked;
}
static BOOL IsVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd) && !IsWindowCloaked(hwnd);
}
static LONG NtSuspendProcessL(HANDLE ProcessHandle)
{
    if (myNtSuspendProcess == IPTR) { /* First time */
        myNtSuspendProcess=LoadDLLProc("NTDLL.DLL", "NtSuspendProcess");
    }
    if (myNtSuspendProcess) { /* We know we have the function */
        return myNtSuspendProcess(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}
#define NtSuspendProcess NtSuspendProcessL

static LONG NtResumeProcessL(HANDLE ProcessHandle)
{
    if (myNtResumeProcess == IPTR) { /* First time */
        myNtResumeProcess=LoadDLLProc("NTDLL.DLL", "NtResumeProcess");
    }
    if (myNtResumeProcess) { /* We know we have the function */
        return myNtResumeProcess(ProcessHandle);
    }
    return 666; /* Here we FAIL with 666 error    */
}
#define NtResumeProcess NtResumeProcessL

static HRESULT DwmIsCompositionEnabledL(BOOL *pfEnabled)
{
    HINSTANCE hdll=NULL;
    HRESULT ret ;

    *pfEnabled = FALSE;
    ret = 666;

    hdll = LoadLibraryA("DWMAPI.DLL");
    if(hdll) {
        myDwmIsCompositionEnabled = (void *)GetProcAddress(hdll, "DwmIsCompositionEnabled");
        if(myDwmIsCompositionEnabled) {
            ret = myDwmIsCompositionEnabled(pfEnabled);
        } else {
            *pfEnabled = FALSE;
            ret = 666;
        }
        FreeLibrary(hdll);
    }
    return ret;
}

static BOOL HaveDWM()
{
    static int first=1;
    static BOOL have_dwm = FALSE;
    if(first)
        DwmIsCompositionEnabledL(&have_dwm);
    first = 0;

    return have_dwm;
}

/* PSAPI.DLL */
static DWORD (WINAPI *myGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize) = IPTR;
static DWORD GetModuleFileNameExL(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
    if (myGetModuleFileNameEx == IPTR) { /* First time */
        myGetModuleFileNameEx=LoadDLLProc("PSAPI.DLL", "GetModuleFileNameExW");
    }
    if (myGetModuleFileNameEx) { /* We have the function */
        return myGetModuleFileNameEx(hProcess, hModule, lpFilename, nSize);
    }
    return 0;
}
static DWORD (WINAPI *myGetProcessImageFileName)(HANDLE hProcess, LPWSTR lpImageFileName, DWORD nSize) = IPTR;
DWORD GetProcessImageFileNameL(HANDLE hProcess, LPWSTR lpImageFileName, DWORD    nSize)
{
    if (myGetProcessImageFileName == IPTR) {
        myGetProcessImageFileName=LoadDLLProc("PSAPI.DLL", "GetProcessImageFileNameW");
    }
    if (myGetProcessImageFileName) {
        return myGetProcessImageFileName(hProcess, lpImageFileName, nSize);
    }
    return 0;
}

static DWORD GetWindowProgName(HWND hwnd, wchar_t *title, size_t title_len)
{
    DWORD pid;
    HANDLE proc;
    DWORD ret=0;
    GetWindowThreadProcessId(hwnd, &pid);
    proc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);

    if (proc) ret = GetModuleFileNameExL(proc, NULL, title, title_len);
    else proc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if(!proc) return 0;

    if(!ret) ret = GetProcessImageFileNameL(proc, title, title_len);

    CloseHandle(proc);

    PathStripPathL(title);
    return ret? pid: 0;
}

/* Helper function to get the Min and Max tracking sizes */
static void GetMinMaxInfo(HWND hwnd, POINT *Min, POINT *Max)
{
    MINMAXINFO mmi = { {0, 0}, {0, 0}, {0, 0}
                     , {GetSystemMetrics(SM_CXMINTRACK), GetSystemMetrics(SM_CYMINTRACK)}
                     , {GetSystemMetrics(SM_CXMAXTRACK), GetSystemMetrics(SM_CXMAXTRACK)}};
    SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
    *Min = mmi.ptMinTrackSize;
    *Max = mmi.ptMaxTrackSize;
}

/* Helper function to call SetWindowPos with the SWP_ASYNCWINDOWPOS flag */
static BOOL MoveWindowAsync(HWND hwnd, int posx, int posy, int width, int height)
{
    /* flag = (!flag) * SWP_NOREDRAW; */
    return SetWindowPos(hwnd, NULL, posx, posy, width, height
               , SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_ASYNCWINDOWPOS);
}
static void MaximizeWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}
static void RestoreWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
}
static void ToggleMaxRestore(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd)? SC_RESTORE: SC_MAXIMIZE, 0);
}
static void MinimizeWindow(HWND hwnd)
{
    PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}
/* Just changes the window Z-order */
static BOOL SetWindowLevel(HWND hwnd, HWND hafter)
{
    return SetWindowPos(hwnd, hafter, 0, 0, 0, 0
    , SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
}
static int HitTestTimeoutL(HWND hwnd, LPARAM lParam)
{
    DorQWORD area=0;

    while(hwnd && SendMessageTimeout(hwnd, WM_NCHITTEST, 0, lParam, SMTO_NORMAL, 255, &area)){
        if((int)area == HTTRANSPARENT)
            hwnd = GetParent(hwnd);
        else
            break;
    }
    return (int)area;
}
#define HitTestTimeout(hwnd, x, y) HitTestTimeoutL(hwnd, MAKELPARAM(x, y))

/* This is used to detect is the window was snapped normally outside of
 * AltDrag, in this case the window appears as normal
 * ie: wndpl.showCmd=SW_SHOWNORMAL, but  its actual rect does not match with
 * its rcNormalPosition and if the WM_RESTORE command is sent, The window
 * will be restored. This is a non documented behaviour. */
static int IsWindowSnapped(HWND hwnd)
{
    RECT rect;
    int W, H, nW, nH;
    WINDOWPLACEMENT wndpl = { sizeof(WINDOWPLACEMENT) };

    if(!GetWindowRect(hwnd, &rect)) return 0;
    W = rect.right  - rect.left;
    H = rect.bottom - rect.top;

    GetWindowPlacement(hwnd, &wndpl);
    nW = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    nH = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

    return (W != nW || H != nH);
}

/* If pt and ptt are it is the same points with 4px tolerence */
static xpure int IsSamePTT(const POINT *pt, const POINT *ptt)
{
    return !( pt->x > ptt->x+4 || pt->y > ptt->y+4 ||pt->x < ptt->x-4 || pt->y < ptt->y-4 );
}
/* Limit x between l and h */
static xpure int CLAMP(int l, int x, int h)
{
    return (x < l)? l: ((x > h)? h: x);
}

/* Says if a rect is inside another one */
static pure BOOL RectInRect(const RECT *big, const RECT *wnd)
{
    return wnd->left >= big->left && wnd->top >= big->top
        && wnd->right <= big->right && wnd->bottom <= big->bottom;
}

static pure unsigned WhichSideRectInRect(const RECT *mon, const RECT *wnd)
{
    unsigned flag;
    flag  = ((wnd->left == mon->left) & (mon->right-wnd->right > 16)) << 2;
    flag |= ((wnd->right == mon->right) & (wnd->left-mon->left > 16)) << 3;
    flag |= ((wnd->top == mon->top) & (mon->bottom-wnd->bottom > 16)) << 4;
    flag |= ((wnd->bottom == mon->bottom) & (wnd->top-mon->top > 16)) << 5;

    return flag;
}

static xpure int IsEqualT(int a, int b, int th)
{
    return (b - th <= a) & (a <= b + th);
}
static int IsInRangeT(int x, int a, int b, int T)
{
    return (a-T <= x) & (x <= b+T);
}

static pure unsigned AreRectsAligned(const RECT *a, const RECT *b, const int tol)
{
    return IsEqualT(a->left, b->right, tol) << 2
         | IsEqualT(a->top, b->bottom, tol) << 4
         | IsEqualT(a->right, b->left, tol) << 3
         | IsEqualT(a->bottom, b->top, tol) << 5;
}
static int InRange(int x, int a, int b)
{
    return (x >= a) && (x <= b);
}
static xpure int SegT(long ax, long bx, const long *_ay12, const long *_by12, int tol)
{
    const long by1 = _by12[0]; /* left/top */
    const long by2 = _by12[2]; /* right/bottom */
    const long ay1 = _ay12[0]; /* left/top */
    const long ay2 = _ay12[2]; /* right/bottom */
    return IsEqualT(ax, bx, tol) /* ax == bx */
        && ( InRange(ay1, by1, by2)
          || InRange(by1, ay1, ay2)
          || InRange(ay2, by1, by2)
          || InRange(by2, ay1, ay2) );
}
/* Tells if rect b is touching rect a and on which side
 * 2^X, x=2 for Left, 3 for right, 4 for top and 5 for Bottom*/
static pure unsigned AreRectsTouchingT(const RECT *a, const RECT *b, const int tol)
{
    return SegT(a->left, b->right, &a->top, &b->top, tol) << 2 /* Left */
         | SegT(a->right, b->left, &a->top, &b->top, tol) << 3 /* Right */
         | SegT(a->top, b->bottom, &a->left, &b->left, tol) << 4 /* Top */
         | SegT(a->bottom, b->top, &a->left, &b->left, tol) << 5; /* Bottom */
}
static void CropRect(RECT *__restrict__ wnd, RECT *crop)
{
    wnd->left   = max(wnd->left,   crop->left);
    wnd->top    = max(wnd->top,    crop->top);
    wnd->right  = min(wnd->right,  crop->right);
    wnd->bottom = min(wnd->bottom, crop->bottom);
}

static void CenterRectInRect(RECT *__restrict__ wnd, const RECT *mon)
{
    int width  = wnd->right  - wnd->left;
    int height = wnd->bottom - wnd->top;
    wnd->left = mon->left + (mon->right-mon->left)/2-width/2;
    wnd->top  = mon->top  + (mon->bottom-mon->top)/2-height/2;
    wnd->right  = wnd->left + width;
    wnd->bottom = wnd->top  + height;
}

static void RectFromPts(RECT *rc, const POINT a, const POINT b)
{
    rc->left = min(a.x, b.x);
    rc->top = min(a.y, b.y);
    rc->right = max(a.x, b.x);
    rc->bottom= max(a.y, b.y);
}

#endif
