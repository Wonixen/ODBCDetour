// clang-format off
 #include <windows.h>
 #include <sql.h>
 #include <sqlext.h>
// for installation driver functions!
 #include <odbcinst.h>
// clang-format on

#include "Logging.h"
#include "SqlInfoType.h"

#include <array>
#include <cstdio>
#include <map>
#include <optional>
#include <print>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <variant>
#include <vector>

// #pragma warning(disable : 4100) // unreferenced formal parameter

// functions that changed in 64 bits!
// https://learn.microsoft.com/en-us/sql/odbc/reference/odbc-64-bit-information?view=sql-server-ver16

// when function takes a SQLPOINTER the buffer size will always be in byte(octet) even if the parameter is a buffer to wchar_t!
//
namespace
{

std::map<std::string, FARPROC> odbc_functions;

template <typename ProcType>
std::optional<ProcType> findProc(const std::string &FuncName)
{
   std::optional<ProcType> result;
   if (auto it = odbc_functions.find(FuncName); it != odbc_functions.end())
   {
      result.emplace(reinterpret_cast<ProcType>(it->second));
      return result;
   }
   else
   {
      return result;
   }
}

// function to load odbc functions
static bool LoadODBCFunctions(HMODULE hModule)
{
   // list of odbc functions to load
   auto functionsToLoad = {
       "ConfigDriverW",
       "ConfigDSN",
       "ConfigDSNW",
       "SQLAllocConnect",
       "SQLAllocEnv",
       "SQLAllocHandle",
       "SQLAllocStmt",
       "SQLBindCol",
       "SQLBindParameter",
       "SQLBrowseConnectW",
       "SQLBulkOperations",
       "SQLCancel",
       "SQLCancelHandle",
       "SQLCloseCursor",
       "SQLColAttributeW",
       "SQLColumnPrivilegesW",
       "SQLColumnsW",
       "SQLCompleteAsync",
       "SQLConnectW",
       "SQLCopyDesc",
       "SQLDescribeColW",
       "SQLDescribeParam",
       "SQLDisconnect",
       "SQLDriverConnectW",
       "SQLEndTran",
       "SQLExecDirectW",
       "SQLExecute",
       "SQLExtendedFetch",
       "SQLFetch",
       "SQLFetchScroll",
       "SQLFreeConnect",
       "SQLFreeEnv",
       "SQLFreeHandle",
       "SQLFreeStmt",
       "SQLGetConnectAttrW",
       "SQLGetCursorNameW",
       "SQLGetData",
       "SQLGetDescFieldW",
       "SQLGetDescRecW",
       "SQLGetDiagFieldW",
       "SQLGetDiagRecW",
       "SQLGetEnvAttr",
       "SQLGetFunctions",
       "SQLGetInfoW",
       "SQLGetStmtAttrW",
       "SQLGetTypeInfoW",
       "SQLMoreResults",
       "SQLNativeSqlW",
       "SQLNumParams",
       "SQLNumResultCols",
       "SQLParamData",
       "SQLPrepareW",
       "SQLPrimaryKeysW",
       "SQLProcedureColumnsW",
       "SQLProceduresW",
       "SQLPutData",
       "SQLRowCount",
       "SQLSetConnectAttrW",
       "SQLSetCursorNameW",
       "SQLSetDescFieldW",
       "SQLSetDescRec",
       "SQLSetEnvAttr",
       "SQLSetPos",
       "SQLSetScrollOptions",
       "SQLSetStmtAttrW",
       "SQLSpecialColumnsW",
       "SQLStatisticsW",
       "SQLTablePrivilegesW",
       "SQLTablesW"};

   for (auto name : functionsToLoad)
   {
      if (auto proc = GetProcAddress(hModule, name); proc == nullptr)
      {
         std::print(LOG, "Failed to load function: {}", name);
      }
      else
      {
         odbc_functions[name] = proc;
      }
   }
   return true;
}

static HMODULE proxiedDll = nullptr;
int proxyDllRefCount = 0;

static bool InitializeLibrary()
{
   // not first time loading the dll
   if (proxiedDll != nullptr)
   {
      proxyDllRefCount++;
      return true;
   }

#pragma message("Using hardcoded path to load the dll")
   auto dllName = LR"(C:\Program Files\Microsoft Office\root\VFS\ProgramFilesCommonX64\Microsoft Shared\Office16\ACEODBC.DLL)";
   // auto dllName = LR"(C:\Users\wonix\dev\JadaOdbc_gsl\build\x64\src\Debug\JadaOdbc.dll)";

   auto hModule = LoadLibrary(dllName);

   if (hModule == nullptr)
   {
      // std::print(LOG, "Failed to load {}", dllName);
      return false;
   }
   if (LoadODBCFunctions(hModule) == false)
   {
      FreeLibrary(hModule);
      return false;
   }
   proxyDllRefCount = 1;
   proxiedDll = hModule;
   return true;
}

void UninitializeLibrary()
{
   if (proxiedDll == nullptr)
   {
      return;
   }

   if (!FreeLibrary(proxiedDll))
   {
      return;
   }

   if (--proxyDllRefCount)
   {
      proxiedDll = nullptr;
   }
}

// template function which find a function and then call it with all params by fowarding them
template <typename ProcType, typename... Args>
auto FowardToOdbcDll(const std::string &FuncName, Args... args)
{

   if (auto proc = findProc<ProcType>(FuncName); proc)
   {
      return (*proc)(args...);
   }
   ProcType proc;
   decltype(proc(args...)) result{};
   if constexpr (std::is_same_v<decltype(result), BOOL>)
      return decltype(result){FALSE};
   else
      return decltype(result){SQL_ERROR};
}

template <typename ProcType, typename... Args>
class FowardTraceODBC
{
 public:
   FowardTraceODBC(const std::string &FuncName, Args... args)
   {
      std::print(LOG, R"(FowardTraceODBC({}, {}))", FuncName, args...);
   }
};
} // namespace

