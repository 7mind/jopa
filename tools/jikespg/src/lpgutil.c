static char hostfile[] = __FILE__;

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "header.h"

/**********************************************************************/
/* The following are global variables and constants used to manage a  */
/* pool of temporary space. Externally, the user invokes the function */
/* "talloc" just as he would invoke "malloc".                         */
/**********************************************************************/
#ifdef DOS
#define LOG_BLKSIZE 12
#else
#define LOG_BLKSIZE 14
#endif

#define BLKSIZE (1 << LOG_BLKSIZE)
#define BASE_INCREMENT 64

typedef long cell;

static cell **temp_base = NULL;
static long temp_top = 0,
            temp_size = 0,
            temp_base_size = 0;

/**********************************************************************/
/*                          ALLOCATE_MORE_SPACE:                      */
/**********************************************************************/
/* This procedure obtains more TEMPORARY space.                       */
/**********************************************************************/
static BOOLEAN allocate_more_space(cell ***base, long *size, long *base_size)
{
    int k;

/**********************************************************************/
/* The variable size always indicates the maximum number of cells     */
/* that has been allocated and reserved for the storage pool.         */
/* Initially, size should be set to 0 to indicate that no space has   */
/* yet been allocated. The pool of cells available is divided into    */
/* segments of size 2**LOG_BLKSIZE each and each segment is pointer   */
/* to by a slot in the array base.                                    */
/*                                                                    */
/* By dividing "size" by the size of the segment we obtain the        */
/* index for the next segment in base. If base is already full, it is */
/* reallocated.                                                       */
/*                                                                    */
/**********************************************************************/
    k = (*size) >> LOG_BLKSIZE; /* which segment? */
    if (k == (*base_size))      /* base overflow? reallocate */
    {
        register int i = (*base_size);

        (*base_size) += BASE_INCREMENT;
        (*base) = (cell **)
                  ((*base) == NULL ?
                   malloc(sizeof(cell *) * (*base_size)) :
                   realloc((*base), sizeof(cell *) * (*base_size)));
        if ((*base) == (cell **) NULL)
            return FALSE;

        for (i = i; i < (*base_size); i++)
            (*base)[i] = NULL;
    }

/**********************************************************************/
/* If the Ast slot "k" does not already contain a segment, We try to  */
/* allocate one and place its address in (*base)[k].                  */
/* If the allocation was not successful, we terminate;                */
/* otherwise, we adjust the address in (*base)[k] so as to allow us   */
/* to index the segment directly, instead of having to perform a      */
/* subtraction for each reference. Finally, we update size.           */
/*                                                                    */
/* Finally, we set the block to zeros.                                */
/**********************************************************************/
    if ((*base)[k] == NULL)
    {
        (*base)[k] = (cell *) malloc(sizeof(cell) << LOG_BLKSIZE);
        if ((*base)[k] == (cell *) NULL)
            return FALSE;
        (*base)[k] -= (*size);
    }

    memset((void *)((*base)[k] + (*size)), 0, sizeof(cell) << LOG_BLKSIZE);
    (*size) += BLKSIZE;

    return TRUE;
}


/**********************************************************************/
/*                         RESET_TEMPORARY_SPACE:                     */
/**********************************************************************/
/* This procedure resets the temporary space already allocated so     */
/* that it can be reused before new blocks are allocated.             */
/**********************************************************************/
void reset_temporary_space(void)
{
    temp_top = 0;         /* index of next usable elemt */
    temp_size = 0;

    return;
}


/**********************************************************************/
/*                         FREE_TEMPORARY_SPACE:                      */
/**********************************************************************/
/* This procedure frees all allocated temporary space.                */
/**********************************************************************/
void free_temporary_space(void)
{
    int k;

    for (k = 0; k < temp_base_size && temp_base[k] != NULL; k++)
    {
        temp_base[k] += (k * BLKSIZE);
        ffree(temp_base[k]);
    }

    if (temp_base != NULL)
    {
        ffree(temp_base);
        temp_base = NULL;
    }

    temp_base_size = 0;
    temp_top = 0;
    temp_size = 0;

    return;
}


