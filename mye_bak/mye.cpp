// ---------------------------------------------------------------------------
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

#include <boost/interprocess/managed_shared_memory.hpp>

#include <vcl.h>
#include <tchar.h>
#include "commctrl.h"
#include <windows.h>
#include <windowsx.h>



#include <shellapi.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include "strsafe.h"
#include "nIcon.h"
#include "Graphics.hpp"
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Dialogs.hpp>
#include <FileCtrl.hpp>
#include <ExtCtrls.hpp>
#include <System.hpp>
#include <SysUtils.hpp>
#include <SEHMAP.H>
#include <excpt.h>
#include <Dbghelp.h>
#include <psapi.h>
#pragma link "madExcept"
#pragma link "madLinkDisAsm"
#pragma hdrstop
#include "mye.h"

#include "functions.h"

#pragma argsused

CallWndProc p_CallWndProc;
MouseWndProc p_MouseWndProc;

PSHELLTRAYDATA pstd;
COPYDATASTRUCT *cd;
HINSTANCE hInst;
HHOOK trayhook;
HHOOK mousehook;

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

icons_v icons;

CRITICAL_SECTION modifyCallbacks;
CRITICAL_SECTION modifyIcons;

HANDLE hCbkLoop;
HMODULE hLib;
bool already_running = false;
static int attached_processes = 0;

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
        // try {
        // named_mutex mutex(open_or_create, "myeDllLoaded");
        // }
        // catch(interprocess_exception& e)
        // {
        // already_running=true;
        // deb("another instance of mye lib running");
        // }
        char sz1[MAX_PATH];
        GetModuleFileNameExA(GetCurrentProcess(), NULL, sz1, sizeof(sz1));
        CharLowerBuffA(sz1, strlen(sz1));
        deb("  %s", sz1);
        hInst = hinstDLL;

        dwShellId = (DWORD)CreateThread(NULL, 0, processMessagesPipe, NULL, 0, &dwMsgsId);
        InstallTrayHook();

        // CreateThread(NULL, 0, processMessages, NULL, 0, &dwMsgsId);
        // CreateThread(NULL, 0, processMouseMessagesPipe, NULL, 0, &dwMsgsId);
        // Sleep(600);
        // InstallMouseHook();

        srand(dwShellId + (rand() % 1 ? dwMsgsId:0));

        InitializeCriticalSectionAndSpinCount(&modifyCallbacks, 0x80000400);
        InitializeCriticalSection(&modifyIcons);
        DisableThreadLibraryCalls(hinstDLL);
        break;

        case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        deb("DLL_THREAD_ATTACH");

        GetModuleFileNameExA(GetCurrentProcess(), NULL, sz1, sizeof(sz1));
        CharLowerBuffA(sz1, strlen(sz1));
        deb("  %s", sz1);
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

