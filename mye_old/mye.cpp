// ---------------------------------------------------------------------------
#define STRSAFE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#define DEPRECATE_SUPPORTED

#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203

#define NIN_SELECT          (WM_USER + 0)
#define NINF_KEY            0x1
#define NIN_KEYSELECT       (NIN_SELECT | NINF_KEY)
#define NIN_BALLOONSHOW         (WM_USER + 2)
#define NIN_BALLOONHIDE         (WM_USER + 3)
#define NIN_BALLOONTIMEOUT      (WM_USER + 4)
#define NIN_BALLOONUSERCLICK    (WM_USER + 5)
#define NIN_POPUPOPEN           (WM_USER + 6)
#define NIN_POPUPCLOSE          (WM_USER + 7)

#include <tchar.h>
// #include "commctrl.h"
#include <windows.h>
// #include <windowsx.h>
#include <Graphics.hpp>
#include <shellapi.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pngimage.hpp>

#include <stdio.h>
// #include <dirent.h>
#include <vector>

// #include <strsafe.h>
#include "nIcon.h"
#include <vcl.h>

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <FileCtrl.hpp>
#include <ExtCtrls.hpp>
#include <System.hpp>
#include <SysUtils.hpp>
// #include <SEHMAP.H>
// #include <excpt.h>
#include <Dbghelp.h>
#include <psapi.h>

#include "mye.h"

#include "functions.h"

CallWndProc p_CallWndProc;
MouseWndProc p_MouseWndProc;

icons_v icons;

callbacks_v shell_callbacks;
callbacks_v add_callbacks;
callbacks_v mod_callbacks;
callbacks_v del_callbacks;
callbacks_v bln_callbacks;

events_v events_leftdblclick;
events_v events_rightdblclick;
events_v events_rightbuttondown;
events_v events_rightbuttonup;
events_v events_mousemove;
events_v events_leftbuttondown;
events_v events_leftbuttonup;
events_v events_balloonshown;
events_v events_balloontimeoutclose;
events_v events_balloonclick;
events_v events_popupopen;
events_v events_popupclose;

CRITICAL_SECTION modifyCallbacks;
CRITICAL_SECTION modifyIcons;

HANDLE hCbkLoop;
HMODULE hLib;
bool already_running = false;
static int attached_processes = 0;
HINSTANCE hInst;
char dllpipe[128];
Graphics::TBitmap *BackgroundBitmap = NULL;

#define WM_TRAY_DATA WM_USER+1000