/**********************************************************************/
/*                                TALLOC:                             */
/**********************************************************************/
/* talloc allocates an object of size "size" in temporary space and   */
/* returns a pointer to it.                                           */
/**********************************************************************/
void *talloc(long size)
{
    long i;

    i = temp_top;
    temp_top += ((size + sizeof(cell) - 1) / sizeof(cell));
    if (temp_top > temp_size)
    {
        i = temp_size;
        temp_top = temp_size +
                   ((size + sizeof(cell) - 1) / sizeof(cell));
        if (! allocate_more_space(&temp_base, &temp_size, &temp_base_size))
        {
            temp_top = temp_size;
            return NULL;
        }
    }

    return ((void *) &(temp_base[i >> LOG_BLKSIZE] [i]));
}


/**********************************************************************/
/*                      TEMPORARY_SPACE_ALLOCATED:                    */
/**********************************************************************/
/* Return the total size of temporary space allocated.                */
/**********************************************************************/
long temporary_space_allocated(void)
{
    return ((temp_base_size * sizeof(cell **)) +
            (temp_size * sizeof(cell)));
}


/**********************************************************************/
/*                         TEMPORARY_SPACE_USED:                      */
/**********************************************************************/
/* Return the total size of temporary space used.                     */
/**********************************************************************/
long temporary_space_used(void)
{
    return (((temp_size >> LOG_BLKSIZE) * sizeof(cell **)) +
             (temp_top * sizeof(cell)));
}


/**********************************************************************/
/*                                                                    */
/* The following are global variables and constants used to manage a  */
/* pool of global space. Externally, the user invokes one of the      */
/* functions:                                                         */
/*                                                                    */
/*    ALLOCATE_NODE                                                   */
/*    ALLOCATE_GOTO_MAP                                               */
/*    ALLOCATE_SHIFT_MAP                                              */
/*    ALLOCATE_REDUCE_MAP                                             */
/*                                                                    */
/* These functions allocate space from the global pool in the same    */
/* using the function "galloc" below.                                 */
/*                                                                    */
/**********************************************************************/
static cell **global_base = NULL;
static long global_top = 0,
            global_size = 0,
            global_base_size = 0;

static struct node *node_pool = NULL;

/**********************************************************************/
/*                          PROCESS_GLOBAL_WASTE:                     */
/**********************************************************************/
/* This function is invoked when the space left in a segment is not   */
/* enough for GALLOC to allocate a requested object. Rather than      */
/* waste the space, as many NODE structures as possible are allocated */
/* in that space and stacked up in the NODE_POOL list.                */
/**********************************************************************/
static void process_global_waste(long top)
{
    struct node *p;
    long i;

    while (TRUE)
    {
        i = top;
        top += ((sizeof(struct node) + sizeof(cell) - 1) / sizeof(cell));
        if (top > global_size)
            break;
        p = (struct node *) &(global_base[i >> LOG_BLKSIZE] [i]);
        p -> next = node_pool;
        node_pool = p;
    }

    return;
}

/**********************************************************************/
/*                                GALLOC:                             */
/**********************************************************************/
/* galloc allocates an object of size "size" in global space and      */
/* returns a pointer to it. It is analoguous to "talloc", but it      */
/* is a local (static) routine that is only invoked in this file by   */
/* other more specialized routines.                                   */
/**********************************************************************/
static void *galloc(long size)
{
    long i;

    i = global_top;
    global_top += ((size + sizeof(cell) - 1) / sizeof(cell));
    if (global_top > global_size)
    {
        process_global_waste(i);
        i = global_size;
        global_top = global_size +
                     ((size + sizeof(cell) - 1) / sizeof(cell));
        if (! allocate_more_space(&global_base,
                                  &global_size, &global_base_size))
        {
            global_top = global_size;
            return NULL;
        }
    }

    return ((void *) &(global_base[i >> LOG_BLKSIZE] [i]));
}


/****************************************************************************/
/*                              ALLOCATE_NODE:                              */
/****************************************************************************/
/*   This function allocates a node structure and returns a pointer to it.  */
/* it there are nodes in the free pool, one of them is returned. Otherwise, */
/* a new node is allocated from the global storage pool.                    */
/****************************************************************************/
struct node *allocate_node(char *file, long line)
{
    struct node *p;

    p = node_pool;
    if (p != NULL)  /* is free list not empty? */
         node_pool = p -> next;
    else
    {
        p = (struct node *) galloc(sizeof(struct node));
        if (p == NULL)
            nospace(file, line);
    }

    return(p);
}


