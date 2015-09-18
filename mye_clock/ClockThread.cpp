// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ClockThread.h"
#include "functions.h"
#include "main.h"
#include "../mye/mye.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#pragma package(smart_init)

extern HWND hwnd;
extern HDC hdc;
extern TForm1 *Form1;
extern HMODULE hLib;
mye_DockWindow_api dockwindow;
// ---------------------------------------------------------------------------

// Important: Methods and properties of objects in VCL can only be
// used in a method called using Synchronize, for example:
//
// Synchronize(&UpdateCaption);
//
// where UpdateCaption could look like:
//
// void __fastcall ClockThread::UpdateCaption()
// {
// Form1->Caption = "Updated in a thread";
// }
// ---------------------------------------------------------------------------

__fastcall ClockThread::ClockThread(bool CreateSuspended):TThread(CreateSuspended)
{

}

void onpaint(void)
{

    SendMessage(Form1->Handle, WM_ONCUSTOMPAINT, 0, 0);
}

// ---------------------------------------------------------------------------
void __fastcall ClockThread::Execute()
{
    //SetWindowPos(Form1->Handle, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_HIDEWINDOW);
    // ShowWindow(Application->MainFormHandle, SW_HIDE);
    // int style = GetWindowLong(Application->MainFormHandle, GWL_EXSTYLE);
    // style |= WS_EX_TOOLWINDOW;
    //
    // SetWindowLong(Application->MainFormHandle, GWL_EXSTYLE, style);
    // ShowWindow(Application->MainFormHandle, SW_SHOW);

 //   deb("hwnd to dock: %x", Form1->Handle);
 //   deb("dockwindow @ %p onpaint @ %p", dockwindow, onpaint);
    dockwindow(Form1->Handle, 0, 0, onpaint);
   // deb("done dockwindow()");
    SetWindowLong(Form1->Handle,GWL_EXSTYLE,WS_EX_TOOLWINDOW);
    SetWindowPos(Form1->Handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
}
// ---------------------------------------------------------------------------
