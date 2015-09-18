// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "desktop.h"
#include <windows.h>
#include <windowsx.h>
#include "functions.h"
#include <shellapi.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <map.h>
#include <vector>
#include <list>
#include "Dbghelp.h"
#include "../mye/mye.h"
#include "symbols.h"

// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 * Form1;

typedef int(WINAPI * init)(void);
typedef int(WINAPI * unload)(void);
unload unloadProc;
init procInit;

std::list<unload>unloadProcs;
std::list<unload>::iterator next;
typedef std::vector<HANDLE> extensions;

extensions exs;

// ---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
   deb("destructor");

   for(extensions::iterator it = exs.begin();it != exs.end();it++)
   {
      deb("extension %x ", (*it));
      TerminateProcess((*it), 0);
   }
}

// ---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent * Owner) : TForm(Owner)
{
   deb("Form1 ready");

   kill_processes("lovi");
   kill_processes("VMWareTray");
   kill_processes("R&Q.exe");
   kill_processes("Skype");
   kill_processes("gismet");
   kill_processes("roxomitron");
   kill_processes("CUIT");
   kill_processes("example");

   // symshow();

   deb("created initial shell");

   adjustPriviledges();
   loadPlugins();
   loadExtensions();

   DWORD dwId;
   // CreateThread( NULL, 0, ( LPTHREAD_START_ROUTINE ) &cmdThread, NULL, NULL,
   // &dwId );
   HWND hwndtip = CreateToolTip(cmdEdit->Handle, 1, "������ ��������", "������� ���� �������� ���������", true);
   RECT rect;
   //::GetWindowRect(cmdEdit->Handle, &rect);
   ::GetWindowRect(cmdEdit->Handle, &rect);
   deb("client rect.left=%d rect.top=%d rect.right=%d rect.bottom=%d",
      rect.left, rect.top, rect.right, rect.bottom);
   POINT pt = {rect.left,rect.top};
   ::ClientToScreen(cmdEdit->Handle, &pt);
   //pt=cmdEdit->ClientToScreen(pt);
   //rect = cmdEdit->;
   deb("rect.left:%d rect.top:%d pt.x:%d pt.y:%d",
      rect.left,rect.top, pt.x, pt.y);
   SetCursorPos(pt.x, pt.y);
   //SendMessage(hwndtip, WM_MOUSEMOVE, 0, (LPARAM) MAKELONG(rect.left+1, rect.top+1));
   SendMessage(hwndtip, TTM_UPDATE, 0, 0);

}
// ---------------------------------------------------------------------------

int __fastcall TForm1::loadPlugins(void)
{
   char curdir[MAX_PATH];
   char dir[MAX_PATH];

   GetProcessImageFileName(GetCurrentProcess(), dir, sizeof(dir));

   PathRemoveFileSpec(dir);
   deb("base: %s", dir);
   int ret = SetCurrentDirectory(dir);
   if (ret != S_OK)
   {
      deb("failed change dir");
   }

   SetCurrentDirectory("c:\\temp\\myexplorer");

   GetCurrentDirectory(sizeof(curdir), curdir);
   deb("running in %s", curdir);
   snprintf(dir, sizeof(dir), "%s\\plugins", curdir);

   if (CreateDirectory(dir, NULL))
   {
      deb("no plugins directory [%s]", dir);
      return 0;
   }

   SetCurrentDirectory(dir);
   deb("loading plugins [%s]", dir);

   HANDLE fh;
   WIN32_FIND_DATA fd;

   memset(&fd, 0x0, sizeof(fd));
   fh = FindFirstFile("*.dll", &fd);
   deb("fh: %x", fh);
   if (fh == INVALID_HANDLE_VALUE)
   {
      fh = FindFirstFile("data/*.dll", &fd);
      if (fh == INVALID_HANDLE_VALUE)
      {
         deb("no plugins found");
         return 0;
      }
      else
      {
         deb("data dir plugins/");
      }

   }

   int nfiles = 0;
   char fn[MAX_PATH];
   int nloaded = 0;
   do
   {
      __try
      {

         strncpy(fn, fd.cFileName, sizeof(fn));

         HMODULE hm;

         deb("trying %s", fn);

         hm = LoadLibrary(fn);
         if (hm == NULL)
         {
            deb("failed load: %s", fmterr());
            continue;
         }

         procInit = (init)GetProcAddress(hm, "init");
         deb("calling init @ 0x%p %s", procInit, fn);
         unloadProc = (unload)GetProcAddress(hm, "unload");
         deb("unload @ 0x%p %s", unloadProc, fn);
         Application->ProcessMessages();
         if (procInit)
         {
            DWORD dwId;

            deb(" ### -> ProcInit()");
            procInit();
            deb(" ### <- ProcInit()");

            unloadProcs.push_back(unloadProc);

            nloaded++;
         }
         else
         {
            deb("not plugin");
         }
         // FreeLibrary(hm);
         nfiles++;
      }

      catch(...)
      {
         deb("plugins code exception catched on %s", fn);
      }
   }
   while (FindNextFile(fh, &fd));

   // xwalk_list();
   deb("loaded %d plugins", nloaded);
   return nloaded;
}

