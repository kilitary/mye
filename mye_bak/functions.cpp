#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#define NOCOMM

#include <windows.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <tlhelp32.h>
#include <errno.h>
#include <shlwapi.h>
#include <psapi.h>
#include <strsafe.h>

#include "functions.h"
#ifdef FRELEASE
#define deb ////
#endif

#define ERR -1

// #define FORMATERROR fmterr()

#define u_char unsigned char
#define DEB_BUFF 1024
#define snprintf _snprintf

#define XWALK_CALL_STEPS 100

HANDLE heap_hnd = NULL;

bool xwalk_active = false;
bool _xheap_corrupted_warn_show = true;
DWORD _xalloc_calls = 0;
DWORD _xfree_calls = 0;
DWORD _xwalk_calls = 0;
DWORD _xwalk_step = 100;
DWORD _ffree_calls = 0;

BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey)
{
   LPTSTR lpEnd;
   LONG lResult;
   DWORD dwSize;
   TCHAR szName[MAX_PATH];
   HKEY hKey;
   FILETIME ftWrite;

   // First, see if we can delete the key without having
   // to recurse.

   lResult = RegDeleteKey(hKeyRoot, lpSubKey);

   if (lResult == ERROR_SUCCESS)
      return TRUE;

   lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

   if (lResult != ERROR_SUCCESS)
   {
      if (lResult == ERROR_FILE_NOT_FOUND)
      {
         printf("Key not found.\n");
         return TRUE;
      }
      else
      {
         printf("Error opening key.\n");
         return FALSE;
      }
   }

   // Check for an ending slash and add one if it is missing.

   lpEnd = lpSubKey + lstrlen(lpSubKey);

   if (*(lpEnd - 1) != TEXT('\\'))
   {
      *lpEnd = TEXT('\\');
      lpEnd++;
      *lpEnd = TEXT('\0');
   }

   // Enumerate the keys

   dwSize = MAX_PATH;
   lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL, &ftWrite);

   if (lResult == ERROR_SUCCESS)
   {
      do
      {

         StringCchCopy(lpEnd, MAX_PATH * 2, szName);

         if (!RegDelnodeRecurse(hKeyRoot, lpSubKey))
         {
            break;
         }

         dwSize = MAX_PATH;

         lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL, NULL, NULL,
            &ftWrite);

      }
      while (lResult == ERROR_SUCCESS);
   }

   lpEnd--;
   *lpEnd = TEXT('\0');

   RegCloseKey(hKey);

   // Try again to delete the key.

   lResult = RegDeleteKey(hKeyRoot, lpSubKey);

   if (lResult == ERROR_SUCCESS)
      return TRUE;

   return FALSE;
}

// *************************************************************
//
// RegDelnode()
//
// Purpose:    Deletes a registry key and all it's subkeys / values.
//
// Parameters: hKeyRoot    -   Root key
// lpSubKey    -   SubKey to delete
//
// Return:     TRUE if successful.
// FALSE if an error occurs.
//
// *************************************************************

BOOL RegDelnode(HKEY hKeyRoot, LPTSTR lpSubKey)
{
   TCHAR szDelKey[MAX_PATH * 2];

   StringCchCopy(szDelKey, MAX_PATH * 2, lpSubKey);
   return RegDelnodeRecurse(hKeyRoot, szDelKey);
}

DWORD GetDllVersion(LPCTSTR lpszDllName)
{
   HINSTANCE hinstDll;
   DWORD dwVersion = 0;

   /* For security purposes, LoadLibrary should be provided with a
   fully-qualified path to the DLL. The lpszDllName variable should be
   tested to ensure that it is a fully qualified path before it is used. */
   hinstDll = LoadLibrary(lpszDllName);

   if (hinstDll)
   {
      DLLGETVERSIONPROC pDllGetVersion;
      pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll,
         "DllGetVersion");

      /* Because some DLLs might not implement this function, you
      must test for it explicitly. Depending on the particular
      DLL, the lack of a DllGetVersion function can be a useful
      indicator of the version. */

      if (pDllGetVersion)
      {
         DLLVERSIONINFO dvi;
         HRESULT hr;

         ZeroMemory(&dvi, sizeof(dvi));
         dvi.cbSize = sizeof(dvi);

         hr = (*pDllGetVersion)(&dvi);

         if (SUCCEEDED(hr))
         {
            dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
         }
      }

      FreeLibrary(hinstDll);
   }
   return dwVersion;
}