// SQLGetInfoW
// SQLSetConnectAttrW
// SQLSetConnectAttrW

SQLRETURN SQL_API SQLAllocConnect(SQLHENV environment_handle, SQLHDBC *connection_handle)
{
   std::print(LOG, R"(SQLAllocConnect({}, {}))", environment_handle, *connection_handle);
   using SQLAllocConnectPtr = SQLRETURN(SQL_API *)(SQLHENV, SQLHDBC *);
   return FowardToOdbcDll<SQLAllocConnectPtr>(__FUNCTION__, environment_handle, connection_handle);
}

SQLRETURN SQL_API SQLFreeConnect(SQLHDBC connection_handle)
{
   std::print(LOG, R"(SQLFreeConnect({}))", connection_handle);
   using SQLFreeConnectPtr = SQLRETURN(SQL_API *)(SQLHDBC);
   return FowardToOdbcDll<SQLFreeConnectPtr>(__FUNCTION__, connection_handle);
}

SQLRETURN SQL_API SQLAllocEnv(SQLHENV *environment_handle)
{
   std::print(LOG, R"(SQLAllocEnv({}))", *environment_handle);
   if (InitializeLibrary() == false)
   {
      return SQL_ERROR;
   }
   using SQLAllocEnvPtr = SQLRETURN(SQL_API *)(SQLHENV *);
   return FowardToOdbcDll<SQLAllocEnvPtr>(__FUNCTION__, environment_handle);
}

SQLRETURN SQL_API SQLFreeEnv(SQLHENV environment_handle)
{
   std::print(LOG, R"(SQLFreeEnv({}))", environment_handle);
   using SQLFreeEnvPtr = SQLRETURN(SQL_API *)(SQLHENV);
   auto result = FowardToOdbcDll<SQLFreeEnvPtr>(__FUNCTION__, environment_handle);
   UninitializeLibrary();
   return result;
}

SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT handleType, SQLHANDLE inputHandle, SQLHANDLE *outputHandle)
{
   std::print(LOG, R"(SQLAllocHandle({}, {}, {}))", handleType, inputHandle, *outputHandle);

   if (handleType == SQL_HANDLE_ENV)
   {
      if (InitializeLibrary() == false)
      {
         return SQL_ERROR;
      }
   }
   using SQLAllocHandlePtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE, SQLHANDLE *);
   auto result = FowardToOdbcDll<SQLAllocHandlePtr>(__FUNCTION__, handleType, inputHandle, outputHandle);
   std::print(LOG, R"(SQLAllocHandle({}, {}, {}) -> {})", handleType, inputHandle, *outputHandle, result);
   return result;
}

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT handleType, SQLHANDLE handle)
{
   std::print(LOG, R"(SQLFreeHandle({}, {}))", handleType, handle);

   using SQLFreeHandlePtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE);
   auto result = FowardToOdbcDll<SQLFreeHandlePtr>(__FUNCTION__, handleType, handle);
   if (handleType == SQL_HANDLE_ENV)
   {
      UninitializeLibrary();
   }
   return result;
}

SQLRETURN SQL_API SQLAllocStmt(SQLHDBC connection_handle, SQLHSTMT *statement_handle)
{
   std::print(LOG, R"(SQLAllocStmt({}, {}))", connection_handle, *statement_handle);
   using SQLAllocStmtPtr = SQLRETURN(SQL_API *)(SQLHDBC, SQLHSTMT *);
   return FowardToOdbcDll<SQLAllocStmtPtr>(__FUNCTION__, connection_handle, statement_handle);
}

SQLRETURN SQL_API SQLFreeStmt(HSTMT statement_handle, SQLUSMALLINT option)
{
   std::print(LOG, R"(SQLFreeStmt({}, {}))", statement_handle, option);

   using SQLFreeStmtPtr = SQLRETURN(SQL_API *)(HSTMT, SQLUSMALLINT);
   return FowardToOdbcDll<SQLFreeStmtPtr>(__FUNCTION__, statement_handle, option);
}

template <typename Dest, typename Src>
auto narrow_cast(Src v) -> Dest
{
   return static_cast<Dest>(v);
}

std::string ToUtf8(std::wstring_view wstr)
{
   std::string result;
   if (wstr.size() != 0)
   {
      // assume conversion will have same number of characters
      result.resize(wstr.size());
      do
      {
         int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), narrow_cast<int>(wstr.size()), result.data(), narrow_cast<int>(result.size()), nullptr, nullptr);
         result.resize(size);
         if (size <= wstr.size())
         {
            break;
         }

         result.resize(size);
      } while (true);
   }
   return result;
}

std::string ReadString(const wchar_t *str, int size)
{
   if (str == nullptr || size == 0)
   {
      return "NULL";
   }
   // must be done after testing for SQL_NTS, as SQL_NTS is -3
   if (size < 0 && size != SQL_NTS)
   {
      return "NULL";
   }

   std::wstring input;
   if (size == SQL_NTS)
   {
      input = std::wstring(str);
   }
   else
   {
      input = std::wstring{str, static_cast<size_t>(size)};
   }
   return ToUtf8(input);
}

