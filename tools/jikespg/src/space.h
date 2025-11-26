#ifndef SPACE_INCLUDED
#define SPACE_INCLUDED

struct new_state_type
{
    struct reduce_header_type reduce;
    short                     shift_number,
                              link,
                              thread,
                              image;
};

extern struct new_state_type *new_state_element;

extern short *shift_image,
             *real_shift_number;

extern int *term_state_index,
           *shift_check_index;

extern int shift_domain_count,
           num_terminal_states,
           check_size,
           term_check_size,
           term_action_size,
           shift_check_size;

#endif /* SPACE_INCLUDED */