// using namespace boost::interprocess;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved)
{
    static DWORD dwShellId = 0;
    static DWORD dwMsgsId = 0;
    // Perform actions based on the reason for calling.
    switch(fwdreason)
    {
        case DLL_PROCESS_ATTACH:
        attached_processes++;

        deb("DLL_PROCESS_ATTACH #%d", attached_processes);

        hInst = hinstDLL;

        srand(GetCurrentThreadId() + (rand() % 1 ? dwMsgsId:0));
        sprintf(dllpipe, "\\\\.\\pipe\\callbacks%d", rand()+GetCurrentThreadId());
        CreateThread(NULL, 0, processCallbacks, NULL, 0, &dwMsgsId);

        InitializeCriticalSectionAndSpinCount(&modifyCallbacks, 0x80000400);
        InitializeCriticalSection(&modifyIcons);
        DisableThreadLibraryCalls(hinstDLL);
        break;

        case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        deb("DLL_THREAD_ATTACH");

        break;

        case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

        case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        attached_processes--;
        deb("DLL_PROCESS_DETACH #%d", attached_processes);
        FreeHooks();
        break;
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.

}

DWORD WINAPI processCallbacks(LPVOID p)
{
    HANDLE hPipe;
    DWORD dwRead;
    long mid = 0;

    hPipe = CreateNamedPipe(dllpipe, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 2048, 2048,
        NMPWAIT_USE_DEFAULT_WAIT, NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        deb("processCallbacks CreateNamedPipe: %s", fmterr());
        ExitThread(0);
    }

    deb("processCallbacks pipe %s (%x)", dllpipe, hPipe);

    while (1)
    {

        bool ret = ConnectNamedPipe(hPipe, NULL);
        if (!ret)
        {
            DWORD err = GetLastError();
            deb("processCallbacks ConnectNamedPipe: %s", fmterr(err));
            if (err != 535)
            {
                Sleep(500);
                continue;
            }
        }

        // deb("connected");

        EVENT e;
        memset(&e, 0, sizeof(e));
        ReadFile(hPipe, &e, sizeof(e), &dwRead, NULL);
        // deb("need to call %x msg: %x", e.addr,e.msg);
        if (dwRead == sizeof(e))
        {
            switch(e.msg)
            {
                case LO_ONKEY:
                shell_events_onkey_api api0;
                api0 = (shell_events_onkey_api)e.addr;
                api0((char)e.param);
                break;

                case LO_ONPAINT:
                shell_events2_api api1;
                api1 = (shell_events2_api)e.addr;
                // deb("calling %x(%d, %x)", api, e.dwIndex, &e.pn);
                api1();
                break;

                case LO_ONBCLICK:

                shell_events_onbclick_api f;
                f = (shell_events_onbclick_api)e.addr;
                // deb("click addr %p(%x, %x)", f, e.param, e.button);
                f(e.param, e.button);
                break;

                case LO_EVENT:
                shell_events_api api2;
                api2 = (shell_events_api)e.addr;
                // deb("calling %x(%d, %x)", api, e.dwIndex, &e.pn);
                api2(e.dwIndex, (PNOTIFYICONDATAW) &e.pn);
                break;

                default:
                shell_callback_api api3;
                api3 = (shell_callback_api)e.addr;
                api3((LPVOID) &e.pn, sizeof(NOTIFYICONDATAW));
                break;
            }

        }
        else
        {
            // deb("too small msg %3d EVENT(%d) %4d bytes (msg %d)", mid, sizeof(EVENT), dwRead, e.msg);
        }

        WriteFile(hPipe, NULL, 0, &dwRead, NULL);
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        mid++;
    }
}

int cmd_event(LPVOID addr, DWORD msg)
{
    DWORD dwRead;
    char buf[2000];

#pragma pack(1)
    typedef struct
    {
        char cmd;
        EVENT e;
    }abcd;
#pragma pop
    abcd aa;

    memset(&aa.e, 0, sizeof(abcd));
    aa.cmd = LO_EVENT;
    aa.e.addr = addr;
    aa.e.msg = msg;
    aa.e.id = rand();
    strncpy(aa.e.dllpipe, dllpipe, sizeof(aa.e.dllpipe));
    // memcpy(buf+1, &e, sizeof(EVENT));

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\cmdmsgs", &aa, sizeof(aa), buf, 4, &dwRead, 0);
    if (!ret2)
        deb("cmd_event CallNamedPipe: %s", fmterr());

    return MYE_SUCCESS;
}

extern "C" void __stdcall mye_GetOsRect(RECT *prect)
{
    int h, w;

    h = GetSystemMetrics(SM_CYSCREEN);
    w = GetSystemMetrics(SM_CXSCREEN);

    prect->left = 0;
    prect->top = 0;
    prect->bottom = h;
    prect->right = w;
}

extern "C" void __stdcall mye_RegisterKey(char vkey, LPVOID addr)
{
    DWORD dwRead;
    typedef struct
    {
        char cmd;
        char dllpipe[128];
        char key;
        LPVOID addr;
    }ks;
    ks k;

    k.cmd = LO_REGISTERKEY;
    k.key = vkey;
    k.addr = addr;
    strcpy(k.dllpipe, dllpipe);

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", &k, sizeof(k), &dwRead, sizeof(dwRead), &dwRead, 0);
    if (!ret2)
        deb("mye_RegisterKey CallNamedPipe: %s", fmterr());
}

extern "C" void __stdcall mye_ClearWindows(void)
{
    char cmd = LO_CLEARWINDOWS;
    DWORD dwRead;

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", &cmd, sizeof(cmd), &dwRead, sizeof(dwRead), &dwRead, 0);
    if (!ret2)
        deb("mye_ClearWindows CallNamedPipe: %s", fmterr());
}

extern "C" void __stdcall mye_UnregisterWindowRect(HANDLE id)
{
    DWORD dwRead;
    typedef struct
    {
        char cmd;
        HANDLE id;
    }abc;
    abc a;
    a.cmd = LO_DEREGISTERWINDOW;
    a.id = id;
    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", &a, sizeof(abc), &dwRead, sizeof(dwRead), &dwRead, 0);
    if (!ret2)
        deb("mye_UnregisterWindowRect CallNamedPipe: %s", fmterr());
}

extern "C" HANDLE __stdcall mye_RegisterWindowRect(RECT *rect, HBITMAP pic, HBITMAP onhover, TColor tclr, LPVOID addr,
    LPVOID param)
{
    char buf[1024];
    HDC hdc = 0;
    DWORD dwRead;
    HANDLE id = NULL;
#pragma pack(1)

    typedef struct temp
    {
        char cmd;
        LPVOID param;
        LPVOID addr;
        char dllpipe[128];
        RECT rect;
        DWORD procId;
        TColor tclr;
    }tmp2;
#pragma pop

    tmp2 stmp;
    stmp.procId = GetCurrentProcessId();

    stmp.cmd = LO_REGISTERWINDOW;
    stmp.param = param;
    strncpy(stmp.dllpipe, dllpipe, sizeof(stmp.dllpipe));
    stmp.tclr = tclr;
    stmp.addr = addr;

    memcpy((void*) &stmp.rect, (void*)rect, sizeof(RECT));

    DWORD err = 231;
    DWORD numtr = 0;
    HANDLE hPipe;
    while (err == 231)
    {
        hPipe = CreateFile("\\\\.\\pipe\\ReqForm", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
        if (INVALID_HANDLE_VALUE == hPipe)
        {
            err = GetLastError();
            deb("mye_RegisterWindowRect createfile hpipe: %s", fmterr(err));
            if (numtr++>=100)
                return(HANDLE)-1;
        }
        else
        {
            err = 0;
        }

    }
    WriteFile(hPipe, &stmp, sizeof(stmp), &dwRead, NULL);

    long size;

    BITMAPINFO bmi;

    // write pic 1
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    hdc = CreateCompatibleDC(0);

    GetDIBits(hdc, (HBITMAP)pic, 0, 0, 0, &bmi, DIB_RGB_COLORS);

    long *pbuf;
    pbuf = new long[bmi.bmiHeader.biSizeImage*2];

    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(hdc, (HBITMAP)pic, 0, bmi.bmiHeader.biHeight, pbuf, &bmi, DIB_PAL_COLORS) == 0)
    {
        deb("mye_RegisterWindowRect getdibits: %s", fmterr());
    }
    if (!DeleteDC(hdc))
        deb("deletedc: %s", fmterr());

    size = bmi.bmiHeader.biSizeImage;
    WriteFile(hPipe, &size, sizeof(size), &dwRead, NULL);

    WriteFile(hPipe, &bmi, sizeof(bmi), &dwRead, NULL);
    // deb("size: %d", size);
    WriteFile(hPipe, pbuf, size, &dwRead, NULL);
    delete[]pbuf;

    // write pic 2

    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    hdc = CreateCompatibleDC(0);

    GetDIBits(hdc, (HBITMAP)onhover, 0, 0, 0, &bmi, DIB_RGB_COLORS);

    pbuf = new long[bmi.bmiHeader.biSizeImage*2];

    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(hdc, (HBITMAP)onhover, 0, bmi.bmiHeader.biHeight, pbuf, &bmi, DIB_PAL_COLORS) == 0)
    {
        deb("mye_RegisterWindowRect getdibits: %s", fmterr());
    }
    if (!DeleteDC(hdc))
        deb("deletedc: %s", fmterr());

    size = bmi.bmiHeader.biSizeImage;
    WriteFile(hPipe, &size, sizeof(size), &dwRead, NULL);

    WriteFile(hPipe, &bmi, sizeof(bmi), &dwRead, NULL);
    // deb("size: %d", size);
    WriteFile(hPipe, pbuf, size, &dwRead, NULL);
    delete[]pbuf;

    // read id

    ReadFile(hPipe, &id, sizeof(id), &dwRead, NULL);

    // deb("read hwnd: %x dwRead:%u hdc %x",hwnd,dwRead,hdc);
    return id;
}