SQLRETURN SQL_API SQLGetInfoW(SQLHDBC hdbc, SQLUSMALLINT infoType, SQLPOINTER outValue, SQLSMALLINT outValueMaxLength, SQLSMALLINT *outValueLength1)
{
   auto attrName = GetInfotypeName(infoType);

   std::print(LOG, R"(SQLGetInfoW({}, {}, {}, {}, {}))", hdbc, attrName, outValue, outValueMaxLength, (void *)outValueLength1);

   using SQLGetInfoWPtr = SQLRETURN(SQL_API *)(SQLHDBC, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT *);
   auto result = FowardToOdbcDll<SQLGetInfoWPtr>(__FUNCTION__, hdbc, infoType, outValue, outValueMaxLength, outValueLength1);
   auto value = GetInfotypeValueAsString(infoType, outValue, outValueLength1);
   std::print(LOG, R"({}({}, {}, "{}") -> {})", __FUNCTION__, hdbc, attrName, value, result);

   return result;
}

SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV hEnv, SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER valueLen)
{
   std::print(LOG, R"(SQLSetEnvAttr({}, {}, {}, {}))", hEnv, attribute, value, valueLen);

   using SQLSetEnvAttrPtr = SQLRETURN(SQL_API *)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
   auto result = FowardToOdbcDll<SQLSetEnvAttrPtr>(__FUNCTION__, hEnv, attribute, value, valueLen);
   std::print(LOG, R"(SQLSetEnvAttr({}, {}, {}, {}) -> {})", hEnv, attribute, value, valueLen, result);
   return result;
}

//
SQLRETURN SQL_API SQLSetConnectAttrW(SQLHDBC hDbc, SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER valueLen)
{
   std::print(LOG, R"(SQLSetConnectAttrW({}, {}, {}, {}))", hDbc, attribute, value, valueLen);
   using SQLSetConnectAttrWPtr = SQLRETURN(SQL_API *)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER);
   return FowardToOdbcDll<SQLSetConnectAttrWPtr>(__FUNCTION__, hDbc, attribute, value, valueLen);
}

SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT hStmt, SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER valueLen)
{
   std::print(LOG, R"(SQLSetStmtAttrW({}, {}, {}, {}))", hStmt, attribute, value, valueLen);
   using SQLSetStmtAttrWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER);
   return FowardToOdbcDll<SQLSetStmtAttrWPtr>(__FUNCTION__, hStmt, attribute, value, valueLen);
}

SQLRETURN SQL_API SQLGetEnvAttr(SQLHSTMT hEnv, SQLINTEGER attribute, SQLPOINTER outValue, SQLINTEGER outValueMaxLength, SQLINTEGER *outValueLength)
{
   std::print(LOG, R"(SQLGetEnvAttr({}, {}, {}, {}, {}))", hEnv, attribute, outValue, outValueMaxLength, *outValueLength);
   using SQLGetEnvAttrPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
   return FowardToOdbcDll<SQLGetEnvAttrPtr>(__FUNCTION__, hEnv, attribute, outValue, outValueMaxLength, outValueLength);
}

SQLRETURN SQL_API SQLGetConnectAttrW(SQLHSTMT hDbc, SQLINTEGER attribute, SQLPOINTER outValue, SQLINTEGER outValueMaxLength, SQLINTEGER *outValueLength)
{
   std::print(LOG, R"(SQLGetConnectAttrW({}, {}, {}, {}, {}))", hDbc, attribute, outValue, outValueMaxLength, *outValueLength);
   using SQLGetConnectAttrWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
   return FowardToOdbcDll<SQLGetConnectAttrWPtr>(__FUNCTION__, hDbc, attribute, outValue, outValueMaxLength, outValueLength);
}
SQLRETURN SQL_API SQLGetStmtAttrW(SQLHSTMT hStmt, SQLINTEGER attribute, SQLPOINTER outValue, SQLINTEGER outValueMaxLength, SQLINTEGER *outValueLength)
{
   // std::print(LOG, R"(SQLGetStmtAttrW({}, {}, {}, {}, {}))", hStmt, attribute, outValue, outValueMaxLength, *outValueLength);
   using SQLGetStmtAttrWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
   return FowardToOdbcDll<SQLGetStmtAttrWPtr>(__FUNCTION__, hStmt, attribute, outValue, outValueMaxLength, outValueLength);
}

SQLRETURN SQL_API SQLConnectW(SQLHDBC ConnectionHandle, SQLTCHAR *serverName, SQLSMALLINT serverLength, SQLTCHAR *UserName, SQLSMALLINT NameLength2, SQLTCHAR *Authentication, SQLSMALLINT NameLength3)
{
   // std::print(LOG, R"(SQLConnectW({}, {}, {}, {}, {}, {}))", ConnectionHandle, serverName, serverLength, UserName, NameLength2, Authentication, NameLength3);
   using SQLConnectWPtr = SQLRETURN(SQL_API *)(SQLHDBC, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLConnectWPtr>(__FUNCTION__, ConnectionHandle, serverName, serverLength, UserName, NameLength2, Authentication, NameLength3);
}

SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, SQLTCHAR *InConnectionString, SQLSMALLINT StringLength1, SQLTCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2Ptr, SQLUSMALLINT DriverCompletion)
{
   // std::print(LOG, R"(SQLDriverConnectW({}, {}, {}, {}, {}, {}, {}, {}))", ConnectionHandle, WindowHandle, InConnectionString, StringLength1, OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion);
   using SQLDriverConnectWPtr = SQLRETURN(SQL_API *)(SQLHDBC, SQLHWND, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *, SQLUSMALLINT);
   return FowardToOdbcDll<SQLDriverConnectWPtr>(__FUNCTION__, ConnectionHandle, WindowHandle, InConnectionString, StringLength1, OutConnectionString, BufferLength, StringLength2Ptr, DriverCompletion);
}

SQLRETURN SQL_API SQLPrepareW(HSTMT statement_handle, SQLTCHAR *statement_text, SQLINTEGER statement_text_size)
{
   // std::print(LOG, R"(SQLPrepareW({}, {}, {}))", statement_handle, statement_text, statement_text_size);
   using SQLPrepareWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLINTEGER);
   return FowardToOdbcDll<SQLPrepareWPtr>(__FUNCTION__, statement_handle, statement_text, statement_text_size);
}

