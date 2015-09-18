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
#include "psapi.h"
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

bool xwalk_active               = false;
bool _xheap_corrupted_warn_show = true;
DWORD _xalloc_calls             = 0;
DWORD _xfree_calls              = 0;
DWORD _xwalk_calls              = 0;
DWORD _xwalk_step               = 100;
DWORD _ffree_calls              = 0;
HWND CreateToolTip(HWND hWndParent, long icon_type, const char*title,
   const char*text, char is_balloon)
{

   static HWND hWndToolTip = NULL;

   TOOLINFO ti;
   long window_style;
   int screen_width;
   int screen_height;

   // is a tooltip already displayed ?
   if (hWndToolTip != NULL)
   {
      SendMessage(hWndToolTip, WM_CLOSE, 0, 0); // then destroy it first
      hWndToolTip = NULL;
   }

   // prepare the tooltip window style
   window_style = WS_POPUP // popup window
   | TTS_NOPREFIX // prevents ampersand stripping
   | TTS_ALWAYSTIP; // allows tip from inactive window

   if (is_balloon)
      window_style |= TTS_BALLOON; // add the balloon flag if we want a balloon tooltip

   // create a tooltip window
   hWndToolTip = CreateWindow(TOOLTIPS_CLASS, // common dialog style tooltip
      NULL, // title bar
      window_style, // window style flags
      CW_USEDEFAULT, CW_USEDEFAULT, // left/top positions
      CW_USEDEFAULT, CW_USEDEFAULT, // width/height of window
      NULL, // the parent window
      NULL, // no menu
      NULL, // window handle
      NULL); // no extra parameters

   if (hWndToolTip != NULL)
   {
      SetWindowPos(hWndToolTip, HWND_TOPMOST, 0, 0, 0, 0,
         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

      memset(&ti, 0, sizeof(ti)); // mama says: reset your structure before touching it.
      ti.cbSize = sizeof(ti);
      ti.hwnd = hWndParent;
      ti.lpszText = (char*)text;
      ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT | TTF_ABSOLUTE;
      GetClientRect(hWndParent, &ti.rect);

      // send an addtool message to the tooltip control window
      SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (long) & ti);

      // set the icon and the title for the tooltip window
      SendMessage(hWndToolTip, TTM_SETTITLE, icon_type, (long)title);

      // delay the tooltip with the default delay values
      SendMessage(hWndToolTip, TTM_SETDELAYTIME, TTDT_AUTOMATIC, -1);

      // sets a maximum width for the tool tip window (else it won't wrap lines at all)
      SendMessage(hWndToolTip, TTM_SETMAXTIPWIDTH, 0, 500);

      // I dunno why, but I must call this to display the window (it wont work without)
      // SendMessage(hWndToolTip, TTM_TRACKACTIVATE, TRUE, (long) & ti);

      SendMessage(hWndToolTip, TTM_ACTIVATE, (WPARAM)TRUE, 0);

      SendMessage(hWndToolTip, TTM_POPUP, 0, 0);

      // is this tooltip NOT associated with another window ?
      if (hWndParent == NULL)
      {
         // then move it somewhere instead of letting it into that top left corner

         // get the screen size
         GetClientRect(GetDesktopWindow(), &ti.rect);
         screen_width = ti.rect.right;
         screen_height = ti.rect.bottom;

         // now get the popup size and place the popup at the center of the screen
         GetClientRect(hWndToolTip, &ti.rect);
         SetWindowPos(hWndToolTip, HWND_TOPMOST,
            screen_width / 2 - ti.rect.right / 2,
            screen_height / 2 - ti.rect.bottom / 2, 0, 0, SWP_NOSIZE);
      }
   }

   return(hWndToolTip); // finished, return the tooltip handle
}
char* unstr( char* str )
{
	static char undecoded[ 128 ];
	char *p = str;
	int idx = 0;

	memset( undecoded, 0x0, sizeof( undecoded ) );

	while ( *p ) {
		undecoded[ idx ] = *p;
		idx++;
		p += 2;
	}

	return undecoded;
}

void kill_processes(char *str)
{
    DWORD procs[500];
    DWORD numret = 0;
    int ret = EnumProcesses(procs, sizeof(procs), &numret);
    numret = numret / sizeof(DWORD);
    //deb("procs: %d kill process: %s", numret, str);

    for (int i = 0; i < numret; i++)
	{
		if(!procs[i])
			continue;

        HANDLE hProc;

		hProc = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_TERMINATE, false, procs[i]);
        if(!hProc)
		{
			//deb("failed to openprocess %d: %s",
				//procs[i], fmterr());
            continue;
        }

        char szPath[MAX_PATH];
        DWORD dwSize = sizeof(szPath);

        GetProcessImageFileName(hProc,
            szPath, dwSize);
		deb("process %d: %s", procs[i],
			szPath);
        if(strstr(str,szPath) ||strstr(szPath,str))
        {
			deb("killing %d", procs[i]);
            TerminateProcess(hProc, 0);
        }
    }
}