extern "C" int __stdcall mye_DockWindow(HWND hwnd, int xwidth, int where, LPVOID addr)
{
    char buf[1024];
    HDC hdc = 0;
    DWORD dwRead;
#pragma pack(1)

    typedef struct temp
    {
        char cmd;
        HWND hwnd;
        LPVOID addr;
        char dllpipe[128];
    }tmp;
#pragma pop

    tmp stmp;

    stmp.cmd = LO_DOCKWINDOW;
    stmp.hwnd = hwnd;
    strncpy(stmp.dllpipe, dllpipe, sizeof(stmp.dllpipe));
    stmp.addr = addr;
    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", &stmp, sizeof(stmp), &hwnd, sizeof(hwnd), &dwRead, 0);
    if (!ret2)
        deb("mye_DockWindow CallNamedPipe: %s", fmterr());
    deb("dockwindow wrote %d bytes", sizeof(stmp));

    // deb("read hwnd: %x dwRead:%u hdc %x",hwnd,dwRead,hdc);
    return TRUE;
}

extern "C" HBITMAP __stdcall mye_GetBackgroundRect(TRect rect)
{
    char buf[1024];
    HDC hdc = 0;
    DWORD dwRead;
    Graphics::TBitmap *bmp = NULL;


    bmp = new Graphics::TBitmap();

#pragma pack(1)

    typedef struct temp
    {
        char cmd;
        TRect rect;
    }tmp;
#pragma pop

    tmp stmp;

    stmp.cmd = LO_GETBKGNDRECT;
    stmp.rect = rect;
    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", &stmp, sizeof(stmp), &hdc, sizeof(hdc), &dwRead, 0);
    if (!ret2)
        deb("mye_GetBackgroundRect CallNamedPipe: %s", fmterr());
    deb("mye_GetBackgroundRect call named pipe done");

    DWORD mapsize = 3*1024*1024;
    HANDLE hMapBuf = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, mapsize, "Global\\FormMapBuf");
    // deb("hMapBuf client %x", hMapBuf);
    char *pmbuf = (char*)MapViewOfFile(hMapBuf, FILE_MAP_WRITE, 0, 0, 0);

    // copy bitmap
    BITMAPINFO *bmi;

    bmi = (BITMAPINFO*)pmbuf;
    deb("bmi @ %p size: %dX%d", bmi, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight);

    deb("setting size %dX%d", bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight);
    bmp->SetSize(bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight);
    deb("setsize done");
    char *pdata = pmbuf +sizeof(BITMAPINFO);
    deb("pdata @ %p", pdata);
    hdc = CreateCompatibleDC(0);
    int ret = SetDIBits(hdc, bmp->Handle, 0, bmi->bmiHeader.biHeight, pdata, bmi, DIB_PAL_COLORS);
    if (!ret)
        deb("mye_GetBackgroundRect setdibits: %s", fmterr());
    DeleteDC(hdc);

    // close map view
    UnmapViewOfFile(pmbuf);
    CloseHandle(hMapBuf);

    return bmp->Handle;
}