SQLRETURN SQL_API SQLExecute(HSTMT statement_handle)
{
   std::print(LOG, R"(SQLExecute({}))", statement_handle);
   using SQLExecutePtr = SQLRETURN(SQL_API *)(HSTMT);
   return FowardToOdbcDll<SQLExecutePtr>(__FUNCTION__, statement_handle);
}

SQLRETURN SQL_API SQLExecDirectW(HSTMT statement_handle, SQLTCHAR *statement_text, SQLINTEGER statement_text_size)
{
   // std::print(LOG, R"(SQLExecDirectW({}, {}, {}))", statement_handle, statement_text, statement_text_size);
   using SQLExecDirectWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLINTEGER);
   return FowardToOdbcDll<SQLExecDirectWPtr>(__FUNCTION__, statement_handle, statement_text, statement_text_size);
}

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCountPtr)
{
   std::print(LOG, R"(SQLNumResultCols({}, {}))", StatementHandle, *ColumnCountPtr);
   using SQLNumResultColsPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLNumResultColsPtr>(__FUNCTION__, StatementHandle, ColumnCountPtr);
}

SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT statement_handle, SQLUSMALLINT column_number, SQLUSMALLINT field_identifier, SQLPOINTER out_string_value, SQLSMALLINT out_string_value_max_size, SQLSMALLINT *out_string_value_size, SQLLEN *out_num_value)
{
   std::print(LOG, R"(SQLColAttributeW({}, {}, {}, {}, {}, {}, {}))", statement_handle, column_number, field_identifier, out_string_value, out_string_value_max_size, *out_string_value_size, *out_num_value);
   using SQLColAttributeWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT *, SQLLEN *);
   return FowardToOdbcDll<SQLColAttributeWPtr>(__FUNCTION__, statement_handle, column_number, field_identifier, out_string_value, out_string_value_max_size, out_string_value_size, out_num_value);
}

SQLRETURN SQL_API SQLDescribeColW(HSTMT statement_handle, SQLUSMALLINT column_number, SQLTCHAR *out_column_name, SQLSMALLINT out_column_name_max_size, SQLSMALLINT *out_column_name_size, SQLSMALLINT *out_type, SQLULEN *out_column_size, SQLSMALLINT *out_decimal_digits, SQLSMALLINT *out_is_nullable)
{
   //   std::print(LOG, R"(SQLDescribeColW({}, {}, {}, {}, {}, {}, {}, {}, {}))", statement_handle, column_number, out_column_name, out_column_name_max_size, out_column_name_size, out_type, out_column_size, out_decimal_digits, out_is_nullable);
   using SQLDescribeColWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLUSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *, SQLSMALLINT *, SQLULEN *, SQLSMALLINT *, SQLSMALLINT *);
   return FowardToOdbcDll<SQLDescribeColWPtr>(__FUNCTION__, statement_handle, column_number, out_column_name, out_column_name_max_size, out_column_name_size, out_type, out_column_size, out_decimal_digits, out_is_nullable);
}
SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle)
{
   std::print(LOG, R"(SQLFetch({}))", StatementHandle);
   using SQLFetchPtr = SQLRETURN(SQL_API *)(SQLHSTMT);
   return FowardToOdbcDll<SQLFetchPtr>(__FUNCTION__, StatementHandle);
}
SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
   std::print(LOG, R"(SQLFetchScroll({}, {}, {}))", StatementHandle, FetchOrientation, FetchOffset);
   using SQLFetchScrollPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLSMALLINT, SQLLEN);
   return FowardToOdbcDll<SQLFetchScrollPtr>(__FUNCTION__, StatementHandle, FetchOrientation, FetchOffset);
}
SQLRETURN SQL_API SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT Col_or_Param_Num, SQLSMALLINT TargetType, SQLPOINTER TargetValuePtr, SQLLEN BufferLength, SQLLEN *StrLen_or_IndPtr)
{
   std::print(LOG, R"(SQLGetData({}, {}, {}, {}, {}, {}))", StatementHandle, Col_or_Param_Num, TargetType, TargetValuePtr, BufferLength, *StrLen_or_IndPtr);
   using SQLGetDataPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN *);
   return FowardToOdbcDll<SQLGetDataPtr>(__FUNCTION__, StatementHandle, Col_or_Param_Num, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
}
SQLRETURN SQL_API SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValuePtr, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
   std::print(LOG, R"(SQLBindCol({}, {}, {}, {}, {}, {}))", StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, *StrLen_or_Ind);
   using SQLBindColPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN *);
   return FowardToOdbcDll<SQLBindColPtr>(__FUNCTION__, StatementHandle, ColumnNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_Ind);
}
SQLRETURN SQL_API SQLRowCount(HSTMT statement_handle, SQLLEN *out_row_count)
{
   std::print(LOG, R"(SQLRowCount({}, {}))", statement_handle, *out_row_count);
   using SQLRowCountPtr = SQLRETURN(SQL_API *)(HSTMT, SQLLEN *);
   return FowardToOdbcDll<SQLRowCountPtr>(__FUNCTION__, statement_handle, out_row_count);
}
SQLRETURN SQL_API SQLMoreResults(HSTMT statement_handle)
{
   std::print(LOG, R"(SQLMoreResults({}))", statement_handle);
   using SQLMoreResultsPtr = SQLRETURN(SQL_API *)(HSTMT);
   return FowardToOdbcDll<SQLMoreResultsPtr>(__FUNCTION__, statement_handle);
}
SQLRETURN SQL_API SQLDisconnect(HDBC connection_handle)
{
   std::print(LOG, R"(SQLDisconnect({}))", connection_handle);
   using SQLDisconnectPtr = SQLRETURN(SQL_API *)(HDBC);
   return FowardToOdbcDll<SQLDisconnectPtr>(__FUNCTION__, connection_handle);
}

SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT handleType, SQLHANDLE handle, SQLSMALLINT record_number, SQLTCHAR *out_sqlstate, SQLINTEGER *out_native_error_code, SQLTCHAR *out_message, SQLSMALLINT out_message_max_size, SQLSMALLINT *out_message_size)
{
   // std::print(LOG, R"(SQLGetDiagRecW({}, {}, {}, {}, {}, {}, {}, {}))", handleType, handle, record_number, out_sqlstate, out_native_error_code, out_message, out_message_max_size, out_message_size);
   using SQLGetDiagRecWPtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLTCHAR *, SQLINTEGER *, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLGetDiagRecWPtr>(__FUNCTION__, handleType, handle, record_number, out_sqlstate, out_native_error_code, out_message, out_message_max_size, out_message_size);
}
SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT handleType, SQLHANDLE handle, SQLSMALLINT record_number, SQLSMALLINT field_id, SQLPOINTER out_message, SQLSMALLINT out_message_max_size, SQLSMALLINT *out_message_size)
{
   // std::print(LOG, R"(SQLGetDiagFieldW({}, {}, {}, {}, {}, {}, {}))", handleType, handle, record_number, field_id, out_message, out_message_max_size, out_message_size);
   using SQLGetDiagFieldWPtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLGetDiagFieldWPtr>(__FUNCTION__, handleType, handle, record_number, field_id, out_message, out_message_max_size, out_message_size);
}

SQLRETURN SQL_API SQLTablesW(SQLHSTMT StatementHandle, SQLTCHAR *CatalogName, SQLSMALLINT NameLength1, SQLTCHAR *SchemaName, SQLSMALLINT NameLength2, SQLTCHAR *TableName, SQLSMALLINT NameLength3, SQLTCHAR *TableType, SQLSMALLINT NameLength4)
{
   std::print(LOG, R"({}({}, "{}" "{}", "{}", "{}"))", __FUNCTION__, StatementHandle, ReadString(CatalogName, NameLength1), ReadString(SchemaName, NameLength2), ReadString(TableName, NameLength3), ReadString(TableType, NameLength4));
   using SQLTablesWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLTablesWPtr>(__FUNCTION__, StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
}

SQLRETURN SQL_API SQLColumnsW(SQLHSTMT StatementHandle, SQLTCHAR *CatalogName, SQLSMALLINT NameLength1, SQLTCHAR *SchemaName, SQLSMALLINT NameLength2, SQLTCHAR *TableName, SQLSMALLINT NameLength3, SQLTCHAR *ColumnName, SQLSMALLINT NameLength4)
{
   std::print(LOG, R"({}({}, "{}" "{}", "{}", "{}"))", __FUNCTION__, StatementHandle, ReadString(CatalogName, NameLength1), ReadString(SchemaName, NameLength2), ReadString(TableName, NameLength3), ReadString(ColumnName, NameLength4));
   using SQLColumnsWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLColumnsWPtr>(__FUNCTION__, StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, ColumnName, NameLength4);
}
SQLRETURN SQL_API SQLGetTypeInfoW(SQLHSTMT statement_handle, SQLSMALLINT type)
{
   std::print(LOG, R"(SQLGetTypeInfoW({}, {}))", statement_handle, type);
   using SQLGetTypeInfoWPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLSMALLINT);
   return FowardToOdbcDll<SQLGetTypeInfoWPtr>(__FUNCTION__, statement_handle, type);
}

SQLRETURN SQL_API SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParameterCountPtr)
{
   std::print(LOG, R"(SQLNumParams({}, {}))", StatementHandle, *ParameterCountPtr);
   using SQLNumParamsPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLNumParamsPtr>(__FUNCTION__, StatementHandle, ParameterCountPtr);
}

SQLRETURN SQL_API SQLNativeSqlW(HDBC connection_handle, SQLTCHAR *queryStr, SQLINTEGER query_length, SQLTCHAR *out_query, SQLINTEGER out_query_max_length, SQLINTEGER *out_query_length)
{
   // std::print(LOG, R"(SQLNativeSqlW({}, "{}", {}, "{}", {}, {}))", connection_handle, queryStr, query_length, out_query, out_query_max_length, out_query_length);
   using SQLNativeSqlWPtr = SQLRETURN(SQL_API *)(HDBC, SQLTCHAR *, SQLINTEGER, SQLTCHAR *, SQLINTEGER, SQLINTEGER *);
   return FowardToOdbcDll<SQLNativeSqlWPtr>(__FUNCTION__, connection_handle, queryStr, query_length, out_query, out_query_max_length, out_query_length);
}