char * unstr(char * str)
{
   static char undecoded[128];
   char * p = str;
   int idx = 0;

   memset(undecoded, 0x0, sizeof(undecoded));

   while (*p)
   {
      undecoded[idx] = *p;
      idx++;
      p += 2;
   }

   return undecoded;
}
CRITICAL_SECTION deb_cs;

#ifndef FRELEASE

void deb(char *formatstring, ...)
{
    va_list args;
    char modname[] = "mye.dll";
    char logfile[] = "myedll.txt";
    // static char buff[DEB_BUFF];
    char *buff;
    int this_step = 0; // ,  er_backward_sp_check_activated_nseek = 10, need_view = 4;
    static bool first_run = true;

    if (first_run)
    {
        DeleteFile(logfile);
        DeleteFile("plugins/logfile.txt");
        first_run = false;
    }

    va_start(args, formatstring);
    int len=vsnprintf(buff, 0, formatstring, args);
   // char sz1[128];
   // sprintf(sz1,"need %d bytes",len);
   // OutputDebugString(sz1);
    buff = (char*)malloc(len*5);

    ZeroMemory(buff, len*5);

    DWORD thId;
    char temp[16384];
    char spacers[128];

    thId = GetCurrentThreadId();

    ZeroMemory(spacers, sizeof(spacers));
    ZeroMemory(temp, 16384);

    _vsnprintf(buff, len, formatstring, args);
    va_end(args);
    FILE *fp = NULL;
    fp = fopen(logfile, "a");
    if (!fp)
        OutputDebugString("failed to open log file");

    char *p = strtok(buff, "\r\n");
    if (!p)
    {
        _snprintf(temp, 16384, "%04X %10s: %s\r\n", thId, modname, buff);
      
        OutputDebugString(temp);
        fwrite(temp, strlen(temp), 1, fp);

    }
    while (p)
    {
        _snprintf(temp, 16384, "%04X %10s: %s\r\n", thId, modname, p);

        OutputDebugString(temp);
        fwrite(temp, strlen(temp), 1, fp);

        p = strtok(NULL, "\r\n");

    }
    fclose(fp);
    // if (!strstr(buff, "\n"))
    // strncat(buff, "\r\n", sizeof(buff));
    // }


    // running = false;

    // buff[strlen(buff)]=0x0;
    // buff[strlen(buff)-1]=0x0;

    free(buff);
//    OutputDebugString("exit deb())");
    return;
}
#endif

void kill_processes(char * str)
{
   DWORD procs[500];
   DWORD numret = 0;
   int ret = EnumProcesses(procs, sizeof(procs), &numret);
   numret = numret / sizeof(DWORD);
   // deb("procs: %d kill process: %s", numret, str);

   for (int i = 0; i < numret; i++)
   {
      if (!procs[i])
         continue;

      HANDLE hProc;

      hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, false,
         procs[i]);
      if (!hProc)
      {
         // deb("failed to openprocess %d: %s",
         // procs[i], fmterr());
         continue;
      }

      char szPath[MAX_PATH];
      DWORD dwSize = sizeof(szPath);

      GetProcessImageFileName(hProc, szPath, dwSize);
      deb("process %d: %s", procs[i], szPath);
      if (strstr(szPath, str))
      {
         deb("killing %d", procs[i]);
         if (!TerminateProcess(hProc, 0))
            deb("failed to terminate proc: %s", fmterr());
      }
   }
}

void dropmem(char * fname, char * buf, int size)
{
   FILE * fp;

   deb("dropmem('%s', 0x%08p, %d)", fname, buf, size);

   fp = fopen(fname, "w");
   if (!fp)
   {
      // deb("fopen not open  file errno: %d",errno);
      return;
   }
   int ret = fwrite(buf, 1, size, fp);
   if (ret != size)
   {
      deb("fwrite wrote not all bytes (%d froom %d)", ret, size);
      return;
   }
   if (fclose(fp))
   {
      // deb("failed to fclose file errno: %d", errno);
      return;
   }
}