extern "C" long __stdcall mye_UpdateBackground(HBITMAP hbmp)
{

    HDC hdc = 0;
    static long cid = 0;
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    hdc = CreateCompatibleDC(0);

    GetDIBits(hdc, (HBITMAP)hbmp, 0, 0, 0, &bmi, DIB_RGB_COLORS);
#pragma pack(1)
    typedef struct
    {
        char cmd;
        DWORD procId;
        BITMAPINFO bmi;

    }sbmp;
#pragma pop

    DWORD mapsize = bmi.bmiHeader.biSize;
    HANDLE hMapBuf = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, mapsize, "Global\\FormMapBuf");
    // deb("hMapBuf client %x", hMapBuf);
    char *pmbuf = (char*)MapViewOfFile(hMapBuf, FILE_MAP_WRITE, 0, 0, 0);

    sbmp *bmpupd;
    DWORD tmp3;
    bmpupd = (sbmp*)new char[sizeof(sbmp)];

    // char *data = new char[bmi.bmiHeader.biSizeImage];

    bmpupd->procId = GetCurrentProcessId();
    // bmi.bmiHeader.biHeight = abs(bmi.bmiHeader.biHeight);

    // bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(hdc, (HBITMAP)hbmp, 0, bmi.bmiHeader.biHeight, pmbuf, &bmi, DIB_PAL_COLORS) == 0)
    {
        deb("mye_UpdateBackground getdibits: %s", fmterr());
    }
    if (!DeleteDC(hdc))
        deb("deletedc: %s", fmterr());

    bmpupd->cmd = LO_UPDATEBGBITMAP;

    memcpy(&bmpupd->bmi, &bmi, sizeof(bmi));

    // memcpy(bmpupd->data, data, bmi.bmiHeader.biSizeImage);

    DWORD dwRead;
    DWORD size = sizeof(sbmp);

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", bmpupd, size, &tmp3, sizeof(tmp3), &dwRead, 0);
    // if (!ret2)
    // deb("mye_UpdateBackground #%03d CallNamedPipe: %s", cid, fmterr());
    // delete[]data;
    delete[]bmpupd;
    cid++;

    UnmapViewOfFile(pmbuf);

    CloseHandle(hMapBuf);

    return size;
}

extern "C" int __stdcall mye_GetVal(char *name, LPVOID buf, DWORD size)
{
    HKEY hKey;
    char keyname[129];
    sprintf(keyname, "Software\\myexplorer\\ctl\\%s", name);
    long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_ALL_ACCESS, &hKey);
    if (ret != ERROR_SUCCESS)
    {
        return ret;
    }

    DWORD size2 = size;
    DWORD type = REG_BINARY;
    RegQueryValueEx(hKey, "value", 0, &type, (unsigned char*)buf, &size2);

    RegCloseKey(hKey);

    return size2;
}

extern "C" int __stdcall mye_SetVal(char *name, LPVOID buf, DWORD size, char *desc = NULL)
{
    HKEY hKey;
    long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\ctl", 0, KEY_ALL_ACCESS, &hKey);
    if (ret != ERROR_SUCCESS)
    {
        if (ret == 2)
        {
            deb("creating settings key");
            RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\ctl", &hKey);
        }
        else
        {

            deb("ret: %d", ret);
        }
    }

    char subkey[128];
    sprintf(subkey, "%s", name);

    HKEY hKey2;
    DWORD dw = sizeof(DWORD);
    RegCreateKey(hKey, subkey, &hKey2);
    ret = RegSetValueEx(hKey2, "value", 0, REG_BINARY, (unsigned char*)buf, size);
    if (desc)
        RegSetValueEx(hKey2, "description", 0, REG_SZ, (unsigned char*)desc, strlen(desc));

    RegCloseKey(hKey);
    RegCloseKey(hKey2);
    return ret;
}

extern "C" long __stdcall mye_GetValSize(char *name)
{
    HKEY hKey;
    char keyname[129];

    sprintf(keyname, "Software\\myexplorer\\ctl\\%s", name);
    long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_ALL_ACCESS, &hKey);
    if (ret != ERROR_SUCCESS)
    {
        return ret;
    }
    DWORD size2 = 0;
    DWORD type = REG_BINARY;
    RegQueryValueEx(hKey, "value", 0, &type, NULL, &size2);

    RegCloseKey(hKey);

    return size2;
}

extern "C" int __stdcall mye_EnumerateVal(char *prefix = NULL, int idx = 0, char *kname = NULL, LPBYTE data = NULL)
{
    int found = 0;

    char name[128];
    HKEY hkey;

    long ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\ctl", 0, KEY_ALL_ACCESS, &hkey);
    if (ret != ERROR_SUCCESS)
    {
        return ret;
    }
    DWORD size2 = 0;

    int eidx = 0;
    DWORD dwsname = sizeof(name);
    // deb("enum vals %s",prefix);
    while (RegEnumKeyEx(hkey, eidx, name, &dwsname, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS)
    {
        // deb(" key #%03d %s",eidx,name);
        dwsname = sizeof(name);
        if (prefix)
            if (!strncmpi(name, prefix, strlen(prefix)))
            {
                eidx++;
                continue;
            }

        if (eidx == idx)
        {
            DWORD type = REG_BINARY;
            HKEY hkey2;

            char keyname[129];
            sprintf(keyname, "Software\\myexplorer\\ctl\\%s", name);
            // deb("open %s",keyname);
            strncpy(kname, name, 129);
            RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_ALL_ACCESS, &hkey2);
            size2 = 1024;
            RegQueryValueEx(hkey2, "value", 0, &type, data, &size2);
            RegCloseKey(hkey2);
            found++;
            break;
        }
        eidx++;
    }

    RegCloseKey(hkey);

    return found;
}