SQLRETURN SQL_API SQLCloseCursor(HSTMT statement_handle)
{
   std::print(LOG, R"(SQLCloseCursor({}))", statement_handle);
   using SQLCloseCursorPtr = SQLRETURN(SQL_API *)(HSTMT);
   return FowardToOdbcDll<SQLCloseCursorPtr>(__FUNCTION__, statement_handle);
}
SQLRETURN SQL_API SQLBrowseConnectW(HDBC connection_handle, SQLTCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn, SQLTCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut)
{
   // std::print(LOG, R"(SQLBrowseConnectW({}, "{}", {}, "{}", {}, {}))", connection_handle, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
   using SQLBrowseConnectWPtr = SQLRETURN(SQL_API *)(HDBC, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLBrowseConnectWPtr>(__FUNCTION__, connection_handle, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
}
SQLRETURN SQL_API SQLCancel(SQLHSTMT StatementHandle)
{
   std::print(LOG, R"(SQLCancel({}))", StatementHandle);
   using SQLCancelPtr = SQLRETURN(SQL_API *)(SQLHSTMT);
   return FowardToOdbcDll<SQLCancelPtr>(__FUNCTION__, StatementHandle);
}
SQLRETURN SQL_API SQLGetCursorNameW(HSTMT StatementHandle, SQLTCHAR *CursorName, SQLSMALLINT BufferLength, SQLSMALLINT *NameLength)
{
   // std::print(LOG, R"(SQLGetCursorNameW({}, "{}", {}, {}))", StatementHandle, CursorName, BufferLength, NameLength);
   using SQLGetCursorNameWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *);
   return FowardToOdbcDll<SQLGetCursorNameWPtr>(__FUNCTION__, StatementHandle, CursorName, BufferLength, NameLength);
}
SQLRETURN SQL_API SQLGetFunctions(HDBC connection_handle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
   // std::print(LOG, R"(SQLGetFunctions({}, {}, {}))", connection_handle, FunctionId, Supported);
   using SQLGetFunctionsPtr = SQLRETURN(SQL_API *)(HDBC, SQLUSMALLINT, SQLUSMALLINT *);
   return FowardToOdbcDll<SQLGetFunctionsPtr>(__FUNCTION__, connection_handle, FunctionId, Supported);
}
SQLRETURN SQL_API SQLParamData(HSTMT StatementHandle, PTR *Value)
{
   // std::print(LOG, R"(SQLParamData({}, {}))", StatementHandle, Value);
   using SQLParamDataPtr = SQLRETURN(SQL_API *)(HSTMT, PTR *);
   return FowardToOdbcDll<SQLParamDataPtr>(__FUNCTION__, StatementHandle, Value);
}
SQLRETURN SQL_API SQLPutData(HSTMT StatementHandle, PTR Data, SQLLEN StrLen_or_Ind)
{
   std::print(LOG, R"(SQLPutData({}, {}, {}))", StatementHandle, Data, StrLen_or_Ind);
   using SQLPutDataPtr = SQLRETURN(SQL_API *)(HSTMT, PTR, SQLLEN);
   return FowardToOdbcDll<SQLPutDataPtr>(__FUNCTION__, StatementHandle, Data, StrLen_or_Ind);
}
SQLRETURN SQL_API SQLSetCursorNameW(HSTMT StatementHandle, SQLTCHAR *CursorName, SQLSMALLINT NameLength)
{
   // std::print(LOG, R"(SQLSetCursorNameW({}, "{}", {}))", StatementHandle, CursorName, NameLength);
   using SQLSetCursorNameWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLSetCursorNameWPtr>(__FUNCTION__, StatementHandle, CursorName, NameLength);
}

SQLRETURN SQL_API SQLSpecialColumnsW(HSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLTCHAR *CatalogName, SQLSMALLINT NameLength1, SQLTCHAR *SchemaName, SQLSMALLINT NameLength2, SQLTCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
   // std::print(LOG, R"(SQLSpecialColumnsW({}, {}, "{}", {}, "{}", {}, "{}", {}, {}, {}))", StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable);
   using SQLSpecialColumnsWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLUSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
   return FowardToOdbcDll<SQLSpecialColumnsWPtr>(__FUNCTION__, StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable);
}

SQLRETURN SQL_API SQLStatisticsW(HSTMT StatementHandle, SQLTCHAR *CatalogName, SQLSMALLINT NameLength1, SQLTCHAR *SchemaName, SQLSMALLINT NameLength2, SQLTCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
   // std::print(LOG, R"(SQLStatisticsW({}, "{}", {}, "{}", {}, "{}", {}, {}, {}))", StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Unique, Reserved);
   using SQLStatisticsWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLUSMALLINT, SQLUSMALLINT);
   return FowardToOdbcDll<SQLStatisticsWPtr>(__FUNCTION__, StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Unique, Reserved);
}
SQLRETURN SQL_API SQLColumnPrivilegesW(HSTMT hstmt, SQLTCHAR *szCatalogName, SQLSMALLINT cbCatalogName, SQLTCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLTCHAR *szTableName, SQLSMALLINT cbTableName, SQLTCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
   // std::print(LOG, R"(SQLColumnPrivilegesW({}, "{}", {}, "{}", {}, "{}", {}, "{}", {}))", hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
   using SQLColumnPrivilegesWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLColumnPrivilegesWPtr>(__FUNCTION__, hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
}

SQLRETURN SQL_API SQLDescribeParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT *DataTypePtr, SQLULEN *ParameterSizePtr, SQLSMALLINT *DecimalDigitsPtr, SQLSMALLINT *NullablePtr)
{
   std::print(LOG, R"(SQLDescribeParam({}, {}, {}, {}, {}, {}))", StatementHandle, ParameterNumber, *DataTypePtr, *ParameterSizePtr, *DecimalDigitsPtr, *NullablePtr);
   using SQLDescribeParamPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT *, SQLULEN *, SQLSMALLINT *, SQLSMALLINT *);
   return FowardToOdbcDll<SQLDescribeParamPtr>(__FUNCTION__, StatementHandle, ParameterNumber, DataTypePtr, ParameterSizePtr, DecimalDigitsPtr, NullablePtr);
}
SQLRETURN SQL_API SQLExtendedFetch(SQLHSTMT StatementHandle, SQLUSMALLINT FetchOrientation, SQLLEN FetchOffset, SQLULEN *RowCountPtr, SQLUSMALLINT *RowStatusArray)
{
   std::print(LOG, R"(SQLExtendedFetch({}, {}, {}, {}, {}))", StatementHandle, FetchOrientation, FetchOffset, *RowCountPtr, *RowStatusArray);
   using SQLExtendedFetchPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLLEN, SQLULEN *, SQLUSMALLINT *);
   return FowardToOdbcDll<SQLExtendedFetchPtr>(__FUNCTION__, StatementHandle, FetchOrientation, FetchOffset, RowCountPtr, RowStatusArray);
}
SQLRETURN SQL_API SQLPrimaryKeysW(HSTMT hstmt, SQLTCHAR *szCatalogName, SQLSMALLINT cbCatalogName, SQLTCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLTCHAR *szTableName, SQLSMALLINT cbTableName)
{
   // std::print(LOG, R"(SQLPrimaryKeysW({}, "{}", {}, "{}", {}, "{}", {}))", hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
   using SQLPrimaryKeysWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLPrimaryKeysWPtr>(__FUNCTION__, hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}

SQLRETURN SQL_API SQLProcedureColumnsW(HSTMT hstmt, SQLTCHAR *szCatalogName, SQLSMALLINT cbCatalogName, SQLTCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLTCHAR *szProcName, SQLSMALLINT cbProcName, SQLTCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
   // std::print(LOG, R"(SQLProcedureColumnsW({}, "{}", {}, "{}", {}, "{}", {}, "{}", {}))", hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName, szColumnName, cbColumnName);
   using SQLProcedureColumnsWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLProcedureColumnsWPtr>(__FUNCTION__, hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName, szColumnName, cbColumnName);
}
SQLRETURN SQL_API SQLProceduresW(HSTMT hstmt, SQLTCHAR *szCatalogName, SQLSMALLINT cbCatalogName, SQLTCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLTCHAR *szProcName, SQLSMALLINT cbProcName)
{
   // std::print(LOG, R"(SQLProceduresW({}, "{}", {}, "{}", {}, "{}", {}))", hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
   using SQLProceduresWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLProceduresWPtr>(__FUNCTION__, hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
}

SQLRETURN SQL_API SQLSetPos(HSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
   std::print(LOG, R"(SQLSetPos({}, {}, {}, {}))", hstmt, irow, fOption, fLock);
   using SQLSetPosPtr = SQLRETURN(SQL_API *)(HSTMT, SQLSETPOSIROW, SQLUSMALLINT, SQLUSMALLINT);
   return FowardToOdbcDll<SQLSetPosPtr>(__FUNCTION__, hstmt, irow, fOption, fLock);
}

SQLRETURN SQL_API SQLTablePrivilegesW(HSTMT hstmt, SQLTCHAR *szCatalogName, SQLSMALLINT cbCatalogName, SQLTCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLTCHAR *szTableName, SQLSMALLINT cbTableName)
{
   // std::print(LOG, R"(SQLTablePrivilegesW({}, "{}", {}, "{}", {}, "{}", {}))", hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
   using SQLTablePrivilegesWPtr = SQLRETURN(SQL_API *)(HSTMT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT);
   return FowardToOdbcDll<SQLTablePrivilegesWPtr>(__FUNCTION__, hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}
SQLRETURN SQL_API SQLBindParameter(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT InputOutputType, SQLSMALLINT ValueType, SQLSMALLINT ParameterType, SQLULEN ColumnSize, SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValuePtr, SQLLEN BufferLength, SQLLEN *StrLen_or_IndPtr)
{
   std::print(LOG, R"(SQLBindParameter({}, {}, {}, {}, {}, {}, {}, {}, {}, {}))", StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, DecimalDigits, ParameterValuePtr, BufferLength, *StrLen_or_IndPtr);
   using SQLBindParameterPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLUSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLULEN, SQLSMALLINT, SQLPOINTER, SQLLEN, SQLLEN *);
   return FowardToOdbcDll<SQLBindParameterPtr>(__FUNCTION__, StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize, DecimalDigits, ParameterValuePtr, BufferLength, StrLen_or_IndPtr);
}
SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
   std::print(LOG, R"(SQLBulkOperations({}, {}))", StatementHandle, Operation);
   using SQLBulkOperationsPtr = SQLRETURN(SQL_API *)(SQLHSTMT, SQLSMALLINT);
   return FowardToOdbcDll<SQLBulkOperationsPtr>(__FUNCTION__, StatementHandle, Operation);
}

SQLRETURN SQL_API SQLCancelHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
   std::print(LOG, R"(SQLCancelHandle({}, {}))", HandleType, Handle);
   using SQLCancelHandlePtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE);
   return FowardToOdbcDll<SQLCancelHandlePtr>(__FUNCTION__, HandleType, Handle);
}

