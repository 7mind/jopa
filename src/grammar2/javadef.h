#ifndef javadef_INCLUDED
#define javadef_INCLUDED

enum {
      ERROR_CODE,
      BEFORE_CODE,
      INSERTION_CODE,
      INVALID_CODE,
      SUBSTITUTION_CODE,
      DELETION_CODE,
      MERGE_CODE,
      MISPLACED_CODE,
      SCOPE_CODE,
      MANUAL_CODE,
      SECONDARY_CODE,
      EOF_CODE,

      ERROR_SYMBOL      = 109,
      MAX_DISTANCE      = 30,
      MIN_DISTANCE      = 3,
      MAX_NAME_LENGTH   = 36,
      MAX_TERM_LENGTH   = 36,
      NUM_STATES        = 681,

      NT_OFFSET         = 111,
      BUFF_UBOUND       = 30,
      BUFF_SIZE         = 31,
      STACK_UBOUND      = 127,
      STACK_SIZE        = 128,
      SCOPE_UBOUND      = 104,
      SCOPE_SIZE        = 105,
      LA_STATE_OFFSET   = 2172,
      MAX_LA            = 1,
      NUM_RULES         = 566,
      NUM_TERMINALS     = 111,
      NUM_NON_TERMINALS = 235,
      NUM_SYMBOLS       = 346,
      START_STATE       = 622,
      EOFT_SYMBOL       = 85,
      EOLT_SYMBOL       = 85,
      ACCEPT_ACTION     = 2171,
      ERROR_ACTION      = 2172
     };


#endif /* javadef_INCLUDED */