int DropFile(char * fname, unsigned char * fndata, DWORD size)
{
   DWORD written = 0;
   HANDLE hFile;
   char path[MAX_PATH];

   DeleteFile(fname);

   // if(IsBadReadPtr( (const void*)fndata, size))
   // {
   // deb("DropFile: 0x%p is bad read ptr (size %d) fn: %s", fndata, size, fname);
   // }
   // GetWindowsDirectory(path,MAX_PATH);
   /* if(!strstr("\\", path))
   {
   GetCurrentDirectory(MAX_PATH,path);
   strncat(path,"\\",sizeof(path));
   strncat(path,fname,sizeof(path));
   } else {
   strncpy(path, fname, sizeof(path));
   }
   deb("writing %d bytes @ 0x%08p to %s", size, fndata, path); */

   hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
   {
      deb("dropfile: failed to create file %s error: %s\n", fname, fmterr());
      CloseHandle(hFile);
      return -1;
   }
   // deb("dwwritten: %d", written);

   DWORD ret = SetFilePointer(hFile, 0, 0, FILE_BEGIN);

   if (ret == 0xFFFFFFFF || ret == ERROR_NEGATIVE_SEEK)
      deb("negative seek error or seek error: %s", fmterr());

   if (!WriteFile(hFile, (CONST VOID*)fndata, (unsigned long)size, &written,
         NULL))
   {
      deb("error writing %d bytes to %s: %s", size, path, fmterr());
   }
   if (written != size)
   {
      deb("dropfile: not all bytes written! written:%lu need:%lu", written,
         size);
      deb("errornum %d", GetLastError());
      CloseHandle(hFile);
      return -1;
   }
   CloseHandle(hFile);

   // GetCurrentDirectory(sizeof(path), path);
   deb("DropFile %s OK - %lu ", fname, size);
   return 0;
}

DWORD DropFile_getmax(DWORD max)
{

   DWORD norm = max;

   if (!max)
      return 0;

   // while(norm > max && (norm > 1 && max))
   // norm = max - rand()%max;
   norm -= (rand() % max);

   // deb("%u getmax(%u)", norm, max);

   return norm;
}

int DropFileRnd(char * fname, unsigned char * fndata, DWORD size)
{

   HANDLE hFile;
   char path[MAX_PATH];

   // if(IsBadReadPtr( (const void*)fndata, size))
   // {
   // deb("DropFile: 0x%p is bad read ptr (size %d) fn: %s", fndata, size, fname);
   // }
   // GetWindowsDirectory(path,MAX_PATH);
   // if(!strstr("\\", path))
   // {
   // GetCurrentDirectory(MAX_PATH,path);
   // strncat(path,"\\",sizeof(path));
   // strncat(path,fname,sizeof(path));
   // path[strlen(path)] = rand();
   // } else {
   strncpy(path, fname, sizeof(path));
   // }

   hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
   {
      deb("dropfile: failed to create file %s error: %s\n", path, fmterr());
      CloseHandle(hFile);
      return -1;
   }

   DWORD ret = SetFilePointer(hFile, 0, 0, FILE_BEGIN);

   if (ret == 0xFFFFFFFF || ret == ERROR_NEGATIVE_SEEK)
      deb("negative seek error or seek error: %s", fmterr());

   deb("chunk writing %u bytes @ 0x%08p to %s", size, fndata, path);
   // deb("dwwritten: %d", written);

   DWORD dw2Write = 1;
   DWORD fullsize = size;
   bool cc = false;
   DWORD pass = 0;
   DWORD written = 0;
   // ahuet'
   srand((unsigned int)(time(NULL) - size + GetCurrentThreadId()
         + GetCurrentProcessId()));

   while (fullsize)
   {
      dw2Write = DropFile_getmax(fullsize);
      unsigned char * pb = (unsigned char*)
          ((DWORD)fndata + ((cc ? (DWORD)written : 0)));

      deb(
         " #%03d fullsize: %-10u dw2Write: %-10u pb @ %08p fndata @ 0x%08p cc:%s"
         , pass, fullsize, dw2Write, pb, fndata, cc ? "true" : "false");

      if (!WriteFile(hFile, (LPVOID)pb, dw2Write, &written, NULL))
         deb("error writing %d bytes to %s: %s", dw2Write, path, fmterr());

      if (written != dw2Write)
      {
         deb("dropfile: not all bytes written(%s)! written:%lu need:%lu ",
            fmterr(), written, dw2Write);
         CloseHandle(hFile);
         return -1;
      }

      cc = true;
      pass++;
      fullsize -= written;
      deb("      written:  %-10u fullsize: %-10u\r\n", written, fullsize);
   }
   CloseHandle(hFile);

   // GetCurrentDirectory(sizeof(path), path);
   // if(!fullsize)
   deb("DropFile %s OK - %d (fullsize %u, last2write: %d, passes: %u)", fname,
      size, fullsize, dw2Write, pass);
   return 0;
}

