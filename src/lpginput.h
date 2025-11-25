#ifndef lpginput_INCLUDED
#define lpginput_INCLUDED

#include "platform.h"


namespace Jopa { // Open namespace Jopa block
typedef TokenIndex TokenObject;
typedef TokenIndex Location;

inline Location Loc(TokenObject i) { return i; }


} // Close namespace Jopa block

#include "grammar/javasym.h" /* mapping of lexical symbols  */
#include "grammar/javadef.h" /* definition of parsing names */
#include "grammar/javaprs.h" /* parsing action functions    */

#endif // lpginput_INCLUDED
