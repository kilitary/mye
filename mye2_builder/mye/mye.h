// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MYE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MYE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MYE_EXPORTS
#define MYE_API __declspec(dllexport)
#else
#define MYE_API __declspec(dllimport)
#endif

// This class is exported from the mye.dll
class MYE_API Cmye {
public:
	Cmye(void);
	// TODO: add your methods here.
};

extern MYE_API int nmye;

MYE_API int fnmye(void);
