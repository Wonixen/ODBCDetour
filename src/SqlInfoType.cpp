#include "Logging.h"
#include "Platform.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

using DynamicString = std::wstring (*)();

using SqlInfoTypes = std::variant<std::wstring, SQLUSMALLINT, SQLUINTEGER, SQLULEN, DynamicString>;

namespace
{
template <typename T>
constexpr std::string ReadInfo(void *p, SQLSMALLINT size, T = T{})
{
   if constexpr (std::is_same_v<T, SQLUSMALLINT>)
   {
      return std::to_string(*static_cast<SQLUSMALLINT *>(p));
   }
   else if constexpr (std::is_same_v<T, SQLUINTEGER>)
   {
      return std::to_string(*static_cast<SQLUINTEGER *>(p));
   }
   else if constexpr (std::is_same_v<T, SQLULEN>)
   {
      return std::to_string(*static_cast<SQLULEN *>(p));
   }
   else if constexpr (std::is_same_v<T, const std::wstring &>)
   {
      std::string result;
      // size is in bytes!
      size /= 2;
#pragma message("TODO: Fix this conversion")
      std::transform(static_cast<wchar_t *>(p),
                     static_cast<wchar_t *>(p) + size, std::back_inserter(result),
                     [](wchar_t c)
                     {
                        return static_cast<char>(c);
                     });
      return result;
   }
   else if constexpr (true)
   {
      return std::string("");
      //  static_assert(false, "Unknown type");
   }
}

struct AttrDefinition
{
   int type;
   const char *name;
   SqlInfoTypes value;
};

#define ATTR_INFO(DEF, TYPE, VAL) \
   {                              \
       DEF, #DEF, static_cast<TYPE>(VAL)}

// clang-format off
//
std::wstring GetDllName()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return buffer;
}

std::vector<AttrDefinition>& GetDriverInfos()
{
    static std::vector<AttrDefinition> driverInfos =
    {
        ATTR_INFO( SQL_MAX_DRIVER_CONNECTIONS,            SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_CONCURRENT_ACTIVITIES,         SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_IDENTIFIER_LEN,                SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_CATALOG_NAME_LEN,              SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_SCHEMA_NAME_LEN,               SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_TABLE_NAME_LEN,                SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_COLUMNS_IN_GROUP_BY,           SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_COLUMNS_IN_INDEX,              SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_COLUMNS_IN_ORDER_BY,           SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_COLUMNS_IN_SELECT,             SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_COLUMNS_IN_TABLE,              SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_INDEX_SIZE,                    SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_ROW_SIZE,                      SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_STATEMENT_LEN,                 SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_TABLES_IN_SELECT,              SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_USER_NAME_LEN,                 SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,   SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_PROCEDURE_NAME_LEN,            SQLUSMALLINT, 0),
        ATTR_INFO( SQL_MAX_QUALIFIER_NAME_LEN,            SQLUSMALLINT, 0),
        ATTR_INFO(SQL_CURSOR_COMMIT_BEHAVIOR,             SQLUSMALLINT, 1),
        ATTR_INFO(SQL_CURSOR_ROLLBACK_BEHAVIOR,           SQLUSMALLINT, 1),
        ATTR_INFO(SQL_CORRELATION_NAME,                   SQLUSMALLINT, SQL_CN_ANY),
        ATTR_INFO(SQL_NON_NULLABLE_COLUMNS,               SQLUSMALLINT, SQL_NNC_NULL),
        ATTR_INFO(SQL_FILE_USAGE,                         SQLUSMALLINT, SQL_FILE_CATALOG),

        ATTR_INFO(SQL_GETDATA_EXTENSIONS,      SQLUINTEGER, 15),
        ATTR_INFO( SQL_MAX_BINARY_LITERAL_LEN, SQLUINTEGER, 0),
        ATTR_INFO( SQL_MAX_CHAR_LITERAL_LEN,   SQLUINTEGER, 0),

        ATTR_INFO(SQL_SEARCH_PATTERN_ESCAPE, std::wstring, L"\\"),
        ATTR_INFO(SQL_LIKE_ESCAPE_CLAUSE,    std::wstring, L""),
        ATTR_INFO(SQL_ACCESSIBLE_PROCEDURES, std::wstring, L"N"),
        ATTR_INFO(SQL_ACCESSIBLE_TABLES,     std::wstring, L""),
        ATTR_INFO(SQL_CATALOG_NAME,          std::wstring, L""),
        ATTR_INFO(SQL_CATALOG_NAME_SEPARATOR,std::wstring, L"."),
        ATTR_INFO(SQL_CATALOG_TERM,          std::wstring, L"BASE"),
        ATTR_INFO(SQL_COLLATION_SEQ,         std::wstring, L""),
        ATTR_INFO(SQL_COLUMN_ALIAS,          std::wstring, L""),
        ATTR_INFO(SQL_DATA_SOURCE_NAME,      std::wstring, L"ODBC Detour Database"), // this one dynamically changes
        ATTR_INFO(SQL_DATA_SOURCE_READ_ONLY, std::wstring, L"N"),           // this one dynamically changes
        ATTR_INFO(SQL_DATABASE_NAME,         std::wstring, L"jada.db"),     // this one dynamically changes
        ATTR_INFO(SQL_DBMS_NAME,             std::wstring, L""),
        ATTR_INFO(SQL_DBMS_VER,              std::wstring, L""),
        ATTR_INFO(SQL_DESCRIBE_PARAMETER,    std::wstring, L""),
        ATTR_INFO(SQL_DM_VER,                std::wstring, L""),
        ATTR_INFO(SQL_DRIVER_NAME,           std::wstring, GetDllName()), //L"ODBCDETOUR.DLL"),  // from module load ?
        ATTR_INFO(SQL_DRIVER_ODBC_VER,       std::wstring, L"03.51"),       // from registry ?
        ATTR_INFO(SQL_DRIVER_VER,            std::wstring, L""),
        ATTR_INFO(SQL_EXPRESSIONS_IN_ORDERBY,std::wstring, L""),
        ATTR_INFO(SQL_IDENTIFIER_QUOTE_CHAR, std::wstring, L"'"),
        ATTR_INFO(SQL_INTEGRITY,             std::wstring, L""),
        ATTR_INFO(SQL_KEYWORDS,              std::wstring, L""),
    };

    return driverInfos;
}

// clang-format on

// helper type for the visitor
template <class... Ts>
struct overloads : Ts...
{
   using Ts::operator()...;
};

} // namespace

