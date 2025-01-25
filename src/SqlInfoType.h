#pragma once

std::string GetInfotypeName(SQLUSMALLINT infoType);
std::string GetInfotypeValueAsString(SQLUSMALLINT infoType, SQLPOINTER outValue, SQLSMALLINT *outValueLength1);