/****************************************************************************/
/*                             FREE_NODES:                                  */
/****************************************************************************/
/*  This function frees a linked list of nodes by adding them to the free   */
/* list.  Head points to head of linked list and tail to the end.           */
/****************************************************************************/
void free_nodes(struct node *head, struct node *tail)
{
    tail -> next = node_pool;
    node_pool = head;

    return;
}


/****************************************************************************/
/*                             ALLOCATE_GOTO_MAP:                           */
/****************************************************************************/
/*   This function allocates space for a goto map with "size" elements,     */
/* initializes and returns a goto header for that map. NOTE that after the  */
/* map is successfully allocated, it is offset by one element. This is      */
/* to allow the array in question to be indexed from 1..size instead of     */
/* 0..(size-1).                                                             */
/****************************************************************************/
struct goto_header_type allocate_goto_map(int size, char *file, long line)
{
    struct goto_header_type go_to;

    go_to.size = size;
    go_to.map = (struct goto_type *)
                galloc(size * sizeof(struct goto_type));
    if (go_to.map == NULL)
        nospace(file, line);
    go_to.map--;   /* map will be indexed in range 1..size */

    return(go_to);
}


/****************************************************************************/
/*                            ALLOCATE_SHIFT_MAP:                           */
/****************************************************************************/
/*   This function allocates space for a shift map with "size" elements,    */
/* initializes and returns a shift header for that map. NOTE that after the */
/* map is successfully allocated, it is offset by one element. This is      */
/* to allow the array in question to be indexed from 1..size instead of     */
/* 0..(size-1).                                                             */
/****************************************************************************/
struct shift_header_type allocate_shift_map(int size,
                                            char *file, long line)
{
    struct shift_header_type sh;

    sh.size = size;
    sh.map = (struct shift_type *)
             galloc(size * sizeof(struct shift_type));
    if (sh.map == NULL)
        nospace(file, line);
    sh.map--;   /* map will be indexed in range 1..size */

    return(sh);
}


/****************************************************************************/
/*                             ALLOCATE_REDUCE_MAP:                         */
/****************************************************************************/
/*   This function allocates space for a REDUCE map with "size"+1 elements, */
/* initializes and returns a REDUCE header for that map. The 0th element of */
/* a reduce map is used for the default reduction.                          */
/****************************************************************************/
struct reduce_header_type allocate_reduce_map(int size,
                                              char *file, long line)
{
    struct reduce_header_type red;

    red.map = (struct reduce_type *)
              galloc((size + 1) * sizeof(struct reduce_type));
    if (red.map == NULL)
        nospace(file, line);
    red.size = size;

    return(red);
}


/**********************************************************************/
/*                        GLOBAL_SPACE_ALLOCATED:                     */
/**********************************************************************/
/* Return the total size of global space allocated.                   */
/**********************************************************************/
long global_space_allocated(void)
{
    return ((global_base_size * sizeof(cell **)) +
            (global_size * sizeof(cell)));
}


/**********************************************************************/
/*                           GLOBAL_SPACE_USED:                       */
/**********************************************************************/
/* Return the total size of global space used.                        */
/**********************************************************************/
long global_space_used(void)
{
    return (((global_size >> LOG_BLKSIZE) * sizeof(cell **)) +
             (global_top * sizeof(cell)));
}


/****************************************************************************/
/*                           ALLOCATE_INT_ARRAY:                            */
/****************************************************************************/
/*   This function allocates an array of size "size" of int integers.       */
/****************************************************************************/
int *allocate_int_array(long size, char *file, long line)
{
    int *p;

    p = (int *) calloc(size, sizeof(int));
    if (p == (int *) NULL)
        nospace(file, line);

    return(&p[0]);
}


/****************************************************************************/
/*                           ALLOCATE_SHORT_ARRAY:                          */
/****************************************************************************/
/*   This function allocates an array of size "size" of short integers.     */
/****************************************************************************/
short *allocate_short_array(long size, char *file, long line)
{
    short *p;

    p = (short *) calloc(size, sizeof(short));
    if (p == (short *) NULL)
        nospace(file, line);

    return(&p[0]);
}


/****************************************************************************/
/*                           ALLOCATE_BOOLEAN_ARRAY:                        */
/****************************************************************************/
/*   This function allocates an array of size "size" of type boolean.       */
/****************************************************************************/
BOOLEAN *allocate_boolean_array(long size, char *file, long line)
{
    BOOLEAN *p;

    p = (BOOLEAN *) calloc(size, sizeof(BOOLEAN));
    if (p == (BOOLEAN *) 0)
        nospace(file, line);

    return(&p[0]);
}


