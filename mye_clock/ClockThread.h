//---------------------------------------------------------------------------

#ifndef ClockThreadH
#define ClockThreadH
//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
class ClockThread : public TThread
{
protected:
    void __fastcall Execute();
public:
    __fastcall ClockThread(bool CreateSuspended);

};
//---------------------------------------------------------------------------
 void  onpaint(void);
#endif