SQLRETURN SQL_API SQLCompleteAsync(SQLSMALLINT HandleType, SQLHANDLE Handle, RETCODE *AsyncRetCodePtr)
{
   std::print(LOG, R"(SQLCompleteAsync({}, {}, {}))", HandleType, Handle, *AsyncRetCodePtr);
   using SQLCompleteAsyncPtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE, RETCODE *);
   return FowardToOdbcDll<SQLCompleteAsyncPtr>(__FUNCTION__, HandleType, Handle, AsyncRetCodePtr);
}
SQLRETURN SQL_API SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
   std::print(LOG, R"(SQLEndTran({}, {}, {}))", HandleType, Handle, CompletionType);
   using SQLEndTranPtr = SQLRETURN(SQL_API *)(SQLSMALLINT, SQLHANDLE, SQLSMALLINT);
   return FowardToOdbcDll<SQLEndTranPtr>(__FUNCTION__, HandleType, Handle, CompletionType);
}
SQLRETURN SQL_API SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr, SQLINTEGER BufferLength, SQLINTEGER *StringLengthPtr)
{
   std::print(LOG, R"(SQLGetDescFieldW({}, {}, {}, {}, {}, {}))", DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, *StringLengthPtr);
   using SQLGetDescFieldWPtr = SQLRETURN(SQL_API *)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
   return FowardToOdbcDll<SQLGetDescFieldWPtr>(__FUNCTION__, DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength, StringLengthPtr);
}
SQLRETURN SQL_API SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLTCHAR *Name, SQLSMALLINT BufferLength, SQLSMALLINT *StringLengthPtr, SQLSMALLINT *TypePtr, SQLSMALLINT *SubTypePtr, SQLLEN *LengthPtr, SQLSMALLINT *PrecisionPtr, SQLSMALLINT *ScalePtr, SQLSMALLINT *NullablePtr)
{
   // std::print(LOG, R"(SQLGetDescRecW({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}))", DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
   using SQLGetDescRecWPtr = SQLRETURN(SQL_API *)(SQLHDESC, SQLSMALLINT, SQLTCHAR *, SQLSMALLINT, SQLSMALLINT *, SQLSMALLINT *, SQLSMALLINT *, SQLLEN *, SQLSMALLINT *, SQLSMALLINT *, SQLSMALLINT *);
   return FowardToOdbcDll<SQLGetDescRecWPtr>(__FUNCTION__, DescriptorHandle, RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr, LengthPtr, PrecisionPtr, ScalePtr, NullablePtr);
}
SQLRETURN SQL_API SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier, SQLPOINTER ValuePtr, SQLINTEGER BufferLength)
{
   std::print(LOG, R"(SQLSetDescFieldW({}, {}, {}, {}, {}))", DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
   using SQLSetDescFieldWPtr = SQLRETURN(SQL_API *)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLINTEGER);
   return FowardToOdbcDll<SQLSetDescFieldWPtr>(__FUNCTION__, DescriptorHandle, RecNumber, FieldIdentifier, ValuePtr, BufferLength);
}
SQLRETURN SQL_API SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type, SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale, SQLPOINTER DataPtr, SQLLEN *StringLengthPtr, SQLLEN *IndicatorPtr)
{
   std::print(LOG, R"(SQLSetDescRec({}, {}, {}, {}, {}, {}, {}, {}, {}, {}))", DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, DataPtr, *StringLengthPtr, *IndicatorPtr);
   using SQLSetDescRecPtr = SQLRETURN(SQL_API *)(SQLHDESC, SQLSMALLINT, SQLSMALLINT, SQLSMALLINT, SQLLEN, SQLSMALLINT, SQLSMALLINT, SQLPOINTER, SQLLEN *, SQLLEN *);
   return FowardToOdbcDll<SQLSetDescRecPtr>(__FUNCTION__, DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, DataPtr, StringLengthPtr, IndicatorPtr);
}
SQLRETURN SQL_API SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
   std::print(LOG, R"(SQLCopyDesc({}, {}))", SourceDescHandle, TargetDescHandle);
   using SQLCopyDescPtr = SQLRETURN(SQL_API *)(SQLHDESC, SQLHDESC);
   return FowardToOdbcDll<SQLCopyDescPtr>(__FUNCTION__, SourceDescHandle, TargetDescHandle);
}

