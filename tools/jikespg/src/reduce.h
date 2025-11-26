#ifndef REDUCE_INCLUDED
#define REDUCE_INCLUDED

/***************************************************************************/
/* CONFLICT_SYMBOLS is a mapping from each state into a set of terminal    */
/* symbols on which an LALR(1) conflict was detected in the state in       */
/* question.                                                               */
/*                                                                         */
/* LA_INDEX and LA_SET are temporary look-ahead sets, each of which will   */
/* be pointed to by a GOTO action, and the associated set will be          */
/* initialized to READ_SET(S), where S is the state identified by the GOTO */
/* action in question. READ_SET(S) is a set of terminals on which an action*/
/* is defined in state S. See COMPUTE_READ for more details.               */
/* LA_TOP is used to compute the number of such sets needed.               */
/*                                                                         */
/* The boolean variable NOT_LRK is used to mark whether or not a grammar   */
/* is not LR(k) for any k. NOT_LRK is marked true when either:             */
/*    1. The grammar contains a nonterminal A such that A =>+rm A          */
/*    2. The automaton contains a cycle with each of its edges labeled     */
/*       with a nullable nonterminal.                                      */
/* (Note that these are not the only conditions under which a grammar is   */
/*  not LR(k). In fact, it is an undecidable problem.)                     */
/* The variable HIGHEST_LEVEL is used to indicate the highest number of    */
/* lookahead that was necessary to resolve all conflicts for a given       */
/* grammar. If we can detect that the grammar is not LALR(k), we set       */
/* HIGHEST_LEVEL to INFINITY.                                              */
/***************************************************************************/
extern struct node **conflict_symbols;
extern BOOLEAN_CELL *read_set,
                    *la_set;
extern int highest_level;
extern long la_top;
extern short *la_index;
extern BOOLEAN not_lrk;

#endif /* REDUCE_INCLUDED */