char * dern(char * str)
{
   for (int i = 0; str[i]; i++)
   {
      if (str[i] == '\r' || str[i] == '\n' || str[i] == '%')
      {
         str[i] = ' ';
      }
   }
   char * p = str;

   for (int i = 0; p[i]; i++)
   {
      if (p[i] == 0x20 && p[i + 1] == 0x20)
      {
         strncpy(p + i, p + i + 1, strlen(str));
         // p[i+2]=0x0;
      }
   }
   return str;
}

USHORT checksum(USHORT * buffer, int size)
{
   unsigned long cksum = 0;

   while (size > 1)
   {
      cksum += *buffer++;
      size -= sizeof(USHORT);
   }

   if (size)
   {
      cksum += *(UCHAR*)buffer;
   }

   cksum = (cksum >> 16) + (cksum & 0xffff);
   cksum += (cksum >> 16);

   return(USHORT)(~cksum);
}

void hexdump(char * buffer, int size)
{
   int i, d;
   char szOut[128];
   char szTemp[128];

   szOut[0] = 0x0;
   deb("dumping buffer at %p size: %d\n", buffer, size);
   deb("hex: ");
   for (d = 0; d < size; d++)
   {
      wsprintf(szOut, "hex: ");
      for (i = 0; i < 12; i++)
      {
         wsprintf(szTemp, "0x%02x ", (u_char)buffer[d++]);
         lstrcat(szOut, szTemp);
      }
      deb("%s", szOut);
   }
   deb("end of dump.\n");
}
/*
int resolve(char* Host)
{
DWORD addr;
struct hostent *he;

addr = inet_addr(Host);

if(addr != INADDR_NONE) {
return addr;
}

he = gethostbyname((char FAR*) Host);

if(he == NULL) {
deb("resolve error: %d",WSAGetLastError());
return NULL;
}

addr = ((struct in_addr *) (he->h_addr_list[0]))->s_addr;

return addr;
} */

char * format_system_error(DWORD error)
{
   static char message[1024];
   LPVOID lpMsgBuf;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL,
            SUBLANG_DEFAULT), // Default language
         (LPTSTR) & lpMsgBuf, 0, NULL) == 0)
   {
      deb("error while formatting message: %s", fmterr());
      return "FORMATMESSAGE ERROR";
   }

   strncpy(message, (char*)lpMsgBuf, sizeof(message));

   LocalFree(lpMsgBuf);

   return message;
}

void dump(char * data, int size, char* tag)
{
   static int num = 0;
   static bool run = false;

   if (!run)
   {
      HANDLE fh;
      WIN32_FIND_DATA fd;

      memset(&fd, 0x0, sizeof(fd));
      fh = FindFirstFile("*.txt", &fd);
      // deb("dump fh: %x", fh);
      if (fh != INVALID_HANDLE_VALUE)
      {
         unlink(fd.cFileName);
         do
         {
            // deb("del %s", fd.cFileName);
            unlink(fd.cFileName);
         }
         while (FindNextFile(fh, &fd));
      }
      run = true;
   }

   FILE * fp;
   char name[MAX_PATH];
   sprintf(name, "%d%s.txt", num++, tag);
   fp = fopen(name, "wb+");
   fwrite(data, size, 1, fp);
   fclose(fp);
}

