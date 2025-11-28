#include "long.h"
#include "double.h"

namespace Jopa {

LongInt::LongInt(const IEEEfloat& f)
{
    *this = f.LongValue();
}

LongInt::LongInt(const IEEEdouble& d)
{
    *this = d.LongValue();
}

} // namespace Jopa
