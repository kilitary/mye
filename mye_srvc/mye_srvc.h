//---------------------------------------------------------------------------
#ifndef mye_srvcH
#define mye_srvcH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Classes.hpp>
#include <SvcMgr.hpp>
#include <vcl.h>
//---------------------------------------------------------------------------
class Tloviossrvc : public TService
{
__published:    // IDE-managed Components
    void __fastcall ServiceStart(TService *Sender, bool &Started);
    void __fastcall ServiceStop(TService *Sender, bool &Stopped);
    void __fastcall ServiceExecute(TService *Sender);
 
private:        // User declarations
public:         // User declarations
	__fastcall Tloviossrvc(TComponent* Owner);
	TServiceController __fastcall GetServiceController(void);

	friend void __stdcall ServiceController(unsigned CtrlCode);
};
//---------------------------------------------------------------------------
extern PACKAGE Tloviossrvc *loviossrvc;
//---------------------------------------------------------------------------
#endif
