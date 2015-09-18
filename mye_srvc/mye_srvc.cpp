// ---------------------------------------------------------------------------
#include "mye_srvc.h"
#include "functions.h"
#include "srvc_main.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

Tloviossrvc *loviossrvc;
TSrvcThread *thr;

extern HANDLE hWrapProc;

// ---------------------------------------------------------------------------
__fastcall Tloviossrvc::Tloviossrvc(TComponent *Owner):TService(Owner)
{

}

TServiceController __fastcall Tloviossrvc::GetServiceController(void)
{
    return(TServiceController)ServiceController;
}

void __stdcall ServiceController(unsigned CtrlCode)
{
    loviossrvc->Controller(CtrlCode);
}

void __fastcall Tloviossrvc::ServiceStart(TService *Sender, bool &Started)
{

    thr = new TSrvcThread(false);

    Started = true;
    deb("mye service started");
}
// ---------------------------------------------------------------------------

void __fastcall Tloviossrvc::ServiceStop(TService *Sender, bool &Stopped)
{
    thr->Terminate();
    Stopped = true;

    TerminateProcess(hWrapProc, 0);
}

// ---------------------------------------------------------------------------
void __fastcall Tloviossrvc::ServiceExecute(TService *Sender)
{
    while (!Terminated)
    {
        ServiceThread->ProcessRequests(true);
    }
}
// ---------------------------------------------------------------------------