/*****************************************************************************/
/*                              FILL_IN:                                     */
/*****************************************************************************/
/* FILL_IN is a subroutine that pads a buffer, STRING,  with CHARACTER a     */
/* certain AMOUNT of times.                                                  */
/*****************************************************************************/
void fill_in(char string[], int amount, char character)
{
    int i;

    for (i = 0; i <= amount; i++)
        string[i] = character;
    string[i] = '\0';

    return;
}


/*****************************************************************************/
/*                                  QCKSRT:                                  */
/*****************************************************************************/
/* QCKSRT is a quicksort algorithm that takes as arguments an array of       */
/* integers, two numbers L and H that indicate the lower and upper bound     */
/* positions in ARRAY to be sorted.                                          */
/*****************************************************************************/
static void qcksrt(short array[], int l, int h)
{
    int lower,
        upper,
        top,
        i,
        j,
        pivot,
        lostack[14],  /* A stack of size 14 can sort an array of up to */
        histack[14];  /* 2 ** 15 - 1 elements                          */

    top = 1;
    lostack[top] = l;
    histack[top] = h;
    while (top != 0)
    {
        lower = lostack[top];
        upper = histack[top--];

        while (upper > lower)
        {
            i = lower;
            pivot = array[lower];
            for (j = lower + 1; j <= upper; j++)
            {
                if (array[j] < pivot)
                {
                    array[i] = array[j];
                    i++;
                    array[j] = array[i];
                }
            }
            array[i] = pivot;

            top++;
            if (i - lower < upper - i)
            {
                lostack[top] = i + 1;
                histack[top] = upper;
                upper = i - 1;
            }
            else
            {
                histack[top] = i - 1;
                lostack[top] = lower;
                lower = i + 1;
            }
        }
    }

    return;
}


/*****************************************************************************/
/*                               NUMBER_LEN:                                 */
/*****************************************************************************/
/* NUMBER_LEN takes a state number and returns the number of digits in that  */
/* number.                                                                   */
/*****************************************************************************/
int number_len(int state_no)
{
    int num = 0;

    do
    {
        state_no /= 10;
        num++;
    }   while (state_no != 0);

    return num;
}


/*************************************************************************/
/*                            RESTORE_SYMBOL:                            */
/*************************************************************************/
/* This procedure takes two character strings as arguments: IN and OUT.  */
/* IN identifies a grammar symbol or name that is checked as to whether  */
/* or not it needs to be quoted. If so, the necessary quotes are added   */
/* as IN is copied into the space identified by OUT.                     */
/* NOTE that it is assumed that IN and OUT do not overlap each other.    */
/*************************************************************************/
void restore_symbol(char *out, char *in)
{
    int  len;

    len = strlen(in);
    if (len > 0)
    {
        if ((len == 1 && in[0] == ormark) ||
            (in[0] == escape)             ||
            (in[0] == '\'')               ||
            (in[len - 1] == '\'')         ||
            (strchr(in, ' ') != NULL &&
            (in[0] != '<' || in[len - 1] != '>')))
        {
            *(out++) = '\'';
            while(*in != '\0')
            {
                if (*in == '\'')
                    *(out++) = *in;
                *(out++) = *(in++);
            }
            *(out++) = '\'';
            *out = '\0';

            return;
        }
    }

    strcpy(out, in);

    if (out[0] == '\n')   /* one of the special grammar symbols? */
        out[0] = escape;

    return;
}


/*****************************************************************************/
/*                          PRINT_LARGE_TOKEN:                               */
/*****************************************************************************/
/* PRINT_LARGE_TOKEN generates code to print a token that may exceed the     */
/* limit of its field.  The argument are LINE which is the symbol a varying  */
/* length character string, TOKEN which is the symbol to be printed, INDENT  */
/* which is a character string to be used as an initial prefix to indent the */
/* output line, and LEN which indicates the maximum number of characters that*/
/* can be printed on a given line.  At the end of this process, LINE will    */
/* have the value of the remaining substring that can fit on the output line.*/
/* If a TOKEN is too large to be indented in a line, but not too large for   */
/* the whole line, we forget the indentation, and printed it. Otherwise, it  */
/* is "chapped up" and printed in pieces that are each indented.             */
/*****************************************************************************/
void print_large_token(char *line, char *token, char *indent, int len)
{
    int toklen;

    char temp[SYMBOL_SIZE + 1];

    toklen = strlen(token);

    if (toklen > len && toklen <= PRINT_LINE_SIZE-1)
    {
        fprintf(syslis, "\n%s", token);
        ENDPAGE_CHECK;
        token = "";
        strcpy(line,indent);
    }
    else
    {
        for (; toklen > len; toklen = strlen(temp))
        {
            memcpy(temp, token, len);
            temp[len] = '\0';
            fprintf(syslis, "\n%s",temp);
            ENDPAGE_CHECK;
            strcpy(temp, token+len + 1);
            token = temp;
        }
        strcpy(line,indent);
        strcat(line,token);
    }

    return;
}


