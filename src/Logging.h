#include <fstream>

// create a class proxy for an ostream that will flush the buffer after usage
class OstreamProxy
{
 public:
   OstreamProxy(std::ofstream &out);
   ~OstreamProxy();
   operator std::ofstream &();

 private:
   std::ofstream &m_out;
};

std::ofstream &log();

#define LOG OstreamProxy(log())
