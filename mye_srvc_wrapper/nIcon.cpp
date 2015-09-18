#define _CRT_SECURE_NO_WARNINGS
#include "nIcon.h"
#include "functions.h"
#include <vector>


 nIcon::nIcon(RECT prect, char * ptip, HICON phicon, UINT pid)
{
   rect = prect;
   strncpy(tip, ptip, sizeof(tip));
   hicon = CopyIcon(phicon);
   id = pid;
   modify_calls = 0;
   deb("new nIcon: %d", id);
   balloon_icon = 0;
   calls=0;
}

 void nIcon::setuCallbackMessage(UINT msgid)
{
   uCallbackMessage = msgid;
}

 nIcon::nIcon()
{
   memset(tip, 0x0, sizeof(tip));
   hicon = NULL;
   memset(&rect, 0x0, sizeof(rect));
   balloon_showing = false;
   modify_calls = 0;
   balloon_icon = 0;
}

 nIcon::~nIcon(void)
{
   if (hicon)
      DestroyIcon(hicon);
}

void nIcon::setTip(char * ptip)
{
   memcpy(tip, ptip, sizeof(tip));
}

void nIcon::setRect(RECT prect)
{
   rect = prect;
}

void nIcon::setId(UINT pid)
{
   id = pid;
}

void nIcon::setHicon(HICON phicon)
{
   if(hicon)
    DestroyIcon(hicon);

   if(! (hicon = CopyIcon(phicon)))
   {
       deb("failed to CopyIcon: %s", fmterr());
   }
   // hicon = phicon;
}
void nIcon::setPn(PNOTIFYICONDATAW pn2)
{
    memcpy((void*) &pn, pn2, sizeof(NOTIFYICONDATAW));
}
