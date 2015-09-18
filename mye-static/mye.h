enum {DOCK_LEFT, DOCK_RIGHT};

typedef struct dock_window
{
	HWND hwnd;
	int where;
} dock_window;


#define MYEAPI  __declspec(dllexport)

#ifdef __cplusplus
extern "C"  {
#endif


MYEAPI bool __stdcall mye_dockwindow(HWND hwnd, int where);

#ifdef __cplusplus
}
#endif




