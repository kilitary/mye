#ifndef nicon_included
#define nicon_included
#include "windows.h"
#include <vector>

class nIcon
{

public:
   RECT rect;

   UINT left;
   UINT right;
   UINT top;
   UINT bottom;

   char tip[64];
   HICON hicon;
   UINT id;
   UINT uID;
   UINT uCallbackMessage;
   HWND hwnd;
   UINT uFlags;
   char szInfo[256];
   bool balloon_icon;

   union
   {
      UINT uTimeout;
      UINT uVersion;
   };

   char szInfoTitle[64];
   DWORD dwInfoFlags;
   HICON hBalloonIcon;
   bool balloon_showing;

   UINT modify_calls;
   UINT calls;

   nIcon(RECT, char * , HICON, UINT);
   nIcon(void);
   ~nIcon(void);

   void setTip(char * ptip);
   void setRect(RECT prect);
   void setId(UINT pid);
   void setHicon(HICON phicon);
   void setuCallbackMessage(UINT);
   void setPn(PNOTIFYICONDATAW pn2);

   NOTIFYICONDATAW pn;
};

#endif

