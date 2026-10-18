#include <cmath>
#include <cstdint>
#include <ostream>

struct HumanReadable{
   std::uintmax_t size {};

   private: 
      friend std::ostream& operator<<(std::ostream& os, HumanReadable hr)
       {
           int o{};
           double mantissa = hr.size;
           for (; mantissa >= 1024.; mantissa /= 1024., ++o);
           os << std::ceil(mantissa * 10.) / 10. << "BKMGTPE"[o];
           return o ? os << "B (" << hr.size << ')' : os;
       }
};
