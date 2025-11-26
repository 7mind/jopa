// Originally generated using jikespg on java.g.
// Now maintained manually - the grammar tool is no longer available.

#pragma once


namespace Jopa { // Open namespace Jopa block
#define SCOPE_REPAIR
#define DEFERRED_RECOVERY
#define FULL_DIAGNOSIS
#define SPACE_TABLES

class LexStream;

class javaprs_table
{
public:
    static int original_state(int state) { return -base_check[state]; }
    static int asi(int state) { return asb[original_state(state)]; }
    static int nasi(int state) { return nasb[original_state(state)]; }
    static int in_symbol(int state) { return in_symb[original_state(state)]; }

    static const unsigned char  rhs[];
    static const   signed short check_table[];
    static const   signed short *base_check;
    static const unsigned short lhs[];
    static const unsigned short *base_action;
    static const unsigned char  term_check[];
    static const unsigned short term_action[];

    static const unsigned short asb[];
    static const unsigned char  asr[];
    static const unsigned short nasb[];
    static const unsigned short nasr[];
    static const unsigned short name_start[];
    static const unsigned char  name_length[];
    static const          char  string_buffer[];
    static const unsigned short terminal_index[];
    static const unsigned short non_terminal_index[];
    static const unsigned short scope_prefix[];
    static const unsigned short scope_suffix[];
    static const unsigned short scope_lhs[];
    static const unsigned char  scope_la[];
    static const unsigned short scope_state_set[];
    static const unsigned short scope_rhs[];
    static const unsigned short scope_state[];
    static const unsigned short in_symb[];

    static int nt_action(int state, int sym)
    {
        return base_action[state + sym];
    }

    static int t_action(int state, int sym, LexStream *)
    {
        return term_action[term_check[base_action[state]+sym] == sym
                               ? base_action[state] + sym
                               : base_action[state]];
    }
};


} // Close namespace Jopa block

