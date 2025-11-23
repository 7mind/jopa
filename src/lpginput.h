#ifndef lpginput_INCLUDED
#define lpginput_INCLUDED

#include "platform.h"


namespace Jikes { // Open namespace Jikes block
typedef TokenIndex TokenObject;
typedef TokenIndex Location;

inline Location Loc(TokenObject i) { return i; }


} // Close namespace Jikes block

#include "javasym.h" /* mapping of lexical symbols  */
#include "javadef.h" /* definition of parsing names */
#include "javaprs.h" /* parsing action functions    */

#endif // lpginput_INCLUDED