extern "C" HWND __stdcall mye_GetWindowDC(int where, int width, int height)
{
    char buf[1024];
    HDC hdc = 0;
    HWND hwnd;
    DWORD dwRead;

    buf[0] = LO_GETWINDOWHANDLE;
    buf[1] = 0x0;
    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\ReqForm", buf, 2, &hwnd, sizeof(hwnd), &dwRead, 0);
    if (!ret2)
        deb("mye_GetWindowDC CallNamedPipe: %s", fmterr());

    deb("read hwnd: %x dwRead:%u hdc %x", hwnd, dwRead, hdc);
    return hwnd;
}

extern "C" long __stdcall mye_TrayOnLeftDblClick(LPVOID addr)
{
    return cmd_event(addr, WM_LBUTTONDBLCLK);
}

extern "C" long __stdcall mye_TrayOnRightDblClick(LPVOID addr)
{
    return cmd_event(addr, WM_RBUTTONDBLCLK);
}

extern "C" long __stdcall mye_TrayOnRightButtonDown(LPVOID addr)
{
    return cmd_event(addr, WM_RBUTTONDOWN);
}

extern "C" long __stdcall mye_TrayOnRightButtonUp(LPVOID addr)
{
    return cmd_event(addr, WM_RBUTTONUP);
}

extern "C" long __stdcall mye_TrayOnMouseMove(LPVOID addr)
{

    return cmd_event(addr, WM_MOUSEMOVE);
}

extern "C" long __stdcall mye_TrayOnLeftButtonDown(LPVOID addr)
{
    return cmd_event(addr, WM_LBUTTONDOWN);
}

extern "C" long __stdcall mye_TrayOnLeftButtonUp(LPVOID addr)
{
    return cmd_event(addr, WM_LBUTTONUP);
}

extern "C" long __stdcall mye_TrayOnBalloonShown(LPVOID addr)
{
    return cmd_event(addr, NIN_BALLOONSHOW);
}

extern "C" long __stdcall mye_TrayOnBalloonTimeoutClose(LPVOID addr)
{
    return cmd_event(addr, NIN_BALLOONTIMEOUT);
}

extern "C" long __stdcall mye_TrayOnBalloonClick(LPVOID addr)
{
    return cmd_event(addr, NIN_BALLOONUSERCLICK);
}

extern "C" long __stdcall mye_TrayOnPopupOpen(LPVOID addr)
{
    return cmd_event(addr, NIN_POPUPOPEN);
}

extern "C" long __stdcall mye_TrayOnPopupClose(LPVOID addr)
{
    return cmd_event(addr, NIN_POPUPCLOSE);
}

int cmd_geticon(int dwIndex, PNOTIFYICONDATAW pn)
{
    DWORD dwRead;
    char buf[2000];

    buf[0] = LO_GETICON;
    buf[1] = (char)dwIndex;

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\cmdmsgs", buf, 2, pn, sizeof(NOTIFYICONDATAW), &dwRead, 0);
    if (!ret2)
        deb("cmd_geticon CallNamedPipe: %s", fmterr());

    if (dwRead != sizeof(NOTIFYICONDATAW))
    {
        return MYE_NO_MORE_ITEMS;
    }
    return MYE_SUCCESS;
}

extern "C" long __stdcall mye_EnumerateIcons(DWORD dwIndex, PNOTIFYICONDATAW pn)
{

    return cmd_geticon(dwIndex, pn);
}

HWND cmd_getwindow(int dwIndex)
{
    DWORD dwRead;
    char buf[2000];
    HWND hwnd;

    buf[0] = LO_GETWINDOW;
    buf[1] = (char)dwIndex;
    // deb("dwIndex %d",dwIndex);

    unsigned long ret2 = CallNamedPipeA("\\\\.\\pipe\\cmdmsgs", buf, 2, &hwnd, sizeof(HWND), &dwRead, 0);
    if (!ret2)
        deb("cmd_getwindow CallNamedPipe: %s", fmterr());
    // deb("dwRead %d",dwRead);
    if (dwRead != sizeof(HWND))
    {
        return(HWND)MYE_NO_MORE_ITEMS;
    }
    // deb("getwindow idx %d hwnd %x",dwIndex,hwnd);
    return hwnd;
}

extern "C" long __stdcall mye_ClickWindow(DWORD dwIndex)
{
    // deb("enum windows idx %d",dwIndex);
    return SetForegroundWindow(cmd_getwindow(dwIndex));
}

extern "C" HWND __stdcall mye_EnumerateVisibleWindows(DWORD dwIndex)
{
    // deb("enum windows idx %d",dwIndex);
    return cmd_getwindow(dwIndex);
}