wchar_t* unicode(char* src, wchar_t* dst, int maxlen)
{

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1,
      reinterpret_cast<LPWSTR>(dst), maxlen);
   return dst;
}

char * deunicode(wchar_t * src, char * dst, int maxlen)
{
   // deb("deunicode: wcslen=%d", wcslen((wchar_t*)src));
   int wclen = wcslen((wchar_t*)src);
   WideCharToMultiByte(CP_ACP, 0, (wchar_t*)src, -1, dst, maxlen, NULL, NULL);
   // deb("deunicode: strlen=%d [%s]", strlen(dst), dst);
   int slen = strlen(dst);
   if (wclen != slen)
      deb("failed deunicode, wclen=%d slen=%d", wclen, slen);
   return dst;
}

void Dump_Blocks_In_All_Heaps()

{

   // get all the heaps in the process

   HANDLE heaps[100];

   DWORD c = ::GetProcessHeaps(100, heaps);

   deb("The process has %d heaps.\n", c);

   // get the default heap and the CRT heap (both are among

   // those retrieved above)

   const HANDLE default_heap = ::GetProcessHeap();

   // const HANDLE crt_heap = (HANDLE) _crtheap;

   for (unsigned int i = 0; i < c; i++)

   {

      // query the heap attributes

      ULONG heap_info = 0;

      SIZE_T ret_size = 0;

      if (::HeapQueryInformation(heaps[i],

            HeapCompatibilityInformation,

            &heap_info,

            sizeof(heap_info),

            &ret_size))

      {

         // show the heap attributes

         switch(heap_info)

         {

            case 0:

            deb("Heap %d is a regular heap.\n", (i + 1));

            break;

            case 1:

            deb("Heap %d is a heap with look-asides (fast heap).\n", (i + 1));

            break;

            case 2:

            deb("Heap %d is a LFH (low-fragmentation) heap.\n", (i + 1));

            break;

            default:

            deb("Heap %d is of unknown type.\n", (i + 1));

            break;

         }

         if (heaps[i] == default_heap)

         {

            deb(" This the DEFAULT process heap.\n");

         }

         // if (heaps [i] == crt_heap)

         // {

         // deb (" This the heap used by the CRT.\n");

         // }

         // walk the heap and show each allocated block inside it

         // (the attributes of each entry will differ between

         // DEBUG and RELEASE builds)

         PROCESS_HEAP_ENTRY entry;

         memset(&entry, 0, sizeof(entry));

         int count = 0;

         while (::HeapWalk(heaps[i], &entry))

         {

            if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)

            {

               deb(" Allocated entry %d: size: %d, overhead: %d", ++count,
                  entry.cbData, entry.cbOverhead);

            }

         }

      }

   }

}

#ifndef FRELEASE

char * fmterr(void)
{

   // DWORD err = GetLastError();
   // SetLastError(err);

   char * lpMsgBuf = NULL;
   static char szInternal[1024] =
   {
      0
   };
   DWORD err;
   err = GetLastError();
   if (!err)
      return "no error";

   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL,
         SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL);

   // lstrcpy(szInternal,(char*) lpMsgBuf);

 //  lpMsgBuf[strlen((char*)(lpMsgBuf) - 1)] = 0x0;
 //  lpMsgBuf[strlen((char*)(lpMsgBuf) - 1)] = 0x0;

   snprintf(szInternal, sizeof(szInternal), "%s (error %d)", (char*)lpMsgBuf,
      err);
   LocalFree(lpMsgBuf);

   // szInternal[strlen(szInternal)-1]=0x0;
   return szInternal;
}

char * fmterr(DWORD err)
{
   LPVOID lpMsgBuf = NULL;
   static char szInternal[255] =
   {
      0
   };

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) & lpMsgBuf, 0, NULL);

   lstrcpy(szInternal, (char*)lpMsgBuf);
   LocalFree(lpMsgBuf);

   szInternal[strlen(szInternal) - 1] = 0x0;
   return szInternal;
}
#else

char * fmterr(void)
{
   return NULL;
}

char * fmterr(unsigned long r)
{
   return NULL;
}
#endif