/*****************************************************************************/
/*                                PRINT_ITEM:                                */
/*****************************************************************************/
/* PRINT_ITEM takes as parameter an ITEM_NO which it prints.                 */
/*****************************************************************************/
void print_item(int item_no)
{
    int rule_no,
        symbol,
        len,
        offset,
        i,
        k;

    char tempstr[PRINT_LINE_SIZE + 1],
         line[PRINT_LINE_SIZE + 1],
         tok[SYMBOL_SIZE + 1];

    /*********************************************************************/
    /* We first print the left hand side of the rule, leaving at least   */
    /* 5 spaces in the output line to accomodate the equivalence symbol  */
    /* "::=" surrounded by blanks on both sides.  Then, we print all the */
    /* terminal symbols in the right hand side up to but not including   */
    /* the dot symbol.                                                   */
    /*********************************************************************/

    rule_no = item_table[item_no].rule_number;
    symbol = rules[rule_no].lhs;

    restore_symbol(tok, RETRIEVE_STRING(symbol));
    len = PRINT_LINE_SIZE - 5;
    print_large_token(line, tok, "", len);
    strcat(line, " ::= ");
    i = (PRINT_LINE_SIZE / 2) - 1;
    offset = MIN(strlen(line)-1, i);
    len = PRINT_LINE_SIZE - (offset + 4);
    i = rules[rule_no].rhs;  /* symbols before dot */

    k = ((rules[rule_no].rhs + item_table[item_no].dot) - 1);
    for (; i <= k; i++)
    {
        symbol = rhs_sym[i];
        restore_symbol(tok, RETRIEVE_STRING(symbol));
        if (strlen(tok) + strlen(line) > PRINT_LINE_SIZE - 4)
        {
            fprintf(syslis,"\n%s", line);
            ENDPAGE_CHECK;
            fill_in(tempstr, offset, SPACE);
            print_large_token(line, tok, tempstr, len);
        }
        else
            strcat(line, tok);
        strcat(line, BLANK);
    }

    /*********************************************************************/
    /* We now add a DOT "." to the output line and print the remaining   */
    /* symbols in the right hand side.  If ITEM_NO is a complete item,   */
    /* we also print the rule number.                                    */
    /*********************************************************************/
    if (item_table[item_no].dot == 0 || item_table[item_no].symbol == empty)
        strcpy(tok, ".");
    else
        strcpy(tok, " .");
    strcat(line, tok);
    len = PRINT_LINE_SIZE - (offset + 1);
    for (i = rules[rule_no].rhs +
              item_table[item_no].dot;/* symbols after dot*/
          i <= rules[rule_no + 1].rhs - 1; i++)
    {
        symbol = rhs_sym[i];
        restore_symbol(tok, RETRIEVE_STRING(symbol));
        if (strlen(tok) + strlen(line) > PRINT_LINE_SIZE -1)
        {
            fprintf(syslis, "\n%s", line);
            ENDPAGE_CHECK;
            fill_in(tempstr, offset, SPACE);
            print_large_token(line, tok, tempstr, len);
        }
        else
            strcat(line, tok);
        strcat(line, BLANK);
    }
    if (item_table[item_no].symbol == empty)   /* complete item */
    {
        sprintf(tok, " (%d)", rule_no);
        if (strlen(tok) + strlen(line) > PRINT_LINE_SIZE - 1)
        {
            fprintf(syslis, "\n%s", line);
            ENDPAGE_CHECK;
            fill_in(line,offset, SPACE);
        }
        strcat(line,tok);
    }
    fprintf(syslis, "\n%s", line);
    ENDPAGE_CHECK;

    return;
}


