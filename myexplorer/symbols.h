#include <windows.h>
#include <DbgHelp.h>

BOOL CALLBACK SymEnumLinesProcc(PSRCCODEINFO li,PVOID UserContext);
void symshow(void);