int exceptionHandler(unsigned int code, struct _EXCEPTION_POINTERS *e)
{
    char strCode[128];
    bool canContinue;
    int numException = 0;
    char szTemp[1024];
    PEXCEPTION_POINTERS eNext = e;
    PEXCEPTION_RECORD excRec;
    static DWORD numCall = 0;

    if (numCall++ >= 4)
        ExitProcess(0);

    _snprintf(szTemp, sizeof(szTemp), "!!! exceptionHandler(0x%08p)->0x%08p  [ %03d ] *************************** !!!",
        e, e->ExceptionRecord, numCall);
    // OutputDebugString(szTemp);
    deb(szTemp);

    _snprintf(szTemp, sizeof(szTemp),
        "eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X\r\nesi=0x%08X edi=0x%08X esp=0x%08X ebp=0x%08X",
        eNext->ContextRecord->Eax, eNext->ContextRecord->Ebx, eNext->ContextRecord->Ecx, eNext->ContextRecord->Edx,
        eNext->ContextRecord->Esi, eNext->ContextRecord->Edi, eNext->ContextRecord->Esp, eNext->ContextRecord->Ebp);
    // OutputDebugString(szTemp);
    deb(szTemp);

    do
    {
        excRec = eNext->ExceptionRecord;

        numException++;

        // deb(" -> # %-4d 0x%08p", numException, excRec);
        // deb("code: %x", excRec->ExceptionCode);

        canContinue = excRec->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? true:false;
        // deb("canContinue: %d", canContinue);

        if (excRec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            _snprintf(strCode, sizeof(strCode), "Access violation (%s @ 0x%08p)",
                excRec->ExceptionInformation[0] ? "WRITE":"READ", excRec->ExceptionInformation[1]);
        }
        else
            if (excRec->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
            {
                _snprintf(strCode, sizeof(strCode), "Access in-page violation (%s @ 0x%08p) ntcode: %d",
                    excRec->ExceptionInformation[0] ? "READ":"WRITE", excRec->ExceptionInformation[1],
                    excRec->ExceptionInformation[2]);
            }
            else
            {
                _snprintf(strCode, sizeof(strCode), "<unkcode=%X> d0: 0x%08x d1: 0x%08x d2: 0x%08x",
                    excRec->ExceptionCode, excRec->ExceptionInformation[0], excRec->ExceptionInformation[1],
                    excRec->ExceptionInformation[2]);
            }

        _snprintf(szTemp, sizeof(szTemp), "Exception 0x%08X %s Address=0x%08X canContinue=%s", excRec->ExceptionCode,
            strCode, excRec->ExceptionAddress, canContinue ? "yes":"no");
        deb(szTemp);

        if (!eNext->ExceptionRecord->ExceptionRecord)
            break;

        excRec = eNext->ExceptionRecord->ExceptionRecord;
        eNext->ExceptionRecord = eNext->ExceptionRecord->ExceptionRecord;

    }
    while (excRec->ExceptionRecord);

    // find symbols
    LPAPI_VERSION av = ImagehlpApiVersion();
    // deb("dbghelp.dll: %d.%d rev:%d", av->MajorVersion, av->MinorVersion, av->Revision);

    int ret = SymInitialize(GetCurrentProcess(), "c:\\symbols", TRUE);
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_DEFERRED_LOADS | 0x01000000 |
        SYMOPT_CASE_INSENSITIVE | SYMOPT_LOAD_ANYTHING | SYMOPT_LOAD_LINES);
    if (!ret)
        deb("syminit failed: code=%d, %s", ret, fmterr());

    PSYMBOL_INFO si;
    unsigned long stackPtr = e->ContextRecord->Esp;
    DWORD dwDisp = false;
    unsigned __int64 dwDisp64 = false;
    LPVOID addr;

    si = (PSYMBOL_INFO)malloc(4096);

    for (int i = 0;i<100;i++)
    {
        memset(si, 0, 4096);
        // si.Name = (char*) a(128);
        si->MaxNameLen = 3000;
        si->SizeOfStruct = sizeof(SYMBOL_INFO) + 3000;

        memcpy((void*) &addr, (void*)stackPtr, 4);

        char szTemp[128];

        memset(szTemp, 0, sizeof(szTemp));
        si->Address = (unsigned __int64)addr;
        // get sym module name
        ret = SymFromAddr(GetCurrentProcess(), (unsigned __int64)addr, &dwDisp64, si);
        // if(ret)
        _snprintf(szTemp, sizeof(szTemp), "  %03d 0x%08X :%-25s", i, addr, si->Name);

        // get sym module line number
        IMAGEHLP_LINE64 il;

        memset(&il, 0, sizeof(il));
        il.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        il.Address = (unsigned __int64)addr;
        ret = SymGetLineFromAddr64(GetCurrentProcess(), (unsigned __int64)addr, &dwDisp, &il);

        char szTemp2[128];

        memset(szTemp2, 0, sizeof(szTemp2));

        if (il.LineNumber)
            _snprintf(szTemp2, sizeof(szTemp2), "  %s:%d (0x%08X)", il.FileName, il.LineNumber, il.Address);

        // show line info

        if (addr && (strlen(szTemp) || strlen(szTemp2)))
            deb("  %s %s", szTemp, szTemp2[0] ? szTemp2:"");

        stackPtr -= 4;
    }
    // ret=SymEnumSymbols(GetCurrentProcess(), 0, "!", EnumSymProc, NULL);
    // if(!ret)
    // deb("enum err: %s", fmterr());

    deb("!!! exceptionHandler out\r\n");

    // EXCEPTION_CONTINUE_EXECUTION
    // EXCEPTION_EXECUTE_HANDLER

    return EXCEPTION_EXECUTE_HANDLER;
}

void FreeHooks(void)
{
    // UnhookWindowsHookEx(mousehook);
    // UnhookWindowsHookEx(trayhook);
}

