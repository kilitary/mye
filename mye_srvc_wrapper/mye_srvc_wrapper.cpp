//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "mye_srvc_wrapper.h"
#include "functions.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm3 *Form3;
//---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent* Owner)
    : TForm(Owner)
{
    ShowWindow(Application->MainFormHandle, SW_HIDE);
    Application->MainFormOnTaskBar=false;
    Hide();

    deb("srvc_wrapper");
    HMODULE hLib;

    hLib = LoadLibrary("mye.dll");
    deb("hLib:%x", hLib);
}
//---------------------------------------------------------------------------
