// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "srvc_main.h"
#include "functions.h"
#pragma package(smart_init)
HANDLE hWrapProc;
// ---------------------------------------------------------------------------

// Important: Methods and properties of objects in VCL can only be
// used in a method called using Synchronize, for example:
//
// Synchronize(&UpdateCaption);
//
// where UpdateCaption could look like:
//
// void __fastcall TSrvcThread::UpdateCaption()
// {
// Form1->Caption = "Updated in a thread";
// }
// ---------------------------------------------------------------------------

__fastcall TSrvcThread::TSrvcThread(bool CreateSuspended):TThread(CreateSuspended)
{
}

// ---------------------------------------------------------------------------
void __fastcall TSrvcThread::Execute()
{
    deb("service executed");
    HANDLE hUserToken = (HANDLE)0x1;

    hUserToken = NULL;
    while(!hUserToken)
    {
        hUserToken = get_user_token();
        Sleep(200);
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    memset(&si,0,sizeof(si));
    memset(&pi,0,sizeof(pi));
    CreateProcessAsUser(hUserToken, "c:\\projects\\osServiceWrapper.exe",NULL,NULL,NULL,NULL,NULL,NULL,NULL,&si,&pi);
    deb("wrapper process hndl: %x",pi.hProcess);
    hWrapProc=pi.hProcess;
    while (!Terminated)
    {
        Sleep(1000);
    }
}
// ---------------------------------------------------------------------------
