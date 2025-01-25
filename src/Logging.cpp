#include "logging.h"

#include <chrono>
#include <format>
#include <fstream>
#include <optional>
#include <stdarg.h>
#include <stdlib.h>
#include <string>

#include "Platform.h"

namespace
{
// compile time log function to get the number of digits in fraction of a second ex: ms, us, ns
template <typename T>
constexpr std::enable_if_t<std::is_integral_v<T>, size_t> intlen(T param)
{
   size_t result{1U};

   while (T{} != (param /= T{10}))
      ++result;
   return result;
}

auto GetTimeFract_a = [](const std::chrono::system_clock::time_point &now)
{
   using namespace std::chrono;
   auto fract = duration_cast<std::chrono::microseconds>(now.time_since_epoch());
   fract -= duration_cast<std::chrono::seconds>(now.time_since_epoch());
   return fract;
};

std::string GetTimeFract(const std::chrono::system_clock::time_point &now)
{
   auto fract = GetTimeFract_a(now);
   return std::format("{:0{}d}", fract.count(), intlen(decltype(fract)::duration::period::den) - 1);
}

// Function to get current date and  time in nanoseconds using std::chrono
std::string GetCurrentTimestamp()
{
   const auto now = std::chrono::system_clock::now();

   auto micro = GetTimeFract(now);
   auto ltime = std::chrono::current_zone()->to_local(now);

   return std::format("{:%Y-%m-%d %X}.{}: ", ltime, micro);
}

FILE *traceFile = nullptr;

void CloselogFile()
{
   if (traceFile != nullptr)
   {
      fclose(traceFile);
   }
}

std::optional<std::string> GetHomePath()
{
   std::optional<std::string> result;
   size_t len{};
   char buf[1000];
   if (getenv_s(&len, buf, sizeof(buf), "HOMEPATH") == 0)
   {
      if (len > 0)
      {
         result.emplace(std::string(buf));
         return result;
      }
   }
   return result;
}

FILE *InitTrace()
{
   if (traceFile != nullptr)
   {
      return traceFile;
   }

   FILE *log = nullptr;
   // get null device name
   std::string path = R"(.\NUL)";
   if (auto homepath = GetHomePath(); homepath.has_value())
   {
      path = homepath.value();
      path += R"(\JadaOdbcDetour.txt)";
   }
   if (fopen_s(&log, path.c_str(), "a") != 0)
   {
      return stderr;
   }
   traceFile = log;
   atexit(CloselogFile);
   return traceFile;
}

} // namespace

void Trace(const char *fmt, ...)
{
   static FILE *log = InitTrace();
   if (log == nullptr)
   {
      return;
   }
   auto now = GetCurrentTimestamp();
   fprintf(log, "%s", now.c_str());
   va_list args;
   va_start(args, fmt);
   vfprintf(log, fmt, args);
   va_end(args);
   fprintf(log, "\n");
   fflush(log);
}

OstreamProxy::OstreamProxy(std::ofstream &out)
    : m_out(out)
{
}
OstreamProxy::~OstreamProxy()
{
   m_out << std::endl;
   m_out.flush();
}

OstreamProxy::operator std::ofstream &()
{
   auto now = GetCurrentTimestamp();
   m_out.write(now.c_str(), now.size());
   auto threadid = std::format(" {:05d},  ", GetCurrentThreadId());
   m_out.write(threadid.c_str(), threadid.size());
   return m_out;
}

std::ofstream *logFile = nullptr;
void CloseLogFile()
{
   if (logFile != nullptr)
   {
      logFile->close();
      logFile = nullptr;
   }
}

std::ofstream &log()
{
   if (logFile != nullptr)
   {
      return *logFile;
   }

   if (auto homepath = GetHomePath(); homepath.has_value())
   {
      std::string path = homepath.value() + R"(\JadaOdbcDetour2.txt)";
      static std::ofstream out(path.c_str(), std::ios::ate | std::ios::out);
      atexit(CloseLogFile);
      return out;
   }
   // could not open log file, return NUL device
   static std::ofstream nul(R"(.\NUL)", std::ios::ate | std::ios::out);
   return nul;
}