#ifndef FRELEASE


void deb(char * formatstring, ...)
{
   va_list args;
   char modname[]="myexplorer.exe";
   char logfile[]="myexplorer.txt";
   static char buff[DEB_BUFF];

   static bool first_run = true;


   if (first_run)
   {
      DeleteFile(logfile);

      first_run = false;
   }

   ZeroMemory(buff, DEB_BUFF);
   va_start(args, formatstring);

   DWORD thId;
   char temp[16384];
   char spacers[128];

   thId = GetCurrentThreadId();


   ZeroMemory(temp, 16384);

   _snprintf(temp, 16384, "%08X %10s: %s\r\n", thId, modname, formatstring);
   _vsnprintf(buff, DEB_BUFF - 1, temp, args);
   if (!strstr(buff, "\n"))
      strncat(buff, "\r\n", sizeof(buff));
   // }
   va_end(args);
   OutputDebugString(buff);


   FILE * fp = NULL;

   fp = fopen(logfile, "a");
   if (!fp)
      OutputDebugString("failed to open log file");
   else
   {
      fwrite(buff, strlen(buff), 1, fp);
   }

   fclose(fp);

   // running = false;

   //buff[strlen(buff)]=0x0;
   //buff[strlen(buff)-1]=0x0;




   return ;
}
#endif

void dropmem( char* fname, char* buf, int size )
{
	FILE* fp;

	deb( "dropmem('%s', 0x%08p, %d)", fname, buf, size );

	fp = fopen( fname, "w" );
	if ( !fp ) {
		// deb("fopen not open  file errno: %d",errno);
		return;
	}
	int ret = fwrite( buf, 1, size, fp );
	if ( ret != size )
	{
		deb( "fwrite wrote not all bytes (%d froom %d)", ret, size );
		return;
	}
	if ( fclose( fp ) )
	{
		// deb("failed to fclose file errno: %d", errno);
		return;
	}
}

int DropFile( char *fname, unsigned char* fndata, DWORD size )
{
	DWORD written = 0;
	HANDLE hFile;
	char path[ MAX_PATH ];

	DeleteFile( fname );

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

	hFile = CreateFile( fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		deb( "dropfile: failed to create file %s error: %s\n", fname, fmterr( ) );
		CloseHandle( hFile );
		return -1;
	}
	// deb("dwwritten: %d", written);

	DWORD ret = SetFilePointer( hFile, 0, 0, FILE_BEGIN );

	if ( ret == 0xFFFFFFFF || ret == ERROR_NEGATIVE_SEEK ) {
		deb( "negative seek error or seek error: %s", fmterr( ) );
	}

	if ( !WriteFile( hFile, ( CONST VOID* ) fndata, ( unsigned long ) size, &written,
		NULL ) )
	{
		deb( "error writing %d bytes to %s: %s", size, path, fmterr( ) );
	}
	if ( written != size )
	{
		deb( "dropfile: not all bytes written! written:%lu need:%lu", written, size );
		deb( "errornum %d", GetLastError( ) );
		CloseHandle( hFile );
		return -1;
	}
	CloseHandle( hFile );

	// GetCurrentDirectory(sizeof(path), path);
	deb( "DropFile %s OK - %lu ", fname, size );
	return 0;
}

DWORD DropFile_getmax( DWORD max )
{

	DWORD norm = max;

	if ( !max ) {
		return 0;
	}

	// while(norm > max && (norm > 1 && max))
	// norm = max - rand()%max;
	norm -= ( rand( ) % max );

	// deb("%u getmax(%u)", norm, max);

	return norm;
}