BOOL INSTAPI ConfigDSNW(HWND hwnd, WORD fRequest, LPCWSTR lpszDriver, LPCWSTR lpszAttributes)
{
   //   std::print(LOG, R"(ConfigDSNW({}, {}, "{}", "{}"))", (void *)hwnd, fRequest, lpszDriver, lpszAttributes);
   using ConfigDSNWPtr = BOOL(INSTAPI *)(HWND, WORD, LPCWSTR, LPCWSTR);
   return FowardToOdbcDll<ConfigDSNWPtr>(__FUNCTION__, hwnd, fRequest, lpszDriver, lpszAttributes);
}

BOOL INSTAPI ConfigDSN(HWND hwnd, WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes)
{
   std::print(LOG, R"(ConfigDSN({}, {}, "{}", "{}"))", (void *)hwnd, fRequest, lpszDriver, lpszAttributes);
   using ConfigDSNPtr = BOOL(INSTAPI *)(HWND, WORD, LPCSTR, LPCSTR);
   return FowardToOdbcDll<ConfigDSNPtr>(__FUNCTION__, hwnd, fRequest, lpszDriver, lpszAttributes);
}

BOOL INSTAPI ConfigDriverW(HWND hwnd, WORD fRequest, LPCWSTR lpszDriver, LPCWSTR lpszArgs, LPWSTR lpszMsg, WORD cbMsgMax, WORD *pcbMsgOut)
{
   //   std::print(LOG, R"(ConfigDriverW({}, {}, "{}", "{}", "{}", {}, {}))", (void *)hwnd, fRequest, lpszDriver, lpszArgs, lpszMsg, cbMsgMax, pcbMsgOut);
   using ConfigDriverWPtr = BOOL(INSTAPI *)(HWND, WORD, LPCWSTR, LPCWSTR, LPWSTR, WORD, WORD *);
   return FowardToOdbcDll<ConfigDriverWPtr>(__FUNCTION__, hwnd, fRequest, lpszDriver, lpszArgs, lpszMsg, cbMsgMax, pcbMsgOut);
}

SQLRETURN SQL_API SQLSetScrollOptions(HSTMT hstmt, SQLUSMALLINT fConcurrency, SQLLEN crowKeyset, SQLUSMALLINT crowRowset)
{
   std::print(LOG, R"(SQLSetScrollOptions({}, {}, {}, {}))", hstmt, fConcurrency, crowKeyset, crowRowset);
   using SQLSetScrollOptionsPtr = SQLRETURN(SQL_API *)(HSTMT, SQLUSMALLINT, SQLLEN, SQLUSMALLINT);
   return FowardToOdbcDll<SQLSetScrollOptionsPtr>(__FUNCTION__, hstmt, fConcurrency, crowKeyset, crowRowset);
}
