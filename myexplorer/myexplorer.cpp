// ---------------------------------------------------------------------------
LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS * ei);

// #include <vcl.h>
#pragma hdrstop
#pragma link "madExcept"
#pragma link "madLinkDisAsm"
#include <tchar.h>
#include <windows.h>

#include "symbols.h"
#include "functions.h"
// ---------------------------------------------------------------------------
USEFORM("desktop.cpp", Form1);
//---------------------------------------------------------------------------
HINSTANCE hInst;
// ---------------------------------------------------------------------------
LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS * ei)
{
   deb("Exception in myexplorer.exe code: 0x%08X @ 0x%08X ",
      ei->ExceptionRecord->ExceptionCode,
      ei->ExceptionRecord->ExceptionAddress);
   deb("can-continue: %s", (ei->ExceptionRecord->ExceptionFlags ==
         EXCEPTION_NONCONTINUABLE) ? "no" : " yes");
}

WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{
   CONTEXT lpc;
   hInst = hInstance;
   // GetThreadContext(GetCurrentThread(), &lpc);
   // deb("lpc: %x", lpc.Eip);
   //symshow();

   //SetUnhandledExceptionFilter(UnhandledExceptionFilter);
   deb("tWinMAin");
   try
   {
      deb("initializing ....");
      Application->Initialize();
      Application->MainFormOnTaskBar = false;
      deb("creating main form ...");
      Application->CreateForm(__classid(TForm1), &Form1);
         deb("running application ...");
      Application->Run();
   }
   catch(Exception & exception)
   {
      Application->ShowException(&exception);
   }
   catch(...)
   {
      try
      {
         throw Exception("");
      }
      catch(Exception & exception)
      {
         Application->ShowException(&exception);
      }
   }
   return 0;
}
// ---------------------------------------------------------------------------