extern "C" long __stdcall mye_TrayOnLeftDblClick(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_leftdblclick.begin();it != events_leftdblclick.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_leftdblclick.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_leftdblclick.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_leftdblclick.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long mye_TrayOnRightDblClick(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_rightdblclick.begin();it != events_rightdblclick.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_rightdblclick.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_rightdblclick.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_rightdblclick.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnRightButtonDown(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_rightbuttondown.begin();it != events_rightbuttondown.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_rightbuttondown.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_rightbuttondown.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_rightbuttondown.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnRightButtonUp(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_rightbuttonup.begin();it != events_rightbuttonup.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_rightbuttonup.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_rightbuttonup.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_rightbuttonup.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnMouseMove(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_mousemove.begin();it != events_mousemove.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_mousemove.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_mousemove.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_mousemove.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnLeftButtonDown(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_leftbuttondown.begin();it != events_leftbuttondown.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_leftbuttondown.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_leftbuttondown.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_leftbuttondown.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnLeftButtonUp(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_leftbuttonup.begin();it != events_leftbuttonup.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_leftbuttonup.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_leftbuttonup.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_leftbuttonup.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnBalloonShown(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_balloonshown.begin();it != events_balloonshown.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_balloonshown.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_balloonshown.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_balloonshown.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnBalloonTimeoutClose(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_balloontimeoutclose.begin();it != events_balloontimeoutclose.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_balloontimeoutclose.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_balloontimeoutclose.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_balloontimeoutclose.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnBalloonClick(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_balloonclick.begin();it != events_balloonclick.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_balloonclick.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_balloonclick.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_balloonclick.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnPopupOpen(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_popupopen.begin();it != events_popupopen.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_popupopen.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_popupopen.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_popupopen.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_TrayOnPopupClose(LPVOID addr)
{
    unsigned long id = 0;
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 1000)
    {
        deb("removing events_v id %d", addr);
        unsigned pos = 0;
        for (events_v::iterator it = events_popupclose.begin();it != events_popupclose.end();it++)
        {
            deb("checking event pos %d addr %x", pos, (*it));
            if (pos > events_popupclose.size() + 1)
            {
                deb("error");
                break;
            }
            if ((unsigned long)addr == (*it).id)
            {
                events_popupclose.erase(it);
                break;
            }
            pos++;
        }
    }
    else
    {
        EVENTS e;

        e.addr = addr;
        id = rand()%1000;
        e.id = id;
        events_popupclose.push_back(e);
    }

    // LeaveCriticalSection(&modifyCallbacks);

    return id;
}

extern "C" long __stdcall mye_EnumerateIcons(DWORD dwIndex, PNOTIFYICONDATAW pn)
{
    // deb("mye_EnumerateIcons: %d %x icons.size: %d", dwIndex, pn, icons.size());

    if (dwIndex < icons.size())
    {
        // deb("memcpy(%08x,%08x,%d)", pn, &icons[dwIndex]->pn, sizeof(NOTIFYICONDATAW));
        // deb("copying pn.hicon: 0x%08x hIcon: 0x%08x pn.hWnd: 0x%08x hwnd: 0x%08x", icons[dwIndex]->pn.hIcon,
        // icons[dwIndex]->hicon, icons[dwIndex]->pn.hWnd, icons[dwIndex]->hwnd);
        memcpy((void*)pn, (void*) &icons[dwIndex]->pn, sizeof(NOTIFYICONDATAW));
        return MYE_SUCCESS;
    }

    return MYE_NO_MORE_ITEMS;
}

void IconProcess(DWORD cmd, PNOTIFYICONDATAW pn)
{
    bool found = false;

    if (!pn)
        deb("pn zero");

    EnterCriticalSection(&modifyIcons);
    for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
    {
        // deb("it: %x", (*it));
        if ((*it)->uID == pn->uID && (*it)->hwnd == pn->hWnd)
        {
            found = true;
            char temp[255];

            switch(cmd)
            {
                case NIM_ADD:
                deb("already will be in icons");

                break;

                case NIM_MODIFY:
                // deb("no change");
                (*it)->setPn(pn);

                if (pn->uFlags & NIF_TIP)
                {
                    deunicode((wchar_t*)pn->szTip, (char*)temp, sizeof(temp));
                    (*it)->setTip(temp);
                }
                if ((pn->uFlags & NIF_ICON) && pn->hIcon)
                {
                    // HICON temp=(*it)->hicon;
                    (*it)->setHicon(pn->hIcon);
                    // deb("    hicon: %08x ====> hIcon: %08x", (*it)->pn.hIcon, (*it)->hicon);
                    (*it)->pn.hIcon = (*it)->hicon;

                }
                else
                {
                    (*it)->pn.hIcon = (*it)->hicon;

                }
                (*it)->hwnd = pn->hWnd;
                (*it)->uID = pn->uID;
                (*it)->uFlags = pn->uFlags;
                (*it)->uCallbackMessage = pn->uCallbackMessage;
                // memcpy((void*) &(*it)->pn, pn, sizeof(NOTIFYICONDATAW));

                break;

                case NIM_DELETE:
                deb("deleting from icons %x", (*it));
                // delete(*it);
                icons.erase(it);
                // delete(*it);
                goto _icon_exit;
            }
            if (cmd!=NIM_DELETE)
                (*it)->pn.guidItem.Data1 = (*it)->calls++;
        }
    }

    if (!found && cmd != NIM_DELETE)
    {
        nIcon *ico = new nIcon();
        UINT id = (unsigned)pn->hWnd + pn->uID;

        DWORD dwPid;
        DWORD thId = GetWindowThreadProcessId(pn->hWnd, &dwPid);

        char szFN[MAX_PATH];

        // if(!GetWindowModuleFileNameA(pn->hWnd, szFN, sizeof(szFN)))
        // deb("GetWindowModuleFileName(%x): %s", pn->hWnd, fmterr());
        HANDLE hprc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, false, dwPid);
        if (!hprc)
            deb("openprocess: %s", fmterr());
        GetModuleFileNameEx(hprc, NULL, szFN, sizeof(szFN));

        deb("adding new icon #%-2d hwnd: %x\r\n  %s\r\n  hicon: %08x @ %x id %X", icons.size()+1, pn->hWnd,
            strlwr(szFN), pn->hIcon, ico, id);

        ico->setId(id);

        char temp[255];
        deunicode((wchar_t*)pn->szTip, (char*)temp, sizeof(temp));
        ico->setTip(temp);
        ico->id = id;
        if (pn->uFlags & NIF_ICON)
        {
            ico->setHicon(pn->hIcon);
            // deb("new setHicon(%x) = %x",pn->hIcon, ico->hicon);
        }
        ico->hwnd = pn->hWnd;
        ico->uID = pn->uID;
        ico->uFlags = pn->uFlags;
        ico->uCallbackMessage = pn->uCallbackMessage;
        ico->setPn(pn);
        if (ico->hicon)
            ico->pn.hIcon = ico->hicon;

        icons.push_back(ico);
    }
_icon_exit:
    LeaveCriticalSection(&modifyIcons);
    // __except (exceptionHandler(GetExceptionCode(), GetExceptionInformation()))
    // {
    // deb("exception while iconprocess cmd: %x", cmd);

    // }
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
        snprintf(szTemp, sizeof(szTemp), "  %03d 0x%08X :%-25s", i, addr, si->Name);

        // get sym module line number
        IMAGEHLP_LINE64 il;

        memset(&il, 0, sizeof(il));
        il.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        il.Address = (unsigned __int64)addr;
        ret = SymGetLineFromAddr64(GetCurrentProcess(), (unsigned __int64)addr, &dwDisp, &il);

        char szTemp2[128];

        memset(szTemp2, 0, sizeof(szTemp2));

        if (il.LineNumber)
            snprintf(szTemp2, sizeof(szTemp2), "  %s:%d (0x%08X)", il.FileName, il.LineNumber, il.Address);

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InstallTrayHook(void)
{
    HWND shellwnd = FindWindow("Shell_TrayWnd", NULL);
    // deb("shellwnd: %x", shellwnd);

    if (!hLib)
        hLib = LoadLibrary("hookex.dll");
    deb("hookex.dll: %x", hLib);
    p_CallWndProc = (CallWndProc)GetProcAddress(hLib, "CallWndProc");
    deb("  p_CallWndProc @ 0x%08x", p_CallWndProc);

    DWORD procId;
    DWORD thId = GetWindowThreadProcessId(shellwnd, &procId);
    deb("  procId: %x thId: %x", procId, thId);

    trayhook = SetWindowsHookEx(WH_CALLWNDPROC, p_CallWndProc, hLib, thId);
    if (trayhook)
        deb("  trayhook: %x", trayhook);
    else
        deb("SetWindowsHookEx: %s", fmterr());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InstallMouseHook(void)
{

    if (!hLib)
        hLib = LoadLibrary("hookex.dll");
    p_MouseWndProc = (CallWndProc)GetProcAddress(hLib, "MouseWndProc");
    deb("MouseWndProc @ 0x%08x", p_MouseWndProc);

    mousehook = SetWindowsHookEx(WH_CALLWNDPROC, p_MouseWndProc, hLib, 0);
    if (mousehook)
        deb("mousehook: %x", mousehook);
    else
        deb("SetWindowsHookEx mousehook: %s", fmterr());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FreeHooks(void)
{
    UnhookWindowsHookEx(mousehook);
    UnhookWindowsHookEx(trayhook);
}

LRESULT CALLBACK wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int xPos;
    int yPos;
    int pos = 0;
    bool fBlocked;
    CWPSTRUCT cs;
    PCWPSTRUCT ps;
    char str[128];
    // deb("HWND 0x%x uMsg 0x%08x wParam 0x%x lParam 0x%x", hwnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
        case WM_COPYDATA:

        cd = (COPYDATASTRUCT*)lParam;
        if (cd->dwData == 1)
        {
            pstd = (PSHELLTRAYDATA)cd->lpData;

            // dump((char*) pstd, cd->cbData, "pstd");

            deb("mye.dll: sizeof of nid : %d", cd->cbData);

        }
        else
        {
            deb("mye.dll: cd->dwData: %d", cd->dwData);
        }

        break;

        // /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // case WM_TRAY_DATA:
        //
        // memcpy((void*) &cs, (void*)lParam, sizeof(cs));
        //
        // if (InSendMessage())
        // ReplyMessage(false);
        //
        // deb("wndproc: hwnd: 0x%08x uMsg: %d (0x%08x) wParam 0x%08x lParam 0x%08x", hwnd, uMsg, uMsg, wParam, lParam);
        // fBlocked = (InSendMessageEx(NULL) & (ISMEX_REPLIED|ISMEX_SEND)) == ISMEX_SEND;
        // deb("thread is blocked: %d", fBlocked);
        //
        // // ps = (PCWPSTRUCT)lParam;
        // // if (IsBadReadPtr((void*) ps, sizeof(CWPSTRUCT)))
        // // {
        // // deb("bad ptr 0x%08x", ps);
        // // break;
        // // }
        //
        // for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
        // {
        // if ((*it)->hwnd == cs.hwnd)
        // deb("message for icon %d (%s)", pos, (*it)->tip);
        // pos++;
        // }
        // // GlobalFree((void*)lParam);
        //
        // break;
        // /////////////////////////////////////////////////////////////////////////////////////////////////////////

        case WM_DESTROY:
        PostQuitMessage(WM_QUIT);
        break;

        break;

        default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI processMouseMessagesPipe(LPVOID p)
{
    HANDLE hPipe;
    unsigned long msgid = 0;
    DWORD dwRead;
    WNDPROCSTRUCT wps;
    CWPSTRUCT cs;

    deb("processMouseMessagesPipe: %x", GetCurrentThreadId());

    hPipe = CreateNamedPipe("\\\\.\\pipe\\wndmsgs", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
        50, 50, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        deb("CreateNamedPipe: %s", fmterr());
        ExitThread(0);
    }

    while (1)
    {
        // deb("connecting pipe ...");
        // bool ret=WaitNamedPipeA("\\\\.\\pipe\\wndmsgs", 300);
        // if (!ret)
        // {
        // Sleep(100);
        // continue;
        // }
        bool ret = ConnectNamedPipe(hPipe, NULL);
        if (!ret)
        {
            deb("ConnectNamedPipe: %s", fmterr());
            Sleep(500);
            continue;
        }

        // deb("connected");

        ReadFile(hPipe, &cs, sizeof(cs), &dwRead, NULL);

        WriteFile(hPipe, NULL, 0, &dwRead, NULL);
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);

        try
        {

            int pos = 0;
            for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
            {
                if ((*it)->hwnd == cs.hwnd)
                {
                    deb("message %x for icon %d (%s)", LOWORD(cs.lParam), pos, (*it)->tip);
                    // deb("hwnd: %x wparam: %x lparam: %x message: 0x%04x", cs.hwnd, cs.wParam, cs.lParam,
                    // cs.message);
                    // deb("loword(lparam): %x HIWORD(lparam): %x", LOWORD(cs.lParam), HIWORD(cs.lParam));
                    // deb("GET_X_LPARAM(wParam): %x GET_Y_LPARAM(wParam): %x", GET_X_LPARAM(cs.wParam),
                    // GET_Y_LPARAM(cs.wParam));
                    int msg = LOWORD(cs.lParam);

                    switch(msg)
                    {
                        case WM_LBUTTONDBLCLK:
                        deb("left dbl click");
                        for (events_v::iterator it = events_leftdblclick.begin();it!=events_leftdblclick.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_RBUTTONDBLCLK:
                        deb("right dbl click");
                        for (events_v::iterator it = events_rightdblclick.begin();it!=events_rightdblclick.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_RBUTTONDOWN:
                        deb("right button down");
                        for (events_v::iterator it = events_rightbuttondown.begin();it!=events_rightbuttondown.end();
                            it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_RBUTTONUP:
                        deb("right button up");
                        for (events_v::iterator it = events_rightbuttonup.begin();it!=events_rightbuttonup.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_MOUSEMOVE:
                        deb("mouse move");
                        for (events_v::iterator it = events_mousemove.begin();it!=events_mousemove.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_LBUTTONDOWN:
                        deb("left button down");
                        for (events_v::iterator it = events_leftbuttondown.begin();it!=events_leftbuttondown.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case WM_LBUTTONUP:
                        deb("left button up");
                        for (events_v::iterator it = events_leftbuttonup.begin();it!=events_leftbuttonup.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case NIN_BALLOONSHOW:
                        deb("balloon shown");
                        for (events_v::iterator it = events_balloonshown.begin();it!=events_balloonshown.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        case NIN_BALLOONTIMEOUT:
                        deb("balloon timeout");
                        for (events_v::iterator it = events_balloontimeoutclose.begin();
                            it!=events_balloontimeoutclose.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;

                        case NIN_BALLOONUSERCLICK:
                        deb("balloon user click");
                        for (events_v::iterator it = events_balloonclick.begin();it!=events_balloonclick.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;

                        case NIN_POPUPOPEN:
                        deb("popupmenu open");
                        for (events_v::iterator it = events_popupopen.begin();it!=events_popupopen.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;

                        case NIN_POPUPCLOSE:
                        deb("popupmenu close");
                        for (events_v::iterator it = events_popupclose.begin();it!=events_popupclose.end();it++)
                        {
                            shell_events_api pfunc = (shell_events_api)(*it).addr;
                            pfunc(pos, &icons[pos]->pn);
                        }
                        break;
                        default:
                        if (msg <= WM_USER)
                            deb("unknown msg %x ", msg);
                        break;
                    }

                }
                pos++;
            }
        }
        catch(...)
        {
            deb("exception while processing mouse message pipe");
        }

        // WriteFile(hPipe, &wps, dwRead, &dwRead, NULL);

        // FlushFileBuffers(hPipe);

        // deb("pipe disconnected");
    }
}

DWORD GetIconCallNumById(int hwnd, int uid)
{
    for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
    {
        if ((*it)->pn.hWnd == (HWND)hwnd && (*it)->pn.uID == uid)
            return(*it)->pn.guidItem.Data1;
    }
}

DWORD WINAPI processMessagesPipe(LPVOID p)
{
    HANDLE hPipe;
    unsigned long msgid = 0;

    deb("processMessagesPipe: %x", GetCurrentThreadId());

    hPipe = CreateNamedPipe("\\\\.\\pipe\\traymsgs", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
        1000, 1000, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        deb("CreateNamedPipe: %s", fmterr());
        ExitThread(0);
    }

    while (1)
    {
        bool ret = ConnectNamedPipe(hPipe, NULL);
        if (!ret)
        {
            deb("ConnectNamedPipe: %s", fmterr());
            Sleep(500);
            continue;
        }

        hCbkLoop = CreateMutex(NULL, true, "Global\\hCbkLoop");

        char buffer[1000];
        DWORD dwRead;

        ReadFile(hPipe, buffer, sizeof(buffer), &dwRead, NULL);
        // deb("packet %d ----------------------------------------------------------------", msgid++);

        PSHELLTRAYDATA pstd;

        pstd = (PSHELLTRAYDATA)buffer;

        // deb("sizef notify : %d", dwRead);
        NOTIFYICONDATAW *pn;

        // EnterCriticalSection(&modifyCallbacks);
        switch(pstd->dwMessage)
        {
            case NIM_ADD:

            pn = (NOTIFYICONDATAW*) & pstd->nid;
            deb("NIM_ADD %08x:%-2x #%d", pn->hWnd, pn->uID, GetIconCallNumById((int)pn->hWnd, pn->uID));
            char sz1[1024];

            // dump((char*)pn, pn->cbSize, "nim_add");
            // deb("size: %d tip: %s (A:%s)", pn->cbSize, deunicode(pn->szTip, sz1, sizeof(sz1)), pn->szTip);

            IconProcess(pstd->dwMessage, pn);

            for (callbacks_v::iterator it = add_callbacks.begin();it != add_callbacks.end();it++)
            {
                shell_callback_api pfunc;

                pfunc = (shell_callback_api)(*it);
                // deb("pfunc @ 0x%08X", pfunc);
                if (IsBadCodePtr((FARPROC)pfunc))
                {
                    deb("callback removed while processing it");
                    continue;
                }
                if (pfunc)
                    pfunc(pn, sizeof(NOTIFYICONDATAW));
            }

            /* провер€ем на баллон.  ≈ЋЋбечим всех */
            if (pn->uFlags & NIF_INFO)
            {
                deb("ballon msg %s", deunicode(pn->szInfoTitle, sz1, sizeof(sz1)));
                for (callbacks_v::iterator it = bln_callbacks.begin();it != bln_callbacks.end();it++)
                {
                    shell_callback_api pfunc;

                    pfunc = (shell_callback_api)(*it);
                    // deb("pfunc @ 0x%08X", pfunc);
                    if (IsBadCodePtr((FARPROC)pfunc))
                    {
                        deb("callback removed while processing it");
                        continue;
                    }
                    if (pfunc)
                        pfunc(pn, sizeof(NOTIFYICONDATAW));
                }
            }

            break;
            case NIM_MODIFY:
            pn = (NOTIFYICONDATAW*) & pstd->nid;
            deb("NIM_MODIFY %08x:%-2x #%d", pn->hWnd, pn->uID, GetIconCallNumById((int)pn->hWnd, pn->uID));
            // dump((char*)pn, pn->cbSize, "NIM_MODIFY");

            IconProcess(pstd->dwMessage, pn);

            for (callbacks_v::iterator it = mod_callbacks.begin();it != mod_callbacks.end();it++)
            {
                shell_callback_api pfunc;

                pfunc = (shell_callback_api)(*it);
                // deb("pfunc @ 0x%08X", pfunc);
                if (IsBadCodePtr((FARPROC)pfunc))
                {
                    deb("callback removed while processing it");
                    continue;
                }
                if (pfunc)
                    pfunc(pn, sizeof(NOTIFYICONDATAW));
            }

            /* провер€ем на баллон.  ≈ЋЋбечим всех */
            if (pn->uFlags & NIF_INFO)
            {
                deb("ballon msg %s", deunicode(pn->szInfoTitle, sz1, sizeof(sz1)));
                int idx = 0;
                for (events_v::iterator it = events_balloonshown.begin();it != events_balloonshown.end();it++)
                {
                    shell_callback_api pfunc;

                    pfunc = (shell_callback_api)(*it).addr;
                    // deb("pfunc @ 0x%08X", pfunc);
                    if (IsBadCodePtr((FARPROC)pfunc))
                    {
                        deb("callback removed while processing it");
                        continue;
                    }
                    if (pfunc)
                        pfunc(0, (unsigned long)pn);
                }
            }

            break;
            case NIM_DELETE:

            pn = (NOTIFYICONDATAW*) & pstd->nid;
            deb("NIM_DELETE %x:%x", pn->hWnd, pn->uID);
            IconProcess(pstd->dwMessage, pn);

            for (callbacks_v::iterator it = del_callbacks.begin();it != del_callbacks.end();it++)
            {
                shell_callback_api pfunc;

                pfunc = (shell_callback_api)(*it);
                // deb("pfunc @ 0x%08X", pfunc);
                if (IsBadCodePtr((FARPROC)pfunc))
                {
                    deb("callback removed while processing it");
                    continue;
                }
                if (pfunc)
                    pfunc(pn, sizeof(NOTIFYICONDATAW));
            }

            break;
            default:
            deb("unknown nim %d (0x%x)", pstd->dwMessage, pstd->dwMessage);
            break;
        }

        // LeaveCriticalSection(&modifyCallbacks);

        WriteFile(hPipe, buffer, dwRead, &dwRead, NULL);

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);

        ReleaseMutex(hCbkLoop);
        CloseHandle(hCbkLoop);
        // CloseHandle(hPipe);

    }
}

DWORD WINAPI processMessages(LPVOID p)
{

    WNDCLASS wndclass;

    memset(&wndclass, 0x0, sizeof(wndclass));

    wndclass.style = 0;
    wndclass.lpfnWndProc = wndproc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInst;
    wndclass.hIcon = NULL;
    wndclass.hCursor = NULL;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = "mye_Shell_TrayWnd";

    ATOM awnd = RegisterClass(&wndclass);
    if (!awnd)
        deb("failed to register window class: %s", fmterr());

    HWND hwnd = CreateWindow("mye_Shell_TrayWnd", "mye System tray", 0, 10, 10,
        // WS_VISIBLE
        400, 100, NULL, NULL, (HINSTANCE)hInst, NULL);

    ShowWindow(hwnd, SW_HIDE);

    if (!hwnd)
        deb("failed to create initial window %s", fmterr());
    else
        deb("created hwnd %x for messages", hwnd);

    MSG msg;
    BOOL bRet;
    unsigned long msgid = 0;
    deb("waiting for msgs for %x", hwnd);

    while ((bRet = GetMessage(&msg, hwnd, 0, 0)) != 0)
    {
        // deb("msgid: %u", msgid++);
        if (bRet == -1)
        {
            deb("getmessage: %s", fmterr());
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    }
    deb("exit getMessages");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI processShellCallbacks(LPVOID hinstDLL)
{
    static HKEY shellKey = NULL;

    deb("processShellCallbacks %x", GetCurrentThreadId());

    while (1)
    {

        DWORD dwIndex;
        char keyname[128];
        long ret;

        if (!shellKey)
        {
            ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\myexplorer\\shelltray", 0, KEY_ALL_ACCESS, &shellKey);
            if (ret == 2)
            {
                // deb("no shelltray key");
                Sleep(10);
                continue;
            }
            if (ret != ERROR_SUCCESS)
            {
                deb("mye.cpp: RegOpenKeyEx shelltray: %s", fmterr(ret));
                continue;
            }
        }
        dwIndex = 0;

        while ((ret = RegEnumKey(shellKey, dwIndex, keyname, sizeof(keyname))) == ERROR_SUCCESS)
        {
            // deb("processShellCallbacks dwIndex: %d keyname %s", dwIndex, keyname);
            HKEY keyHkey;

            ret = RegOpenKey(shellKey, keyname, &keyHkey);
            if (ret != ERROR_SUCCESS)
            {
                deb("RegOpenKey ret for %s: %s", keyname, fmterr(ret));
                dwIndex++;
                continue;
            }

            DWORD status;
            DWORD datasize = sizeof(status);
            ret = RegQueryValueEx(keyHkey, "status", 0, 0, (unsigned char*) & status, &datasize);
            if (ret != ERROR_SUCCESS)
            {
                // deb("mye.cpp: RegQueryValueEx('data') ret for %s: %s", keyname, fmterr(ret));
                RegCloseKey(keyHkey);
                dwIndex++;
                continue;
            }

            if (status == 1)
            {
                deb("already processed packet %s", keyname);
                dwIndex++;
                RegCloseKey(keyHkey);
                continue;
            }

            unsigned char data[1000] = "123";
            datasize = sizeof(data);
            ret = RegQueryValueEx(keyHkey, "data", 0, 0, data, &datasize);
            if (ret != ERROR_SUCCESS)
            {
                // deb("mye.cpp: RegQueryValueEx('data') ret for %s: %s", keyname, fmterr(ret));
                RegCloseKey(keyHkey);
                dwIndex++;
                continue;
            }

            DWORD size;
            bool modified = false;
            for (callbacks_v::iterator it = shell_callbacks.begin();it != shell_callbacks.end();it++)
            {

                char strcbkid[128];
                sprintf(strcbkid, "cbk:%x", (*it));
                size = sizeof(strcbkid);
                ret = RegQueryValueEx(keyHkey, strcbkid, 0, 0, strcbkid, &size);
                if (ret == ERROR_SUCCESS)
                {
                    // deb("%s already processed by cbk %x", keyname, (*it));
                    continue;
                }
                deb("processing callback %x for packed %s", (*it), keyname);

                modified = true;
                shell_callback_api p_shell_callback_api = (shell_callback_api)(*it);

                if (IsBadCodePtr((FARPROC)p_shell_callback_api))
                {
                    deb("callback removed while processing callbacks");
                    break;
                }
                ret = p_shell_callback_api(data, datasize);

                char str[32];
                sprintf(str, "cbk:%x", (*it));

                ret = RegSetValueEx(keyHkey, str, 0, REG_SZ, keyname, strlen(keyname));
                if (ret != ERROR_SUCCESS)
                {
                    deb("mye.cpp: RegSetValueEx('%s') ret for %s: %s", str, keyname, fmterr(ret));
                    continue;
                }
            }

            ret = RegSetValueEx(keyHkey, "data", 0, REG_BINARY, data, datasize);
            if (ret != ERROR_SUCCESS)
            {
                deb("mye.cpp: RegSetValueEx('data') ret for %s: %s", keyname, fmterr(ret));
            }

            DWORD dw = 1;
            ret = RegSetValueEx(keyHkey, "status", 0, REG_DWORD, (unsigned char*) & dw, sizeof(DWORD));
            if (ret != ERROR_SUCCESS)
            {
                deb("mye.cpp: RegSetValueEx('status') ret for %s: %s", keyname, fmterr(ret));
            }

            RegCloseKey(keyHkey);
            dwIndex++;
        }
        // deb("processShellCallbacks waiting for changes");

        static HANDLE hEvent = NULL;
        if (!hEvent)
            hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        ret = RegNotifyChangeKeyValue(shellKey, true, REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME, hEvent,
            true);
        if (ret != ERROR_SUCCESS)
            deb("mye.cpp: RegNotifyChangeKeyValue: %s", fmterr(ret));

        if (WaitForSingleObject(hEvent, 300) == WAIT_FAILED)
        {
            deb("Error in WaitForSingleObject: %s", fmterr());
        }
        ResetEvent(hEvent);
        // RegCloseKey(shellKey);
    }
}

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
    if ((size_t)dwIndex > icons.size())
        return false;
    PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, WM_RBUTTONDOWN);
    return PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, WM_RBUTTONUP);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" long __stdcall mye_ClickIcon(BYTE dwIndex)
{
    if ((size_t)dwIndex > icons.size())
        return false;

    deb("sending click to hwnd: %x callback: %x uid: %d", icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage,
        icons[dwIndex]->uID);
    PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, BM_CLICK);
    PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, WM_LBUTTONDOWN);
    return PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, WM_LBUTTONUP);

}

extern "C" long __stdcall mye_DblClickIcon(BYTE dwIndex)
{
    if ((size_t)dwIndex > icons.size())
        return false;

    return PostMessage(icons[dwIndex]->hwnd, icons[dwIndex]->uCallbackMessage, icons[dwIndex]->uID, WM_LBUTTONDBLCLK);

}

extern "C" long __stdcall mye_TrayAddCallback(LPVOID addr)
{
    _wait_for_hCbkLoop();
    // deb("add callback entering critical section. addr %x", addr);
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 100)
    {
        deb("removing add callback id %d", addr);
        unsigned pos = 0;
        for (callbacks_v::iterator it = add_callbacks.begin();it != add_callbacks.end();it++)
        {
            deb("checking callback pos %d addr %x", pos, (*it));
            if (pos > add_callbacks.size() + 1)
            {
                deb("error");
                break;
            }
            if (pos++ == (unsigned)addr)
            {
                add_callbacks.erase(it);
                break;
            }
        }
    }
    else
        add_callbacks.push_back(addr);

    // deb("mye add callback addr %x leave critical section", addr);

    // LeaveCriticalSection(&modifyCallbacks);

    return add_callbacks.size() - 1;
}

extern "C" long __stdcall mye_TrayModifyCallback(LPVOID addr)
{
    _wait_for_hCbkLoop();
    // deb("modify callback entering critical section. addr %x", addr);
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 100)
    {
        deb("removing mod callback id %d", addr);
        unsigned pos = 0;
        for (callbacks_v::iterator it = mod_callbacks.begin();it != mod_callbacks.end();it++)
        {
            if (pos++ == (unsigned)addr)
            {
                mod_callbacks.erase(it);
                break;
            }
        }
    }
    else
        mod_callbacks.push_back(addr);
    // LeaveCriticalSection(&modifyCallbacks);

    return mod_callbacks.size() - 1;
}

extern "C" long __stdcall mye_TrayDeleteCallback(LPVOID addr)
{
    _wait_for_hCbkLoop();
    // deb("delete callback entering critical section. addr %x", addr);
    // EnterCriticalSection(&modifyCallbacks);
    if ((unsigned)addr < 100)
    {
        deb("removing del callback id %d", addr);
        unsigned pos = 0;
        for (callbacks_v::iterator it = del_callbacks.begin();it != del_callbacks.end();it++)
        {
            if (pos++ == (unsigned)addr)
            {
                del_callbacks.erase(it);
                break;
            }
        }
    }
    else
        del_callbacks.push_back(addr);
    // LeaveCriticalSection(&modifyCallbacks);

    return del_callbacks.size() - 1;
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

extern "C" int __stdcall mye_DockWindow(HWND hwnd, int xwidth, int where)
{
    return TRUE;
}

extern "C" int __stdcall InstallService(void)
{

}
