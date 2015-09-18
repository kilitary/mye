// mye.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "mye.h"


// This is an example of an exported variable
MYE_API int nmye=0;

// This is an example of an exported function.
MYE_API int fnmye(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see mye.h for the class definition
Cmye::Cmye()
{
	return;
}