/*****************************************************************************/
/*                               PRINT_STATE:                                */
/*****************************************************************************/
/* PRINT_STATE prints all the items in a state.  NOTE that when single       */
/* productions are eliminated, certain items that were added in a state by   */
/* CLOSURE, will no longer show up in the output.  Example: If we have the   */
/* item [A ::= .B]  in a state, and the GOTO_REDUCE generated on B has been  */
/* replaced by say the GOTO or GOTO_REDUCE of A, the item above can no longer*/
/* be retrieved, since transitions in a given state are reconstructed from   */
/* the KERNEL and ADEQUATE items of the actions in the GOTO and SHIFT maps.  */
/*****************************************************************************/
void print_state(int state_no)
{
    struct shift_header_type sh;

    struct goto_header_type  go_to;

    short *item_list;

    int kernel_size,
        i,
        n,
        item_no,
        next_state;

    BOOLEAN end_node,
            *state_seen,
            *item_seen;

    struct node *q;

    char buffer[PRINT_LINE_SIZE + 1],
         line[PRINT_LINE_SIZE + 1];

    /*********************************************************************/
    /* ITEM_SEEN is used to construct sets of items, to help avoid       */
    /* adding duplicates in a list.  Duplicates can occur because an     */
    /* item from the kernel set will either be shifted on if it is not a */
    /* complete item, or it will be a member of the Complete_items set.  */
    /* Duplicates can also occur because of the elimination of single    */
    /* productions.                                                      */
    /*********************************************************************/

    state_seen = Allocate_boolean_array(max_la_state + 1);
    item_seen  = Allocate_boolean_array(num_items + 1);
    item_list  = Allocate_short_array(num_items + 1);

/* INITIALIZATION -----------------------------------------------------------*/

    for ALL_STATES(i)
        state_seen[i] = FALSE;

    for ALL_ITEMS(i)
        item_seen[i] = FALSE;

    kernel_size = 0;

/* END OF INITIALIZATION ----------------------------------------------------*/

    i = number_len(state_no) + 8; /* 8 = length("STATE") + 2 spaces + newline*/
    fill_in(buffer, (PRINT_LINE_SIZE - i) ,'-');

    fprintf(syslis, "\n\n\nSTATE %d %s",state_no, buffer);
    output_line_no +=3;

    /*********************************************************************/
    /* Print the set of states that have transitions to STATE_NO.        */
    /*********************************************************************/
    n = 0;
    strcpy(line, "( ");

    for (end_node = ((q = in_stat[state_no]) == NULL);
         ! end_node;
         end_node = (q == in_stat[state_no]))
    {/* copy list of IN_STAT into array */
        q = q -> next;
        if (! state_seen[q -> value])
        {
            state_seen[q -> value] = TRUE;
            if (strlen(line) + number_len(q -> value) > PRINT_LINE_SIZE-2)
            {
                fprintf(syslis,"\n%s",line);
                ENDPAGE_CHECK;
                strcpy(line, "  ");
            }
            if (q -> value != 0)
            {
                sprintf(buffer, "%d ", q -> value);
                strcat(line, buffer);
            }
        }
    }
    strcat(line, ")");
    fprintf(syslis,"\n%s\n", line);
    output_line_no++;
    ENDPAGE_CHECK;

    /*********************************************************************/
    /* Add the set of kernel items to the array ITEM_LIST, and mark all  */
    /* items seen to avoid duplicates.                                   */
    /*********************************************************************/

    for (q = statset[state_no].kernel_items; q != NULL; q = q -> next)
    {
        kernel_size++;
        item_no = q -> value;
        item_list[kernel_size] = item_no;    /* add to array */
        item_seen[item_no] = TRUE;         /* Mark as "seen" */
    }

    /*********************************************************************/
    /* Add the Complete Items to the array ITEM_LIST, and mark used.     */
    /*********************************************************************/
    n = kernel_size;
    for (q = statset[state_no].complete_items; q != NULL; q = q -> next)
    {
        item_no = q -> value;
        if (! item_seen[item_no])
        {
            item_seen[item_no] = TRUE; /* Mark as "seen" */
            item_list[++n] = item_no;
        }
    }

    /*********************************************************************/
    /* Iterate over the shift map.  Shift-Reduce actions are identified  */
    /* by a negative integer that indicates the rule in question , and   */
    /* the associated item can be retrieved by indexing the array        */
    /* ADEQUATE_ITEMS at the location of the rule.  For shift-actions, we*/
    /* simply take the predecessor-items of all the items in the kernel  */
    /* of the following state.                                           */
    /* If the shift-action is a look-ahead shift, we check to see if the */
    /* look-ahead state contains shift actions, and retrieve the next    */
    /* state from one of those shift actions.                            */
    /*********************************************************************/
    sh = shift[statset[state_no].shift_number];
    for (i = 1; i <= sh.size; i++)
    {
        next_state = SHIFT_ACTION(sh, i);
        while (next_state > (int) num_states)
        {
            struct shift_header_type next_sh;

            next_sh = shift[lastats[next_state].shift_number];
            if (next_sh.size > 0)
                next_state = SHIFT_ACTION(next_sh, 1);
            else
                next_state = 0;
        }

        if (next_state == 0)
            q = NULL;
        else if (next_state < 0)
            q = adequate_item[-next_state];
        else
        {
            q = statset[next_state].kernel_items;
            if (q == NULL) /* single production state? */
                q = statset[next_state].complete_items;
        }
        for (; q != NULL; q = q -> next)
        {
            item_no = q -> value - 1;
            if (! item_seen[item_no])
            {
                item_seen[item_no] = TRUE;
                item_list[++n] = item_no;
            }
        }
    }

    /*********************************************************************/
    /* GOTOS and GOTO-REDUCES are analogous to SHIFTS and SHIFT-REDUCES. */
    /*********************************************************************/
    go_to = statset[state_no].go_to;
    for (i = 1; i <= go_to.size; i++)
    {
        if (GOTO_ACTION(go_to, i) > 0)
        {
            q = statset[GOTO_ACTION(go_to, i)].kernel_items;
            if (q == NULL)          /* single production state? */
                q = statset[GOTO_ACTION(go_to, i)].complete_items;
        }
        else
            q = adequate_item[- GOTO_ACTION(go_to, i)];

        for (; q != NULL; q = q -> next)
        {
            item_no = q -> value - 1;
            if (! item_seen[item_no])
            {
                item_seen[item_no] = TRUE;
                item_list[++n] = item_no;
            }
        }
    }

    /*********************************************************************/
    /* Print the Kernel items.  If there are any closure items, skip a   */
    /* line, sort then, then print them.  The kernel items are in sorted */
    /* order.                                                            */
    /*********************************************************************/
    for (item_no = 1; item_no <= kernel_size; item_no++)
         print_item(item_list[item_no]);
    if (kernel_size < n)
    {
        fprintf(syslis, "\n");
        ENDPAGE_CHECK;
        qcksrt(item_list, kernel_size + 1, n);
        for (item_no = kernel_size + 1; item_no <= n; item_no++)
            print_item(item_list[item_no]);
    }

    ffree(item_list);
    ffree(item_seen);
    ffree(state_seen);

    return;
}