int __fastcall TForm1::loadExtensions(void)
{
   char curdir[MAX_PATH];
   char dir[MAX_PATH];

   GetProcessImageFileName(GetCurrentProcess(), dir, sizeof(dir));

   PathRemoveFileSpec(dir);
   deb("base: %s", dir);
   int ret = SetCurrentDirectory(dir);
   if (ret != S_OK)
   {
      deb("failed change dir");
   }

   SetCurrentDirectory("c:\\temp\\myexplorer");

   GetCurrentDirectory(sizeof(curdir), curdir);
   deb("running in %s", curdir);
   snprintf(dir, sizeof(dir), "%s\\plugins", curdir);

   if (CreateDirectory(dir, NULL))
   {
      deb("no plugins directory [%s]", dir);
      return 0;
   }

   SetCurrentDirectory(dir);
   deb("loading plugins [%s]", dir);

   HANDLE fh;
   WIN32_FIND_DATA fd;

   memset(&fd, 0x0, sizeof(fd));
   fh = FindFirstFile("*.exe", &fd);
   deb("fh: %x", fh);
   if (fh == INVALID_HANDLE_VALUE)
   {
      fh = FindFirstFile("*.exe", &fd);
      if (fh == INVALID_HANDLE_VALUE)
      {
         deb("no plugins found");
         return 0;
      }
      else
      {
         deb("data dir plugins/");
      }

   }

   int nfiles = 0;
   char fn[MAX_PATH];
   int nloaded = 0;
   do
   {
      __try
      {

         strncpy(fn, fd.cFileName, sizeof(fn));

         HMODULE hm;

         deb("trying %s", fn);

         STARTUPINFO si;
         PROCESS_INFORMATION pi;

         memset(&pi, 0x0, sizeof(pi));
         memset(&si, 0x0, sizeof(si));
         si.cb = sizeof(si);

         char cmdline[1024];

         sprintf(cmdline,"%d", Form1->Handle);

         CreateProcess(fn, cmdline, NULL, NULL, false, CREATE_NO_WINDOW, NULL,
            NULL, &si, &pi);

         exs.push_back(pi.hProcess);
         // FreeLibrary(hm);
         nfiles++;
      }

      catch(...)
      {
         deb("plugins code exception catched on %s", fn);
      }
   }
   while (FindNextFile(fh, &fd));

   // xwalk_list();
   deb("loaded %d plugins", nloaded);
   return nloaded;
}

void __fastcall TForm1::install(void)
{
   HKEY pKey;
   DWORD ret;
   ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", NULL,
      KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_EXECUTE | KEY_WRITE,
      &pKey);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed opening winlogon hkey ret 0x%x: %s", ret, fmterr(ret));
      return;
   }
   char path[MAX_PATH];

   strncpy(path, "c:\\temp\\myexplorer\\myexplorer.exe", sizeof(path));

   ret = RegSetValueEx(pKey, "Shell", NULL, REG_SZ, (LPBYTE)path,
      strlen(path) + 1);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed changing shell hkey value ret 0x%x: %s", ret, fmterr(ret));
      return;
   }
   RegCloseKey(pKey);
   /////////////////////////////////////////////////////////////////////////////////

   ret = RegOpenKeyEx(HKEY_CURRENT_USER,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", NULL,
      KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_EXECUTE | KEY_WRITE,
      &pKey);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed opening winlogon hkey ret 0x%x: %s", ret, fmterr(ret));
      return;
   }

   strncpy(path, "c:\\temp\\myexplorer\\myexplorer.exe", sizeof(path));

   ret = RegSetValueEx(pKey, "Shell", NULL, REG_SZ, (LPBYTE)path,
      strlen(path) + 1);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed changing shell hkey value ret 0x%x: %s", ret, fmterr(ret));
      return;
   }
   RegCloseKey(pKey);
   deb("installed as default shell");
}

