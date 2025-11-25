#include "tab.h"


namespace Jopa { // Open namespace Jopa block
int Tab::tab_size = Tab::DEFAULT_TAB_SIZE;

//
// Compute the length of a wide character string segment after expanding tabs,
// and any non-printable ASCII characters in unicode expansion mode.
//
int Tab::Wcslen(wchar_t *line, int start, int end)
{
    bool expand = Coutput.ExpandWchar();
    for (int i = start--; i <= end; i++)
    {
        if (line[i] == U_HORIZONTAL_TAB)
        {
            int offset = (i - start) - 1;
            start -= ((tab_size - 1) - offset % tab_size);
        }
        else if (expand && (line[i] < U_SPACE || line[i] > 0xFF))
        {
            start -= 5;
            assert(line[i] != U_CARRIAGE_RETURN && line[i] != U_LINE_FEED);
        }
    }

    return (end - start);
}


} // Close namespace Jopa block

