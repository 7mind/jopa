#ifndef tab_INCLUDED
#define tab_INCLUDED

#include "platform.h"


namespace Jopa { // Open namespace Jopa block
class Tab
{
public:
    enum { DEFAULT_TAB_SIZE = 8 };

    inline static int TabSize() { return tab_size; }
    inline static void SetTabSize(int value) { tab_size = value; }

    static int Wcslen(wchar_t *line, int start, int end);

private:
    static int tab_size;
};


} // Close namespace Jopa block

#endif // tab_INCLUDED

