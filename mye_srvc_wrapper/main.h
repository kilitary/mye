//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <vector>
#include "nIcon.h"
#include "../mye/mye.h"


typedef std::vector<EVENT>event_v;
typedef std::vector<nIcon*>icon_v;
typedef std::vector<HWND>hwnd_v;

/* прототип каллбечной функции */
typedef int(WINAPI * shell_callback_api)(LPVOID p, unsigned long size);
/* прототип функции событий */
typedef int(WINAPI * shell_events_api)(DWORD dwIndex, PNOTIFYICONDATAW pn);

typedef LRESULT(CALLBACK * CallWndProc)(int nCode, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK * MouseWndProc)(int nCode, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------------
class TForm3 : public TForm
{
__published:	// IDE-managed Components
    TMemo *debMemo;
    void __fastcall FormResize(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TForm3(TComponent* Owner);
    __fastcall ~TForm3(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm3 *Form3;

//---------------------------------------------------------------------------
DWORD WINAPI processTrayMessages(LPVOID p);
DWORD WINAPI processCmdMessages(LPVOID p);
void InstallTrayHook(void);
void IconProcess(DWORD cmd, PNOTIFYICONDATAW pn);
#endif
