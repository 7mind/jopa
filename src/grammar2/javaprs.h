#ifndef javaprs_INCLUDED
#define javaprs_INCLUDED

#undef  SCOPE_REPAIR
#undef  DEFERRED_RECOVERY
#undef  FULL_DIAGNOSIS
#define SPACE_TABLES

class LexStream;

class javaprs_table
{
public:

    static const unsigned char  rhs[];
    static const unsigned char  check_table[];
    static const unsigned char  *base_check;
    static const unsigned short lhs[];
    static const unsigned short *base_action;
    static const unsigned short default_goto[];
    static const unsigned char  term_check[];
    static const unsigned short term_action[];

    static int nt_action(int state, int sym)
    {
        return (base_check[state + sym] == sym)
                             ? base_action[state + sym]
                             : default_goto[sym];
    }

    static int t_action(int state, int sym, LexStream *stream)
    {
        return term_action[term_check[base_action[state]+sym] == sym
                               ? base_action[state] + sym
                               : base_action[state]];
    }
};

#endif /* javaprs_INCLUDED */
