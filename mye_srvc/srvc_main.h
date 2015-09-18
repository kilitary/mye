//---------------------------------------------------------------------------

#ifndef srvc_mainH
#define srvc_mainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
class TSrvcThread : public TThread
{
private:
protected:
    void __fastcall Execute();
public:
    __fastcall TSrvcThread(bool CreateSuspended);
};
//---------------------------------------------------------------------------
#endif