void __fastcall TForm1::uninstall(void)
{
   HKEY pKey;
   DWORD ret;
   ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", NULL,
      KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_EXECUTE | KEY_WRITE,
      &pKey);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed opening winlogon hkey ret 0x%x: %s", ret, fmterr(ret));
      return;
   }

   char path[MAX_PATH];
   strncpy(path, "explorer.exe", sizeof(path));

   ret = RegSetValueEx(pKey, "Shell", NULL, REG_SZ, (LPBYTE)path,
      strlen(path) + 1);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed changing shell hkey value ret 0x%x: %s", ret, fmterr(ret));
      return;
   }
   RegCloseKey(pKey);
   ///////////////////////////////////////////////////////////////////////////////////
   ret = RegOpenKeyEx(HKEY_CURRENT_USER,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", NULL,
      KEY_READ | KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_EXECUTE | KEY_WRITE,
      &pKey);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed opening winlogon hkey ret 0x%x: %s", ret, fmterr(ret));
      return;
   }

   strncpy(path, "explorer.exe", sizeof(path));

   ret = RegSetValueEx(pKey, "Shell", NULL, REG_SZ, (LPBYTE)path,
      strlen(path) + 1);

   if (ret != ERROR_SUCCESS)
   {
      deb("failed changing shell hkey value ret 0x%x: %s", ret, fmterr(ret));
      return;
   }
   RegCloseKey(pKey);
   deb("installed explorer.exe as shell");
}

void __fastcall TForm1::adjustPriviledges(void)
{
   HANDLE hToken;
   TOKEN_PRIVILEGES tkp;
   if (!OpenProcessToken(GetCurrentProcess(),
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
   {
      deb("OpenProcessToken");
   }

   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1; // one   privilege   to   set
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
   if (GetLastError() != ERROR_SUCCESS)
   {
      deb("AdjustTokenPrivileges");
   }

}

void __fastcall TForm1::Button2Click(TObject * Sender)
{
   install();
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject * Sender)
{
   uninstall();
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Button3Click(TObject * Sender)
{
   ShellExecute(Application->Handle, "open", "c:\\temp\\myexplorer", NULL,
      NULL, SW_SHOW);
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Button4Click(TObject * Sender)
{
   ExitWindowsEx(EWX_LOGOFF,
      SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE |
      SHTDN_REASON_FLAG_PLANNED);
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::Button5Click(TObject * Sender)
{
   ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_APPLICATION);
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject * Sender, TCloseAction & Action)
{
   // unloadProc();
   int i = 0;
   deb("plugins loaded: %d", unloadProcs.size());

   for (next = unloadProcs.begin(); next != unloadProcs.end(); next++)
   {
      unloadProc = *next;
      deb("unloading plugin %d, calling proc at 0x%08x", i++, unloadProc);
      unloadProc();
   }
}
// ---------------------------------------------------------------------------

void __fastcall TForm1::Button6Click(TObject * Sender)
{
   ShellExecute(Application->Handle, "open", "explorer.exe", NULL, NULL,
      SW_SHOW);
}

// ---------------------------------------------------------------------------
void __fastcall TForm1::Button7Click(TObject * Sender)
{
   char cmd[128];

   deunicode(cmdEdit->Text.c_str(), cmd, sizeof(cmd));
   ShellExecute(Application->Handle, "open", cmd, NULL, NULL, SW_SHOW);
}

// ---------------------------------------------------------------------------

void __fastcall TForm1::cmdEditKeyDown(TObject * Sender, WORD & Key,
   TShiftState Shift)

{
   if (Key == '\r' || Key == '\n')
      Button7Click(this);
}
// ---------------------------------------------------------------------------

DWORD WINAPI cmdThread(LPVOID p)
{
   deb("cmdThread: %x", GetCurrentThreadId());

   HANDLE hPipe;

   hPipe = CreateNamedPipe("\\\\.\\pipe\\mye_cmds", PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 1024, 1024,
      5000, NULL);

   while (1)
   {
      Application->ProcessMessages();

      bool bConnected = false;
      bConnected = ConnectNamedPipe(hPipe, NULL);
      deb("%s", bConnected ? "connected pipe" : "not connected pipe");
      if (!bConnected)
         continue;

      char buffer[1024];
      DWORD dwRead = 0;

      ReadFile(hPipe, buffer, 1, &dwRead, NULL);

      switch(buffer[0])
      {
         // *****************************************************************************************************
      case CMD_DOCKWINDOW:
         dock_window * dw;
         ReadFile(hPipe, buffer, sizeof(dw), &dwRead, NULL);
         dw = (dock_window*)buffer;
         deb("cmd DOCK_WINDOW: hwnd: %x where: %d", dw->hwnd, dw->where);
         break;

         // *****************************************************************************************************
      default:
         deb("unknown cmd %d", buffer[0]);
         break;
      }
      DisconnectNamedPipe(hPipe);
   }
}
