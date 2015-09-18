#include <windows.h>
#include <dbghelp.h>
#include "functions.h"

extern HINSTANCE hInst;

BOOL CALLBACK SymEnumLinesProcc(PSRCCODEINFO li, PVOID UserContext)
{
   deb(" # 0x%08X %s [%s:%d] 0x%08X", li->ModBase, li->Obj, li->FileName,
      li->LineNumber, li->Address);
   return true;
}

BOOL CALLBACK enumback(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
{
   deb(" module %s @ 0x%08x", ModuleName, BaseOfDll);
   return true;
}

BOOL CALLBACK ecb(PSRCCODEINFO li, PVOID UserContext)
{
   deb(" # 0x%08X %s [%s:%d] 0x%08X", li->ModBase, li->Obj, li->FileName,
      li->LineNumber, li->Address);
   return true;
}

BOOL CALLBACK sesp(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
   deb("@0x%08X %s 0x%08X", pSymInfo->ModBase, pSymInfo->Name, pSymInfo->Address);
   return true;
}

BOOL CALLBACK esc(PSOURCEFILE pSourceFile, PVOID UserContext)
{
   deb("   pSourceFile: %s", pSourceFile);
   return true;
}

void symshow(void)
{
   DWORD symOptions = SymGetOptions();
   DWORD oldOptions = symOptions;
   //symOptions |= SYMOPT_PUBLICS_ONLY;

   //SymSetOptions(symOptions);
   SymSetOptions(SYMOPT_ALLOW_ABSOLUTE_SYMBOLS
 | SYMOPT_ALLOW_ZERO_ADDRESS | SYMOPT_AUTO_PUBLICS | SYMOPT_CASE_INSENSITIVE |
            SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_ANYTHING
 | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME |
            SYMOPT_OMAP_FIND_NEAREST);

   if (!SymInitialize(GetCurrentProcess(), NULL, true))
      deb("failed to syminit: %s", fmterr());

   API_VERSION api;
   ImagehlpApiVersionEx(&api);
   deb("Image help library version %u.%u rev %u", api.MajorVersion,
      api.MinorVersion, api.Revision);

   // if(!SymEnumLines(GetCurrentProcess(), 0, NULL, NULL, SymEnumLinesProcc, NULL))
   // deb("failed sym enum lines: %s", fmterr());

   SymEnumerateModules64(GetCurrentProcess(), enumback, NULL);

   // if(!(SymEnumSourceLines(GetCurrentProcess(), 0,
   // NULL, NULL, 0, 0, ecb, NULL)))
   // deb("failed to enum source lines: %s", fmterr());
//   DWORD p = SymLoadModule64(GetCurrentProcess(), NULL, "myexplorer",
//      NULL, 0, 0);
//   if(!p)
//      deb("failed ysmload: %s", fmterr());
//   else
//      deb("myexplorer loaded at %p", p);

   if (!SymEnumSymbols(GetCurrentProcess(), (unsigned __int64) hInst, 0, sesp, NULL))
      deb("failed to enum symbols: %s", fmterr());

}
