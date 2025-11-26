#ifndef lpgprs_INCLUDED
#define lpgprs_INCLUDED


#undef  SCOPE_REPAIR
#undef  DEFERRED_RECOVERY
#undef  FULL_DIAGNOSIS
#define SPACE_TABLES

#define original_state(state) (-base_check[state])
#define asi(state)            asb[original_state(state)]
#define nasi(state)           nasb[original_state(state)]
#define in_symbol(state)      in_symb[original_state(state)]

extern const unsigned char  rhs[];
extern const unsigned short lhs[];
extern const unsigned short *base_action;
extern const unsigned char  term_check[];
extern const unsigned short term_action[];

#define nt_action(state, sym) base_action[state + sym]

#define t_action(state, sym, next_tok) \
  term_action[term_check[base_action[state]+sym] == sym ? \
          base_action[state] + sym : base_action[state]]


#endif /* lpgprs_INCLUDED */