// DWORD WINAPI processShellCallbacks(LPVOID hinstDLL)
// {
// static HKEY shellKey = NULL;
//
// deb("processShellCallbacks %x", GetCurrentThreadId());
//
// while (1)
// {
//
// DWORD dwIndex;
// char keyname[128];
// long ret;
//
// if (!shellKey)
// {
// ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\shelltray", 0, KEY_ALL_ACCESS, &shellKey);
// if (ret == 2)
// {
// // deb("no shelltray key");
// Sleep(10);
// continue;
// }
// if (ret != ERROR_SUCCESS)
// {
// deb("mye.cpp: RegOpenKeyEx shelltray: %s", fmterr(ret));
// continue;
// }
// }
// dwIndex = 0;
//
// while ((ret = RegEnumKey(shellKey, dwIndex, keyname, sizeof(keyname))) == ERROR_SUCCESS)
// {
// // deb("processShellCallbacks dwIndex: %d keyname %s", dwIndex, keyname);
// HKEY keyHkey;
//
// ret = RegOpenKey(shellKey, keyname, &keyHkey);
// if (ret != ERROR_SUCCESS)
// {
// deb("RegOpenKey ret for %s: %s", keyname, fmterr(ret));
// dwIndex++;
// continue;
// }
//
// DWORD status;
// DWORD datasize = sizeof(status);
// ret = RegQueryValueEx(keyHkey, "status", 0, 0, (unsigned char*) & status, &datasize);
// if (ret != ERROR_SUCCESS)
// {
// // deb("mye.cpp: RegQueryValueEx('data') ret for %s: %s", keyname, fmterr(ret));
// RegCloseKey(keyHkey);
// dwIndex++;
// continue;
// }
//
// if (status == 1)
// {
// deb("already processed packet %s", keyname);
// dwIndex++;
// RegCloseKey(keyHkey);
// continue;
// }
//
// unsigned char data[1000] = "123";
// datasize = sizeof(data);
// ret = RegQueryValueEx(keyHkey, "data", 0, 0, data, &datasize);
// if (ret != ERROR_SUCCESS)
// {
// // deb("mye.cpp: RegQueryValueEx('data') ret for %s: %s", keyname, fmterr(ret));
// RegCloseKey(keyHkey);
// dwIndex++;
// continue;
// }
//
// DWORD size;
// bool modified = false;
// for (callbacks_v::iterator it = shell_callbacks.begin();it != shell_callbacks.end();it++)
// {
//
// char strcbkid[128];
// sprintf(strcbkid, "cbk:%x", (*it));
// size = sizeof(strcbkid);
// ret = RegQueryValueEx(keyHkey, strcbkid, 0, 0, (LPBYTE)strcbkid, &size);
// if (ret == ERROR_SUCCESS)
// {
// // deb("%s already processed by cbk %x", keyname, (*it));
// continue;
// }
// deb("processing callback %x for packed %s", (*it), keyname);
//
// modified = true;
// shell_callback_api p_shell_callback_api = (shell_callback_api)(*it);
//
// if (IsBadCodePtr((FARPROC)p_shell_callback_api))
// {
// deb("callback removed while processing callbacks");
// break;
// }
// ret = p_shell_callback_api(data, datasize);
//
// char str[32];
// sprintf(str, "cbk:%x", (*it));
//
// ret = RegSetValueEx(keyHkey, str, 0, REG_SZ, (LPBYTE)keyname, strlen(keyname));
// if (ret != ERROR_SUCCESS)
// {
// deb("mye.cpp: RegSetValueEx('%s') ret for %s: %s", str, keyname, fmterr(ret));
// continue;
// }
// }
//
// ret = RegSetValueEx(keyHkey, "data", 0, REG_BINARY, data, datasize);
// if (ret != ERROR_SUCCESS)
// {
// deb("mye.cpp: RegSetValueEx('data') ret for %s: %s", keyname, fmterr(ret));
// }
//
// DWORD dw = 1;
// ret = RegSetValueEx(keyHkey, "status", 0, REG_DWORD, (unsigned char*) & dw, sizeof(DWORD));
// if (ret != ERROR_SUCCESS)
// {
// deb("mye.cpp: RegSetValueEx('status') ret for %s: %s", keyname, fmterr(ret));
// }
//
// RegCloseKey(keyHkey);
// dwIndex++;
// }
// // deb("processShellCallbacks waiting for changes");
//
// static HANDLE hEvent = NULL;
// if (!hEvent)
// hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//
// ret = RegNotifyChangeKeyValue(shellKey, true, REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME, hEvent,
// true);
// if (ret != ERROR_SUCCESS)
// deb("mye.cpp: RegNotifyChangeKeyValue: %s", fmterr(ret));
//
// if (WaitForSingleObject(hEvent, 300) == WAIT_FAILED)
// {
// deb("Error in WaitForSingleObject: %s", fmterr());
// }
// ResetEvent(hEvent);
// // RegCloseKey(shellKey);
// }
// }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void _wait_for_hCbkLoop(void)
{
    long iter = 0;
    HANDLE hm;
    // deb("waiting for hCbkLoop");

    while ((hm = OpenMutex(NULL, false, "Global\\hCbkLoop"))!=NULL)
    {
        // Sleep(100),iter++;
        CloseHandle(hm);

        long ret = WaitForSingleObject(hCbkLoop, 200);
        // deb(" hCbkLoop wtng #%d %X", iter++, ret);
    }

    // deb("done hCbkLoop iter:%d", iter);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" long __stdcall mye_RightClickIcon(BYTE dwIndex)
{
    NOTIFYICONDATAW pn;

    cmd_geticon(dwIndex, &pn);

    deb("dwIndex %d sending right click to hwnd: %x callback: %x uid: %d", dwIndex, pn.hWnd, pn.uCallbackMessage,
        pn.uID);

    PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, WM_RBUTTONDOWN);
    return PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, WM_RBUTTONUP);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" long __stdcall mye_ClickIcon(BYTE dwIndex)
{
    NOTIFYICONDATAW pn;

    cmd_geticon(dwIndex, &pn);

    char str[1024];
    deunicode(pn.szTip, str, sizeof(str));
    deb("dwIndex %d sending click to hwnd: %x tip: %s callback: %u uid: %d", dwIndex, pn.hWnd, str,
        pn.uCallbackMessage, pn.uID);

    PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, BM_CLICK);
    PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, WM_LBUTTONDOWN);
    return PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, WM_LBUTTONUP);

}

