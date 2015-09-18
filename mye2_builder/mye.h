#ifndef MYE_H
#define MYE_H
#define MYEAPI  __declspec(dllexport)

#include <vector>
#include <windows.h>
#include "nIcon.h"
enum
{
    LO_GETWINDOWHANDLE,LO_FILLRECT,LO_TEXTOUT,LO_DOCKWINDOW
};
enum
{
    LO_EVENT, LO_GETICON, LO_NOTFOUND, LO_ADDCLBK, LO_MODCLBK, LO_DELCLBK, LO_TESTCMD, LO_GETWINDOW, LO_ONPAINT
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
} EVENTS, *PEVENTS;

typedef struct _evnt
{
    LPVOID addr;
    DWORD msg;
    char dllpipe[128];
    DWORD dwIndex;
    NOTIFYICONDATAW pn;
    DWORD id;
} EVENT;

//typedef allocator<nIcon*, managed_shared_memory::segment_manager>  ShmemAllocator;

typedef std::vector<LPVOID>callbacks_v;
typedef std::vector<EVENTS>events_v;
typedef std::vector<nIcon*>icons_v;

/* �������� ���������� ������� */
typedef int(WINAPI * shell_callback_api)(LPVOID p, unsigned long size);
/* �������� ������� ������� */
typedef int(WINAPI * shell_events_api)(DWORD dwIndex, PNOTIFYICONDATAW pn);
typedef int(WINAPI * shell_events2_api)(void);

/* ��������� ������� ���������� */
typedef long(__stdcall*mye_TrayCallback_api)(int op, LPVOID addr, long id);
typedef long(__stdcall*mye_TrayAddCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayModifyCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayDeleteCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_TrayBalloonCallback_api)(LPVOID addr);
typedef long(__stdcall*mye_EnumerateIcons_api)(DWORD dwIndex, PNOTIFYICONDATAW pn);
typedef HWND(__stdcall*mye_EnumerateVisibleWindows_api)(DWORD dwIndex);
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

typedef HWND (__stdcall *mye_GetWindowDC_api)(int where, int width, int height);

typedef HWND (__stdcall *mye_DockWindow_api)(HWND hwnd, int width, int height, LPVOID addr);


typedef LRESULT(CALLBACK * CallWndProc)(int nCode, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK * MouseWndProc)(int nCode, WPARAM wParam, LPARAM lParam);

extern "C"
{

    int MYEAPI __stdcall mye_DockWindow(HWND hwnd, int xwidth, int where, LPVOID addr);

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

    HWND MYEAPI __stdcall mye_GetWindowDC(int where, int width, int height);
}

/* ��������� ������� */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved);
DWORD WINAPI processShellCallbacks(LPVOID hinstDLL);
DWORD WINAPI processMessages(LPVOID p);
DWORD WINAPI processMessagesPipe(LPVOID p);
DWORD WINAPI processMouseMessagesPipe(LPVOID p);
void InstallTrayHook(void);
//void InstallMouseHook(void);
void IconProcess(DWORD cmd, PNOTIFYICONDATAW pn);
void FreeHooks(void);

/* private */
void _wait_for_hCbkLoop(void);
int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep);
int exceptionHandler(unsigned int code, struct _EXCEPTION_POINTERS *e);
DWORD WINAPI processCallbacks(LPVOID p);
#endif
