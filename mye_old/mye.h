#ifndef MYE_H
#define MYE_H
#define MYEAPI  __declspec(dllexport)

#include <vector>
#include <windows.h>
#include <pngimage.hpp>
#include "nIcon.h"

#pragma pack(1)
enum
{
    LO_GETWINDOWHANDLE, LO_FILLRECT, LO_TEXTOUT, LO_DOCKWINDOW, LO_REGISTERBACKGROUNDWINDOW, LO_BGREPAINT,
    LO_GETBACKGROUNDBITMAP, LO_UPDATEBGBITMAP, LO_REGISTERWINDOW, LO_DEREGISTERWINDOW, LO_CLEARWINDOWS,
    LO_REGISTERKEY, LO_GETBKGNDRECT
};
enum
{
    LO_EVENT, LO_GETICON, LO_NOTFOUND, LO_ADDCLBK, LO_MODCLBK, LO_DELCLBK, LO_TESTCMD, LO_GETWINDOW, LO_ONPAINT,
    LO_WNDCHANGE, LO_TRAYCHANGE, LO_ONBCLICK, LO_ONKEY
};
enum
{
    DOCK_LEFT, DOCK_RIGHT
};

enum
{
    MYE_ADD_SHELL_CALLBACK, MYE_DEL_SHELL_CALLBACK
};
enum
{
    MYE_SUCCESS, MYE_FAILED, MYE_NO_MORE_ITEMS
};

typedef struct dock_window
{
    HWND hwnd;
    int xwidth;
    int where;
}dock_window;

typedef struct _SHELLTRAYDATA
{
    DWORD dwUnknown;
    DWORD dwMessage;
    PNOTIFYICONDATAW nid;
}*PSHELLTRAYDATA, SHELLTRAYDATA;

typedef struct _wndprocstruct
{
    HWND hwnd;
    WPARAM wParam; // Specifies the identifier of the mouse message
    LPARAM lParam; // Pointer to a MOUSEHOOKSTRUCT structure.
}WNDPROCSTRUCT;

typedef struct _events
{
    LPVOID addr;
    int type;
    unsigned long id;
}EVENTS, *PEVENTS;

typedef struct _evnt
{
    LPVOID addr;
    DWORD msg;
    char dllpipe[128];
    DWORD dwIndex;
    NOTIFYICONDATAW pn;
    DWORD id;
    LPVOID param;
    TMouseButton button;
}EVENT;

#pragma pop

// typedef allocator<nIcon*, managed_shared_memory::segment_manager>  ShmemAllocator;

typedef std::vector<LPVOID>callbacks_v;
typedef std::vector<EVENTS>events_v;
typedef std::vector<nIcon*>icons_v;

/* �������� ���������� ������� */
typedef int(WINAPI * shell_callback_api)(LPVOID p, unsigned long size);
/* �������� ������� ������� */
typedef int(WINAPI * shell_events_api)(DWORD dwIndex, PNOTIFYICONDATAW pn);
typedef int(WINAPI * shell_events2_api)(void);

typedef void(WINAPI * shell_events_onbclick_api)(LPVOID p, TMouseButton b);

typedef void(WINAPI * shell_events_onkey_api)(char key);

/* ��������� ������� ���������� */
typedef long(__stdcall*mye_TrayCallback_api)(int op, LPVOID addr, long id);
typedef long(__stdcall*mye_TrayAddCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayModifyCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayDeleteCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayBalloonCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_EnumerateIcons_api)(DWORD dwIndex, PNOTIFYICONDATAW pn);
typedef long(__stdcall*mye_TrayOnChange)(LPVOID addr);
typedef HWND(__stdcall*mye_EnumerateVisibleWindows_api)(DWORD dwIndex);
typedef long(__stdcall*mye_WndOnChange)(LPVOID addr);
typedef long(__stdcall*mye_ClickIcon_api)(BYTE dwIndex);
typedef long(__stdcall*mye_ClickWindow_api)(BYTE dwIndex);
typedef long(__stdcall*mye_DblClickIcon_api)(BYTE dwIndex);
typedef long(__stdcall*mye_RightClickIcon_api)(BYTE dwIndex);

typedef long(__stdcall*mye_TrayOnLeftDblCLick_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnRightDblClick_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnRightButtonDown_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnRightButtonUp_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnMouseMove_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnLeftButtonDown_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnLeftButtonUp_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnBalloonShown_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnBalloonTimeoutClose_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnBalloonClick_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnPopupOpen_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayOnPopupClose_api)(LPVOID addr);

//typedef HWND(__stdcall*mye_GetWindowDC_api)(int where, int width, int height);
typedef long(__stdcall*mye_UpdateBackground_api)(HBITMAP hbmp);
typedef HWND(__stdcall*mye_DockWindow_api)(HWND hwnd, int width, int height, LPVOID addr);

