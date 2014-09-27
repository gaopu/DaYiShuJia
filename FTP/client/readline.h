#ifndef _READLINE_INCLUDED
#define _READLINE_INCLUDED

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>

#define COMMAND_NUMBER 1024



/*
 * IMPLEMETATION
 */
static char* find_next_string(char* str)
{
    while (isblank(*str))
	++str;
    return str;
}


/** 
 * cut whitespace in start and end of the string,
 * keep back a whitespace between words.
 * @param string 
 * 
 * @return none
 */
void stripwhite(char* string);


/** 
 * return a string without un-needed whitespace.
 * and add it to history.
 * NOTE: should free the memory.
 * 
 * @return pointer to the start of string
 */
char* getl();


/** 
 * bind completion
 * 
 */
void initialize_readline();


/** 
 * attempt to complete on the contents of TEXT. START and END bound
 * the region of rl_line_buffer that contains the word to complete.
 * TEXT is the word to complete. We can use the entire contents of
 * rl_line_buffer in case we want to do some simple parsing. Return
 * the array of matches or NULL if there aren't any
 * @param text 
 * @param start 
 * @param end 
 * 
 * @return the matches
 */
char** shell_completion(const char* text, int start, int end);


/** 
 * generator function for command completion. STATE lets us know
 * whether to start from scratch; without any state(== 0), then
 * we start at the top of the list.
 * 
 * @param text 
 * @param state 
 * 
 * @return 
 */
char* command_generator(const char* text, int state);




#endif
