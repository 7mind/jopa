#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED

/***********************************************************************/
/***********************************************************************/
/**                                                                   **/
/**   The following declarations are specifications for all global    **/
/**   procedures and functions used in the program.                   **/
/**                                                                   **/
/***********************************************************************/
/***********************************************************************/
long temporary_space_allocated(void);
long temporary_space_used(void);
long global_space_allocated(void);
long global_space_used(void);
void reset_temporary_space(void);
void free_temporary_space(void);
void *talloc(long size);
struct node *allocate_node(char *file, long line);
BOOLEAN *allocate_boolean_array(long size, char *file, long line);
int *allocate_int_array(long size, char *file, long line);
short *allocate_short_array(long size, char *file, long line);
struct goto_header_type allocate_goto_map(int size, char *file, long line);
struct shift_header_type allocate_shift_map(int size,
                                            char *file, long line);
struct reduce_header_type allocate_reduce_map(int size,
                                              char *file, long line);
void cmprspa(void);
void cmprtim(void);
void compute_la(int state_no, int item_no, SET_PTR look_ahead);
void create_lastats(void);
void dump_tables(void);
void exit_lalrk_process(void);
void init_lalrk_process(void);
void init_rmpself(SET_PTR produces);
void itoc(int num);
void field(int num, int len);
void fill_in(char string[], int amount, char character);
void free_conflict_space(void);
void free_nodes(struct node *head, struct node *tail);
struct node *lpgaccess(int state_no, int item_no);
void mkfirst(void);
void mkrdcts(void);
void la_traverse(int state_no, int goto_indx, int *stack_top);
void remove_single_productions(void);
void mkstats(void);
void mystrcpy(char *str);
void padline(void);
void nospace(char *, long);
int number_len(int state_no);
void partset(SET_PTR collection, short *element_size, short *list,
             short *start, short *stack, int set_size);
void print_item(int item_no);
void print_large_token(char *line, char *token, char *indent, int len);
void print_state(int state_no);
void compute_action_symbols_range(short *state_start,
                                  short *state_stack,
                                  short *state_list,
                                  short *action_symbols_range);
void compute_naction_symbols_range(short *state_start,
                                   short *state_stack,
                                   short *state_list,
                                   short *naction_symbols_range);
void produce(void);
void process_error_maps(void);
void prnt_shorts(char *title, int init, int bound, int perline, short *array);
void prnt_ints(char *title, int init, int bound, int perline, int *array);
void print_space_parser(void);
void print_time_parser(void);
void process_tables(void);
void ptstats(void);
void remvsp(void);
void sortdes(short array[], short count[], int low, int high, int max);
void reallocate(void);
void resolve_conflicts(int state_no, struct node **action,
                       short *reduce_list, int reduce_root);
void restore_symbol(char *out, char *in);
char *strlwr(char *string);
char *strupr(char *string);

#endif /* HEADER_INCLUDED */
