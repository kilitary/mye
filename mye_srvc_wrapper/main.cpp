#include <vcl.h>
#pragma hdrstop

#include "main.h"
#include "functions.h"
#include "nIcon.h"
#include <Psapi.h>
// #include <windows.h>
#include <vector>
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

PSHELLTRAYDATA pstd;
COPYDATASTRUCT *cd;
HINSTANCE hInst;
HHOOK trayhook;
HHOOK mousehook;
TCriticalSection *is;
MouseWndProc p_MouseWndProc;
icon_v icons;
event_v events;
hwnd_v windows;
TForm3 *Form3;
bool mousedown = false;
HMODULE hLib;

__fastcall TForm3::~TForm3(void)
{
    deb("srvc unload");
    UnhookWindowsHookEx(trayhook);
    UnhookWindowsHookEx(mousehook);

    FreeLibrary(hLib);
}

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

void InstallTrayHook(void)
{
    CallWndProc p_CallWndProc;

    HWND shellwnd = NULL;
    int i = 0;
    while (!(shellwnd = FindWindow("Shell_TrayWnd", NULL)))
    {
        deb("waiting for Shell_TrayWnd window ... %2d", i++);
        Application->ProcessMessages();
        Sleep(300);
    }
    deb("shellwnd: %x", shellwnd);

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

DWORD WINAPI processWndMessages(LPVOID p)
{
    static HANDLE hPipe = NULL;
    unsigned long msgid = 0;
    DWORD dwRead;
    WNDPROCSTRUCT wps;
    CWPSTRUCT cs;

    if (hPipe)
    {
        deb("processWndMessages again call! pipe: %x", hPipe);
        ExitThread(0);
    }
    deb("processWndMessages: %x", GetCurrentThreadId());

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    SetSecurityDescriptorGroup(&sd, NULL, FALSE);
    SetSecurityDescriptorSacl(&sd, FALSE, NULL, FALSE);

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = false;

    hPipe = CreateNamedPipe("\\\\.\\pipe\\wndmsgs", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
        1550, 1550, 3400, &sa);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        deb("processWndMessages CreateNamedPipe: %s", fmterr());
        ExitThread(0);
    }

    while (1)
    {

        try
        {
            bool ret = ConnectNamedPipe(hPipe, NULL);
            if (!ret)
            {
                DWORD err = GetLastError();
                deb("processWndMessages ConnectNamedPipe: %s", fmterr(err));
                if (err != 535)
                {
                    // Sleep(500);
                    continue;
                }
            }

            // deb("connected");

            ReadFile(hPipe, &cs, sizeof(cs), &dwRead, NULL);

            DWORD dwWrite = 0;
            WriteFile(hPipe, NULL, 0, &dwWrite, NULL);
            FlushFileBuffers(hPipe);
            DisconnectNamedPipe(hPipe);

            int pos = 0;
            // deb("wndmsg hwnd: %08x msg: %04x read %d", cs.hwnd, cs.message, dwRead);
            // for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
            is->Acquire();
            for (int i = 0;i<icons.size();i++)
            {
                // deb("checking it #%d %x", pos, icons[i]);
                if (IsBadReadPtr(icons[i], 1))
                {
                    MessageBox(Application->MainFormHandle, "err bad read ptr icons_v", "err", MB_OK);
                    ExitProcess(0);
                }

                if (icons[i]->hwnd == cs.hwnd && icons[i]->uID == cs.wParam && icons[i]->uCallbackMessage == cs.message)
                {
                    // deb("message %x for icon %d (%s)", LOWORD(cs.lParam), pos, icons[i]->tip);
                    // deb("hwnd: %x wparam: %x lparam: %x message: 0x%04x", cs.hwnd, cs.wParam, cs.lParam,
                    // cs.message);
                    // deb("loword(lparam): %x HIWORD(lparam): %x", LOWORD(cs.lParam), HIWORD(cs.lParam));
                    // deb("GET_X_LPARAM(wParam): %x GET_Y_LPARAM(wParam): %x", GET_X_LPARAM(cs.wParam),
                    // GET_Y_LPARAM(cs.wParam));
                    int msg = LOWORD(cs.lParam);

                    for (event_v::iterator it = events.begin();it!=events.end();it++)
                    {
                        if ((*it).msg == msg)
                        {

                            DWORD dwRead;
                            unsigned long ret2 = CallNamedPipeA((*it).dllpipe, &(*it), sizeof(EVENT), NULL, 0, &dwRead,
                                0);
                            // if (!ret2)
                            // deb("processWndMessages/events CallNamedPipe: %s", fmterr());
                        }
                    }

                    switch(msg)
                    {
                        case WM_LBUTTONDBLCLK:
                        deb("left dbl click");

                        break;
                        case WM_RBUTTONDBLCLK:
                        deb("right dbl click");

                        break;
                        case WM_RBUTTONDOWN:
                        deb("right button down");

                        break;
                        case WM_RBUTTONUP:
                        deb("right button up");

                        break;
                        case WM_MOUSEMOVE:
                        deb("mouse move");

                        break;
                        case WM_LBUTTONDOWN:
                        deb("left button down");

                        break;
                        case WM_LBUTTONUP:
                        deb("left button up");

                        break;
                        case NIN_BALLOONSHOW:
                        deb("balloon shown");

                        break;
                        case NIN_BALLOONTIMEOUT:
                        deb("balloon timeout");

                        break;

                        case NIN_BALLOONUSERCLICK:
                        deb("balloon user click");

                        break;

                        case NIN_POPUPOPEN:
                        deb("popupmenu open");

                        break;

                        case NIN_POPUPCLOSE:
                        deb("popupmenu close");

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
        __finally
        {
            is->Release();
        }
        deb("done wndmesg");
    }
}

DWORD WINAPI cleanupThread(LPVOID p)
{
    while (1)
    {
        Sleep(200);

        try
        {
            for (event_v::iterator it = events.begin();it!=events.end();it++)
            {
                DWORD dwRead;
                unsigned long ret2 = CallNamedPipeA((*it).dllpipe, &(*it), 1, NULL, 0, &dwRead, 0);
                if (!ret2)
                {
                    int err = GetLastError();
                    if (err == 2)
                    {
                        // deb("erased stalled event id: %x addr: %x msg: %x pipe: %s", (*it).id, (*it).addr, (*it).msg,
                        // (*it).dllpipe);

                        it = events.erase(it);

                        if (events.end() == it)
                            break;
                    }
                }

            }

            is->Acquire();
            for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
            {
                HWND hwnd = (*it)->hwnd;
                DWORD prId, trId;
                trId = GetWindowThreadProcessId(hwnd, &prId);
                if (!trId || !prId)
                {
                    deb("erasing stalled icon hwnd %x [%s]", hwnd, (*it)->tip);
                   // MessageBox(Application->MainFormHandle, "erased stall icon", "erased icpon", MB_OK);
                    it = icons.erase(it);
                    if (it == icons.end())
                        break;
                }
            }
            is->Release();
        }
        catch(...)
        {
            deb("exception while erasing events");
        }
    }
}

BOOL CALLBACK enumWindowsFunc(HWND hwnd, LPARAM lParam)
{
    bool found = false;
    char str[128];
    GetWindowText(hwnd, str, sizeof(str));
    // deb("hwnd %x (%s)",hwnd,str);

    int style = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (!IsWindowVisible(hwnd) || !str[0] || style & WS_EX_TOOLWINDOW) // || !(style & WS_EX_APPWINDOW)
        return true;

    // deb("visible: %x (%s)",hwnd,str);
    for (hwnd_v::iterator it = windows.begin();it!=windows.end();it++)
    {
        if ((*it) == hwnd)
        {
            found = true;
            continue;
        }
        if (!IsWindowVisible((*it)))
        {
            it = windows.erase(it);
            found = true;
            if (it == windows.end())
                break;
        }
    }
    if (!found)
    {
        windows.push_back(hwnd);
    }
    return true;
}

DWORD WINAPI enumerateWindows(LPVOID p)
{
    while (1)
    {
        Sleep(100);
        // windows.clear();
        EnumWindows(enumWindowsFunc, NULL);
    }
}

// ---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent *Owner):TForm(Owner)
{
    int dwShellId;
    DWORD dwMsgsId;
    static int form_created = 0;

    if (form_created++)
    {
        MessageBox(NULL, "SHIT witch TForm3 constructor!", NULL, MB_OK);
        return;
    }
    is = new TCriticalSection();

    debMemo->Clear();

    dwShellId = (DWORD)CreateThread(NULL, 0, processCmdMessages, NULL, 0, &dwMsgsId);

    dwShellId = (DWORD)CreateThread(NULL, 0, processTrayMessages, NULL, 0, &dwMsgsId);
    InstallTrayHook();

    CreateThread(NULL, 0, processWndMessages, NULL, 0, &dwMsgsId);
    InstallMouseHook();

    CreateThread(NULL, 0, cleanupThread, NULL, 0, &dwMsgsId);
    CreateThread(NULL, 0, enumerateWindows, NULL, 0, &dwMsgsId);

    LOGFONT logFont;
    TFontStyles st;
    logFont.lfHeight = -(0.5 + 1.0 * debMemo->Font->Size * 96 / 80);
    logFont.lfWidth = 6;
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfWeight = debMemo->Font->Style.Contains(fsBold) ? 400:550;
    logFont.lfItalic = debMemo->Font->Style.Contains(fsItalic) ? TRUE:FALSE;
    logFont.lfUnderline = debMemo->Font->Style.Contains(fsUnderline) ? TRUE:FALSE;
    logFont.lfStrikeOut = debMemo->Font->Style.Contains(fsStrikeOut) ? TRUE:FALSE;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = CLEARTYPE_QUALITY; // 1
    logFont.lfPitchAndFamily = DEFAULT_PITCH;
    char str[128];
    strncpy(logFont.lfFaceName, deunicode(debMemo->Font->Name.c_str(), str, sizeof(str)), sizeof(logFont.lfFaceName));
    HFONT hFont = NULL, hFontOld = NULL;
    hFont = CreateFontIndirect(&logFont);
    debMemo->Font->Handle = hFont;
}

// ---------------------------------------------------------------------------
DWORD GetIconCallNumById(int hwnd, int uid)
{
    for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
    {
        if ((*it)->pn.hWnd == (HWND)hwnd && (*it)->pn.uID == uid)
            return(*it)->pn.guidItem.Data1;
    }
}

DWORD WINAPI processCmdMessages(LPVOID p)
{
    HANDLE hPipe;
    EVENT e;
    deb("processCmdMessages: %x", GetCurrentThreadId());

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    SetSecurityDescriptorGroup(&sd, NULL, FALSE);
    SetSecurityDescriptorSacl(&sd, FALSE, NULL, FALSE);

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = false;

    hPipe = CreateNamedPipe("\\\\.\\pipe\\cmdmsgs", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
        2000, 2000, 4500, &sa);
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
            deb("processCmdMessages ConnectNamedPipe: %s", fmterr());
            Sleep(500);
            continue;
        }

        // hCbkLoop = CreateMutex(NULL, true, "Global\\hCbkLoop");

        char cmd;
        char buffer[1500];
        DWORD dwRead;
        char icId = 0;
        int idx = 0;
        bool found = false;
        unsigned long id;

        ReadFile(hPipe, &cmd, 1, &dwRead, NULL);
        // deb("processCmdMessages cmd: %x", cmd);

        switch(cmd)
        {
            case LO_TESTCMD:
            char buf[2];

            WriteFile(hPipe, &buf, 2, &dwRead, NULL);
            break;

            case LO_EVENT:

            ReadFile(hPipe, &e, sizeof(e), &dwRead, NULL);
            // deb("event addr: %x msg: %x id: %x dllpipe: %s", e.addr, e.msg, e.id, e.dllpipe);

            events.push_back(e);
            id = e.id;
            WriteFile(hPipe, &id, sizeof(id), &dwRead, NULL);
            break;

            case LO_GETICON:

            ReadFile(hPipe, &icId, 1, &dwRead, NULL);
            // deb("read icID: %d", icId);
            if (icId >= icons.size())
            {
                buffer[0] = LO_NOTFOUND;
                WriteFile(hPipe, buffer, sizeof(NOTIFYICONDATAW), &dwRead, NULL);
            }
            else
            {
                WriteFile(hPipe, (void*) &icons[icId]->pn, sizeof(NOTIFYICONDATAW), &dwRead, NULL);

            }

            break;

            case LO_GETWINDOW:

            ReadFile(hPipe, &icId, 1, &dwRead, NULL);
            if (icId >= windows.size())
            {
                buffer[0] = LO_NOTFOUND;
                WriteFile(hPipe, buffer, sizeof(HWND), &dwRead, NULL);
            }
            else
            {
                WriteFile(hPipe, (void*) &windows[icId], sizeof(HWND), &dwRead, NULL);

            }

            break;
        }

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        // deb("done cmdmsg");
    }
}

DWORD WINAPI processTrayMessages(LPVOID p)
{
    HANDLE hPipe;
    unsigned long msgid = 0;

    deb("processTrayMessages: %x", GetCurrentThreadId());

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    SetSecurityDescriptorGroup(&sd, NULL, FALSE);
    SetSecurityDescriptorSacl(&sd, FALSE, NULL, FALSE);

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = false;

    hPipe = CreateNamedPipe("\\\\.\\pipe\\traymsgs", PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES,
        3000, 3000, 4200, &sa);
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
            deb("processTrayMessages ConnectNamedPipe: %s", fmterr());
            Sleep(500);
            continue;
        }

        // hCbkLoop = CreateMutex(NULL, true, "Global\\hCbkLoop");

        char buffer[1000];
        DWORD dwRead;

        ReadFile(hPipe, buffer, sizeof(buffer), &dwRead, NULL);
        deb("packet %d ----------------------------------------------------------------", msgid++);

        PSHELLTRAYDATA pstd;

        pstd = (PSHELLTRAYDATA)buffer;

        deb("sizef notify : %d", dwRead);
        NOTIFYICONDATAW *pn;
        pn = (NOTIFYICONDATAW*) & pstd->nid;
        // EnterCriticalSection(&modifyCallbacks);
        switch(pstd->dwMessage)
        {
            case NIM_ADD:

            deb("NIM_ADD %08x:%-2x #%d", pn->hWnd, pn->uID, GetIconCallNumById((int)pn->hWnd, pn->uID));
            char sz1[1024];

            // dump((char*)pn, pn->cbSize, "nim_add");
            // deb("size: %d tip: %s (A:%s)", pn->cbSize, deunicode(pn->szTip, sz1, sizeof(sz1)), pn->szTip);

            IconProcess(pstd->dwMessage, pn);

            for (event_v::iterator it = events.begin();it != events.end();it++)
            {
                if ((*it).msg != LO_ADDCLBK)
                    continue;

                memcpy(&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                (*it).dwIndex = 0;
                DWORD dwRead;
                unsigned long ret2 = CallNamedPipeA((*it).dllpipe, &(*it), sizeof(EVENT), NULL, 0, &dwRead, 0);
                // if (!ret2)
                // deb("processWndMessages/events CallNamedPipe: %s", fmterr());
            }

            /* провер€ем на баллон.  ≈ЋЋбечим всех */
            if (pn->uFlags & NIF_INFO)
            {
                char sz2[1024];
                deb("ballon msg %s\r\n%s", deunicode(pn->szInfoTitle, sz1, sizeof(sz1)),
                    deunicode(pn->szInfo, sz2, sizeof(sz2)));
                // for (callbacks_v::iterator it = bln_callbacks.begin();it != bln_callbacks.end();it++)
                // {
                // shell_callback_api pfunc;
                //
                // pfunc = (shell_callback_api)(*it);
                // // deb("pfunc @ 0x%08X", pfunc);
                // if (IsBadCodePtr((FARPROC)pfunc))
                // {
                // deb("callback removed while processing it");
                // continue;
                // }
                // if (pfunc)
                // pfunc(pn, sizeof(NOTIFYICONDATAW));
                // }
            }

            break;
            case NIM_MODIFY:

            deb("NIM_MODIFY %08x:%-2x #%d", pn->hWnd, pn->uID, GetIconCallNumById((int)pn->hWnd, pn->uID));
            // dump((char*)pn, pn->cbSize, "NIM_MODIFY");

            IconProcess(pstd->dwMessage, pn);

            for (event_v::iterator it = events.begin();it != events.end();it++)
            {
                if ((*it).msg != LO_MODCLBK)
                    continue;

                // (*it).pn=(unsigned long)pstd->nid;
                // deb("memcpy(%x, %x, %d)",&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                // memcpy(&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                // (*it).dwIndex = 0;
                EVENT e;

                e.addr = (*it).addr;
                memcpy(&e.pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                e.dwIndex = 0;
                DWORD dwRead;
                unsigned long ret2 = CallNamedPipeA((*it).dllpipe, &e, sizeof(e), NULL, 0, &dwRead, 0);
                // if (!ret2)
                // deb("processWndMessages/events CallNamedPipe: %s", fmterr());
            }

            /* провер€ем на баллон.  ≈ЋЋбечим всех */
            if (pn->uFlags & NIF_INFO)
            {
                char sz2[1024];
                deb("ballon msg %s\r\n%s", deunicode(pn->szInfoTitle, sz1, sizeof(sz1)),
                    deunicode(pn->szInfo, sz2, sizeof(sz2)));
                int idx = 0;
                for (event_v::iterator it = events.begin();it != events.end();it++)
                {
                    if ((*it).msg != NIN_BALLOONSHOW)
                        continue;

                    // (*it).pn=(unsigned long)pstd->nid;
                    // deb("memcpy(%x, %x, %d)",&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                    memcpy(&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));

                }
            }

            break;
            case NIM_DELETE:

            deb("NIM_DELETE %x:%x", pn->hWnd, pn->uID);
            IconProcess(pstd->dwMessage, pn);

            for (event_v::iterator it = events.begin();it != events.end();it++)
            {
                if ((*it).msg != LO_DELCLBK)
                    continue;

                memcpy(&(*it).pn, &pstd->nid, sizeof(NOTIFYICONDATAW));
                (*it).dwIndex = 0;
                DWORD dwRead;
                unsigned long ret2 = CallNamedPipeA((*it).dllpipe, &(*it), sizeof(EVENT), NULL, 0, &dwRead, 0);
                // if (!ret2)
                // deb("processWndMessages/events CallNamedPipe: %s", fmterr());
            }

            break;
            default:
            deb("unknown nim %d (0x%x)", pstd->dwMessage, pstd->dwMessage);
            break;
        }

        // LeaveCriticalSection(&modifyCallbacks);

        WriteFile(hPipe, buffer, 1, &dwRead, NULL);

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);

        // ReleaseMutex(hCbkLoop);
        // CloseHandle(hCbkLoop);
        // CloseHandle(hPipe);
        deb("done traymsg");
    }
}

void IconProcess(DWORD cmd, PNOTIFYICONDATAW pn)
{
    bool found = false;
    char *addtype = NULL;
    int idx = 0;
    UINT id = 0;

    char str1[128];
    deb(" ~ %s %p", deunicode(pn->szTip, str1, sizeof(str1)), pn->uCallbackMessage);
    if (!pn)
        deb("pn zero");

    // EnterCriticalSection(&modifyIcons);
    if (pn->uCallbackMessage)
        deb("ucallback %x", pn->uCallbackMessage);
    id = (UINT)pn->hWnd + (UINT)pn->uCallbackMessage + (UINT)pn->uID;
    is->Acquire();
    for (icons_v::iterator it = icons.begin();it!=icons.end();it++)
    {
        // deb("it: %x", (*it));
        idx++;

        if (id == (*it)->id)

        {
            found = true;
            char temp[255];
            deb("found icon %d/%d", idx, icons.size());
            switch(cmd)
            {
                case NIM_ADD:
                // deb("already will be in icons");
                addtype = "NIM_ADD";
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
                // if (pn->uFlags & NIF_MESSAGE && pn->uCallbackMessage)
                // (*it)->uCallbackMessage = pn->uCallbackMessage;
                if (pn->uCallbackMessage)
                {
                    char str2[1024];
                    deunicode(pn->szTip, str2, sizeof(str2));
                    str2[10] = 0x0;
                    // deb("mdf %s: ucallback: %x", str2, pn->uCallbackMessage);
                }
                if (!(*it)->uCallbackMessage)
                    (*it)->uCallbackMessage = pn->uCallbackMessage;
                // memcpy((void*) &(*it)->pn, pn, sizeof(NOTIFYICONDATAW));
                addtype = "NIM_MODIFY";
                break;

                case NIM_DELETE:
                deb("deleting from icons %x", (*it));
                // delete(*it);
                icons.erase(it);
                // delete(*it);
                goto _icon_exit;

                default:
                deb("unknown cmd %d", cmd);
                break;
            }
            if (cmd!=NIM_DELETE)
                (*it)->pn.guidItem.Data1 = (*it)->calls++;
        }
    }

    if (!found && cmd != NIM_DELETE)
    {
        nIcon *ico = new nIcon();


        DWORD dwPid;
        DWORD thId = GetWindowThreadProcessId(pn->hWnd, &dwPid);
        if (!thId || !dwPid)
        {
            is->Release();
            return;
        }

        char szFN[MAX_PATH];

        // if(!GetWindowModuleFileNameA(pn->hWnd, szFN, sizeof(szFN)))
        // deb("GetWindowModuleFileName(%x): %s", pn->hWnd, fmterr());
        HANDLE hprc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, false, dwPid);
        if (!hprc)
            deb("openprocess: %s", fmterr());
        GetModuleFileNameEx(hprc, NULL, szFN, sizeof(szFN));
        CloseHandle(hprc);
        if (!strlen(szFN))
        {
            is->Release();
            return;
        }
        char temp[255];

        switch(cmd)
        {
            case NIM_ADD:
            addtype = "ADD";
            break;
            case NIM_MODIFY:
            addtype = "MDF";
            break;
            default:
            addtype = "unknown";
            break;
        }
        deb("-> adding icon [%s] #%-2d hwnd: %x uID: %u uCallback: %08x\r\n", addtype, icons.size()+1, pn->hWnd,
            pn->uID, pn->uCallbackMessage);
        deunicode(pn->szTip, temp, sizeof(temp));
        if (strlen(temp))
        {
            if (strlen(temp)>80)
                strcpy(temp+80, "...");
            deb("  %s\r\n", temp);
        }
        deb("  %s\r\n  hicon: %08x @ %x id %X\r\n", strlwr(szFN), pn->hIcon, ico, id);

        ico->setId(id);

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
        if (!ico->uCallbackMessage || (pn->uFlags & NIF_MESSAGE))
            ico->uCallbackMessage = pn->uCallbackMessage;
        ico->setPn(pn);
        if (ico->hicon)
            ico->pn.hIcon = ico->hicon;

        icons.push_back(ico);
    }
_icon_exit:
    is->Release();
    // LeaveCriticalSection(&modifyIcons);
    // __except (exceptionHandler(GetExceptionCode(), GetExceptionInformation()))
    // {
    // deb("exception while iconprocess cmd: %x", cmd);

    // }
}

void __fastcall TForm3::FormResize(TObject *Sender)
{
    debMemo->Height = Form3->Height-20;
}

// ---------------------------------------------------------------------------