typedef HANDLE(__stdcall*mye_RegisterWindowRect_api)(RECT *rect, HBITMAP pic, HBITMAP onhover, TColor tclr, LPVOID callAddr,
    LPVOID param);
typedef void(__stdcall*mye_UnregisterWindowRect_api)(HANDLE id);
typedef void(__stdcall*mye_ClearWindows_api)(void);

typedef void(__stdcall*mye_RegisterKey_api)(char vkey, LPVOID addr);

typedef void(__stdcall*mye_GetOsRect_api)(RECT *prect);

typedef HBITMAP(__stdcall*mye_GetBackgroundRect_api)(TRect rect);

typedef LRESULT(CALLBACK * CallWndProc)(int nCode, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK * MouseWndProc)(int nCode, WPARAM wParam, LPARAM lParam);

typedef int(__stdcall*mye_SetVal_api)(char*name, LPVOID buf, DWORD size, char*desc = NULL);
typedef long(__stdcall*mye_GetValSize_api)(char*name);
typedef long(__stdcall*mye_GetVal_api)(char*name, LPVOID buf, DWORD size);
typedef int(__stdcall*mye_EnumerateVal_api)(char*prefix, int idx, char*kname, LPBYTE data);

extern "C"
{

    /* ����� ������� */
    long MYEAPI __stdcall mye_TrayCallback(int op, LPVOID addr, long id);

    /* ��������� �������� */
    long MYEAPI __stdcall mye_TrayAddCallback(LPVOID addr);
    long MYEAPI __stdcall mye_TrayModifyCallback(LPVOID addr);
    long MYEAPI __stdcall mye_TrayDeleteCallback(LPVOID addr);
    long MYEAPI __stdcall mye_TrayBalloonCallback(LPVOID addr);

    /* ������������ ������ */
    long MYEAPI __stdcall mye_EnumerateIcons(DWORD dwIndex, PNOTIFYICONDATAW pn);

    HWND MYEAPI __stdcall mye_EnumerateVisibleWindows(DWORD dwIndex);

    /* ����� ������ */
    long MYEAPI __stdcall mye_ClickIcon(BYTE dwIndex);
    long MYEAPI __stdcall mye_ClickWindow(DWORD dwIndex);
    long MYEAPI __stdcall mye_DblClickIcon(BYTE dwIndex);
    long MYEAPI __stdcall mye_RightClickIcon(BYTE dwIndex);

    /* ������� */
    long MYEAPI __stdcall mye_TrayOnLeftDblClick(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnRightDblClick(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnRightButtonDown(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnRightButtonUp(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnMouseMove(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnLeftButtonDown(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnLeftButtonUp(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnBalloonShown(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnBalloonTimeoutClose(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnBalloonClick(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnPopupOpen(LPVOID addr);
    long MYEAPI __stdcall mye_TrayOnPopupClose(LPVOID addr);

    //HWND MYEAPI __stdcall mye_GetWindowDC(int where, int width, int height);
    int MYEAPI __stdcall mye_DockWindow(HWND hwnd, int xwidth, int where, LPVOID addr);
    long MYEAPI __stdcall mye_UpdateBackground(HBITMAP hbmp);

    HANDLE MYEAPI __stdcall mye_RegisterWindowRect(RECT *rect, HBITMAP pic, HBITMAP onhover, TColor tclr, LPVOID callAddr,
        LPVOID param);
    void MYEAPI __stdcall mye_UnregisterWindowRect(HANDLE id);

    void MYEAPI __stdcall mye_GetOsRect(RECT *prect);
    void MYEAPI __stdcall mye_ClearWindows(void);

    HBITMAP MYEAPI __stdcall mye_GetBackgroundRect(TRect rect);

    void MYEAPI __stdcall mye_RegisterKey(char vkey, LPVOID addr);

    int MYEAPI __stdcall mye_SetVal(char*name, LPVOID buf, DWORD size, char*desc);
    long MYEAPI __stdcall mye_GetValSize(char*name);
    int MYEAPI __stdcall mye_GetVal(char*name, LPVOID buf, DWORD size);
    int MYEAPI __stdcall mye_EnumerateVal(char*prefix, int idx, char*kname, LPBYTE data);
}

/* ��������� ������� */ BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved);
DWORD WINAPI processShellCallbacks(LPVOID hinstDLL);
DWORD WINAPI processMessages(LPVOID p);
DWORD WINAPI processMessagesPipe(LPVOID p);
DWORD WINAPI processMouseMessagesPipe(LPVOID p);
void InstallTrayHook(void);
// void InstallMouseHook(void);
void IconProcess(DWORD cmd, PNOTIFYICONDATAW pn);
void FreeHooks(void);

/* private */ void _wait_for_hCbkLoop(void);
int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);
int exceptionHandler(unsigned int code, struct _EXCEPTION_POINTERS *e);
DWORD WINAPI processCallbacks(LPVOID p);
#endif
