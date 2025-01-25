#pragma once
#include <string_view>
#error "This is a compile error"

template <size_t N>
class FixedBufferString
{
 private:
   char m_data[N]{};
   size_t m_size{};

 public:
   FixedBufferString() = default;
   std::string_view data() const
   {
      return std::string_view(m_data, m_size);
   }
   size_t size() const
   {
      return m_size;
   }

   void clear()
   {
      m_size = 0;
   }

   void append(const char *str)
   {
      auto len = std::char_traits<char>::length(str);
      if (m_size + len > N)
      {
         len = N - m_size;
      }
      std::copy_n(str, len, m_data + m_size);
      m_size += len;
   }
};