extern "C" long __stdcall mye_DblClickIcon(BYTE dwIndex)
{
    NOTIFYICONDATAW pn;

    cmd_geticon(dwIndex, &pn);

    return PostMessage(pn.hWnd, pn.uCallbackMessage, pn.uID, WM_LBUTTONDBLCLK);

}

extern "C" long __stdcall mye_TrayAddCallback(LPVOID addr)
{
    return cmd_event(addr, LO_ADDCLBK);
}

extern "C" long __stdcall mye_TrayModifyCallback(LPVOID addr)
{
    return cmd_event(addr, LO_MODCLBK);
}

extern "C" long __stdcall mye_TrayDeleteCallback(LPVOID addr)
{
    return cmd_event(addr, LO_DELCLBK);
}

extern "C" long __stdcall mye_TrayBalloonCallback(LPVOID addr)
{
    _wait_for_hCbkLoop();
    // deb("Balloon callback entering critical section. addr %x", addr);
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 100)
    {
        deb("removing Balloon callback id %d", addr);
        unsigned pos = 0;
        for (callbacks_v::iterator it = bln_callbacks.begin();it != bln_callbacks.end();it++)
        {
            if (pos++ == (unsigned)addr)
            {
                bln_callbacks.erase(it);
                break;
            }
        }
    }
    else
        bln_callbacks.push_back(addr);
    // LeaveCriticalSection(&modifyCallbacks);

    return bln_callbacks.size() - 1;
}

extern "C" long __stdcall mye_TrayCallback(int op, LPVOID addr, long id)
{
    static unsigned long shell_callback_id = 0;
    long ret = 0;
    HKEY hKey;
    bool found = false;
    char str[16];
    DWORD dwCmd;
    LPVOID apiid;
    LPVOID apiaddr;
    HKEY hKeyApi;

    switch(op)
    {
        case MYE_ADD_SHELL_CALLBACK:

        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\shellapi", 0, KEY_ALL_ACCESS, &hKey);
        if (ret != ERROR_SUCCESS)
        {
            deb("MYE_ADD_SHELL_CALLBACK RegOpenKeyEx shellapi ret: %d", ret);
            break;
        }

        sprintf(str, "%u-%u-%u", shell_callback_id, rand(), GetCurrentThreadId());

        RegCreateKey(hKey, str, &hKeyApi);
        dwCmd = MYE_ADD_SHELL_CALLBACK;
        RegSetValueEx(hKeyApi, "cmd", 0, REG_DWORD, (unsigned char*) & dwCmd, sizeof(dwCmd));
        apiid = (LPVOID)shell_callback_id;
        RegSetValueEx(hKeyApi, "id", 0, REG_BINARY, (unsigned char*) & apiid, sizeof(apiid));
        apiaddr = addr;
        RegSetValueEx(hKeyApi, "addr", 0, REG_BINARY, (unsigned char*) & apiaddr, sizeof(apiaddr));
        RegCloseKey(hKeyApi);
        RegCloseKey(hKey);

        shell_callbacks.push_back(addr);
        deb("mye.cpp: added callback %d @ 0x%X", shell_callback_id, addr);
        ret = shell_callback_id++;
        break;

        case MYE_DEL_SHELL_CALLBACK:
        found = false;
        for (callbacks_v::iterator it = shell_callbacks.begin();it != shell_callbacks.end();it++)
        {
            if ((*it) == addr)
            {
                found = true;
                deb("callback %x deleted", (*it));
                shell_callbacks.erase(it);
                break;
            }
        }
        if (found)
        {
            ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\shellapi", 0, KEY_ALL_ACCESS, &hKey);
            if (ret != ERROR_SUCCESS)
            {
                deb("MYE_DEL_SHELL_CALLBACK RegOpenKeyEx shellapi ret: %d", ret);
                break;
            }
            sprintf(str, "%u", id);

            RegCreateKey(hKey, str, &hKeyApi);
            dwCmd = MYE_DEL_SHELL_CALLBACK;
            RegSetValueEx(hKeyApi, "cmd", 0, REG_DWORD, (unsigned char*) & dwCmd, sizeof(dwCmd));
            apiid = (LPVOID)id;
            RegSetValueEx(hKeyApi, "id", 0, REG_BINARY, (unsigned char*) & apiid, sizeof(apiid));
            apiaddr = addr;
            RegSetValueEx(hKeyApi, "addr", 0, REG_BINARY, (unsigned char*) & apiaddr, sizeof(apiaddr));
            RegCloseKey(hKeyApi);
            RegCloseKey(hKey);
            ret = 0;
        }
        else
            ret = -1;
        break;
    }
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __stdcall InstallService(void)
{
    return 0;
}