/*****************************************************************************/
/*                                 NOSPACE:                                  */
/*****************************************************************************/
/* This procedure is invoked when a call to MALLOC, CALLOC or REALLOC fails. */
/*****************************************************************************/
void nospace(char *file_name, long line_number)
{
    fprintf(stderr,
            "*** Cannot allocate space ... LPG terminated in "
            "file %s at line %d\n", file_name, line_number);
    exit(12);
}


/************************************************************************/
/*                               STRUPR:                                */
/************************************************************************/
/* StrUpr and StrLwr.                                                   */
/* These routines set all characters in a string to upper case and lower*/
/*  case (respectively.)  These are library routines in DOS, but        */
/*  are defined here for the 370 compiler.                              */
/************************************************************************/
char *strupr(char *string)
{
    char *s;

    /*********************************************************************/
    /* While not at end of string, change all lower case to upper case.  */
    /*********************************************************************/
    for (s = string; *s != '\0'; s++)
        *s = (islower((int) *s) ? toupper((int) *s) : *s);

    return string;
}

/************************************************************************/
/*                               STRLWR:                                */
/************************************************************************/
char *strlwr(char *string)
{
    char *s;

    /*********************************************************************/
    /* While not at end of string, change all upper case to lower case.  */
    /*********************************************************************/
    for (s = string; *s != '\0'; s++)
        *s = (isupper((int) *s) ? tolower((int) *s) : *s);

    return string;
}