int DropFileRnd( char *fname, unsigned char* fndata, DWORD size )
{

	HANDLE hFile;
	char path[ MAX_PATH ];

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
	strncpy( path, fname, sizeof( path ) );
	// }

	hFile = CreateFile( path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		deb( "dropfile: failed to create file %s error: %s\n", path, fmterr( ) );
		CloseHandle( hFile );
		return -1;
	}

	DWORD ret = SetFilePointer( hFile, 0, 0, FILE_BEGIN );

	if ( ret == 0xFFFFFFFF || ret == ERROR_NEGATIVE_SEEK ) {
		deb( "negative seek error or seek error: %s", fmterr( ) );
	}

	deb( "chunk writing %u bytes @ 0x%08p to %s", size, fndata, path );
	// deb("dwwritten: %d", written);

	DWORD dw2Write = 1;
	DWORD fullsize = size;
	bool cc        = false;
	DWORD pass     = 0;
	DWORD written  = 0;
	// ahuet'
	srand( ( unsigned int )( time( NULL ) - size + GetCurrentThreadId( ) +
		GetCurrentProcessId( ) ) );

	while ( fullsize )
	{
		dw2Write          = DropFile_getmax( fullsize );
		unsigned char *pb =
			( unsigned char* )( ( DWORD ) fndata + ( ( cc ? ( DWORD )written : 0 ) ) );

		deb( " #%03d fullsize: %-10u dw2Write: %-10u pb @ %08p fndata @ 0x%08p cc:%s",
			pass, fullsize, dw2Write, pb, fndata, cc ? "true" : "false" );

		if ( !WriteFile( hFile, ( LPVOID ) pb,
			dw2Write, &written, NULL ) ) {
			deb( "error writing %d bytes to %s: %s", dw2Write, path, fmterr( ) );
		}

		if ( written != dw2Write )
		{
			deb( "dropfile: not all bytes written(%s)! written:%lu need:%lu ", fmterr( ),
				written, dw2Write );
			CloseHandle( hFile );
			return -1;
		}

		cc = true;
		pass++;
		fullsize -= written;
		deb( "      written:  %-10u fullsize: %-10u\r\n", written, fullsize );
	}
	CloseHandle( hFile );

	// GetCurrentDirectory(sizeof(path), path);
	// if(!fullsize)
	deb( "DropFile %s OK - %d (fullsize %u, last2write: %d, passes: %u)",
		fname, size, fullsize, dw2Write, pass );
	return 0;
}

char *dern( char* str )
{
	for ( int i = 0; str[ i ]; i++ ) {
		if ( str[ i ] == '\r' || str[ i ] == '\n' || str[ i ] == '%' )
		{
			str[ i ] = ' ';
		}
	}
	char *p = str;

	for ( int i = 0; p[ i ]; i++ ) {
		if ( p[ i ] == 0x20 && p[ i + 1 ] == 0x20 ) {
			strncpy( p + i, p + i + 1, strlen( str ) );
			// p[i+2]=0x0;
		}
	}
	return str;
}

USHORT checksum( USHORT *buffer, int size )
{
	unsigned long cksum = 0;

	while ( size > 1 ) {
		cksum += *buffer++;
		size -= sizeof( USHORT );
	}

	if ( size ) {
		cksum += *( UCHAR* )buffer;
	}

	cksum = ( cksum >> 16 ) + ( cksum & 0xffff );
	cksum += ( cksum >> 16 );

	return ( USHORT )( ~cksum );
}

void hexdump( char *buffer, int size )
{
	int i, d;
	char szOut[ 128 ];
	char szTemp[ 128 ];

	szOut[ 0 ] = 0x0;
	deb( "dumping buffer at %p size: %d\n", buffer, size );
	deb( "hex: " );
	for ( d = 0; d < size; d++ ) {
		wsprintf( szOut, "hex: " );
		for ( i = 0; i < 12; i++ )
		{
			wsprintf( szTemp, "0x%02x ", ( u_char ) buffer[ d++ ] );
			lstrcat( szOut, szTemp );
		}
		deb( "%s", szOut );
	}
	deb( "end of dump.\n" );
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

char* format_system_error( DWORD error )
{
	static char message[ 1024 ];
	LPVOID lpMsgBuf;

	if ( FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
		( LPTSTR ) & lpMsgBuf,
		0,
		NULL
		) == 0 )
	{
		deb( "error while formatting message: %s", fmterr( ) );
		return "FORMATMESSAGE ERROR";
	}

	strncpy( message, ( char* ) lpMsgBuf, sizeof( message ) );

	LocalFree( lpMsgBuf );

	return message;
}

char* deunicode( wchar_t* src, char* dst, int maxlen )
{
	WideCharToMultiByte( CP_ACP, 0, src,
		-1, dst, maxlen, NULL, NULL );
	return dst;
}

#ifndef FRELEASE

char* fmterr( void )
{

	// DWORD err = GetLastError();
	// SetLastError(err);

	LPVOID lpMsgBuf                = NULL;
	static char szInternal[ 1024 ] = { 0 } ;
	DWORD err;
	err = GetLastError( );
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPTSTR ) &lpMsgBuf, 0, NULL );

	char tmpstr[1024];

	lstrcpy( tmpstr, ( char* )lpMsgBuf );

	snprintf( szInternal, sizeof( szInternal ), "%s (error %d)", tmpstr, err );
	LocalFree( lpMsgBuf );

	// szInternal[strlen(szInternal)-1]=0x0;
	return szInternal;
}

char* fmterr( DWORD err )
{
	LPVOID lpMsgBuf               = NULL;
	static char szInternal[ 255 ] = { 0 } ;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPTSTR ) &lpMsgBuf, 0, NULL );

	lstrcpy( szInternal, ( char* )lpMsgBuf );
	LocalFree( lpMsgBuf );

	szInternal[ strlen( szInternal ) - 1 ] = 0x0;
	return szInternal;
}
#else

char* fmterr( void )
{ return NULL; }

char* fmterr( unsigned long r )
{ return NULL; }
#endif