std::string GetInfotypeName(SQLUSMALLINT infoType)
{
   auto &driverInfo = GetDriverInfos();

   auto it = std::find_if(driverInfo.begin(), driverInfo.end(), [infoType](const auto &info)
                          { return info.type == infoType; });
   if (it != driverInfo.end())
   {
      return it->name;
   }

   return "Unknown type: " + std::to_string(infoType);
}

// extract the value from the pointer outValue and return it as a string
//
std::string GetInfotypeValueAsString(SQLUSMALLINT infoType, SQLPOINTER outValue, SQLSMALLINT *outValueLength1)
{
   auto &driverInfo = GetDriverInfos();

   auto it = std::find_if(driverInfo.begin(), driverInfo.end(), [infoType](const auto &info)
                          { return info.type == infoType; });
   if (it != driverInfo.end())
   {
      auto len = (outValueLength1 != nullptr) ? *outValueLength1 : static_cast<SQLSMALLINT>(0);

      // clang-format off
    const auto visitor = overloads{
        [&outValue, &len](SQLUINTEGER v)         { return ReadInfo<decltype(v)>(outValue, len, v); },
        [&outValue, &len](SQLUSMALLINT v)        { return ReadInfo<decltype(v)>(outValue, len, v); },
        [&outValue, &len](SQLULEN v)             { return ReadInfo<decltype(v)>(outValue, len, v); },
        [&outValue, &len](const std::wstring& v) { return ReadInfo<decltype(v)>(outValue, len, v); },
        [&outValue, &len](DynamicString& ) { return std::string{"dynamicString"}; },
    };
      // clang-format on
      std::string res = std::visit(visitor, it->value);
      return res;
   }
   return "Unknown Value";
}

void TransferInfoIntoOutValue(SQLUSMALLINT infoType, SQLPOINTER outValue, SQLSMALLINT *outValueLength1)
{
   auto &driverInfo = GetDriverInfos();

   auto it = std::find_if(driverInfo.begin(), driverInfo.end(), [infoType](const auto &info)
                          { return info.type == infoType; });
   if (it != driverInfo.end())
   {
      auto len = (outValueLength1 != nullptr) ? *outValueLength1 : static_cast<SQLSMALLINT>(0);

      // clang-format off
    const auto visitor = overloads{
        [&outValue, &len](SQLUINTEGER v)         { *static_cast<SQLUINTEGER *>(outValue) = v; },
        [&outValue, &len](SQLUSMALLINT v)        { *static_cast<SQLUSMALLINT *>(outValue) = v; },
        [&outValue, &len](SQLULEN v)             { *static_cast<SQLULEN *>(outValue) = v; },
        [&outValue, &len](DynamicString& v) {
            std::wstring str = v();
            // size is in bytes, make it in string type
            len /= sizeof(std::wstring::value_type);
            std::copy_n(str.begin(), len, static_cast<wchar_t *>(outValue));
        }, // do nothing
        [&outValue, &len](const std::wstring& v) {
            // size is in bytes, make it in string type
            len /= sizeof(std::wstring::value_type);
            std::copy_n(v.begin(), len, static_cast<wchar_t *>(outValue));
        },
    };
      // clang-format on
      std::visit(visitor, it->value);
   }
}
