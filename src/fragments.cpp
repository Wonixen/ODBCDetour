#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

namespace
{
std::string UndecorateName(const char *decoratedName)
{
   static auto init = SymInitialize(GetCurrentProcess(), NULL, TRUE);

   char undecoratedName[1024];
   if (UnDecorateSymbolName(decoratedName, undecoratedName, sizeof(undecoratedName), UNDNAME_COMPLETE) > 0)
   {
      return undecoratedName;
   }
   return decoratedName;
}
} // namespace
