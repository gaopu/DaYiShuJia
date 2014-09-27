#include "readline.h"

char* bin_cmd_list[COMMAND_NUMBER] = {
    "cd",
    "ls",
    "put",
    "get",
	"quit",
	"mkdir",
	"pwd",
	"alias"
};
int bin_cmd_list_size = 8;	//命令的个数，添加命名要更改此处的数字


char* getl()
{
    char* ret;
    int len;

    ret = readline("[FTP-CMD]$ ");

    if (ret == NULL) {// exit
	puts("");
	exit(0);
    }

    if (ret && *ret)
	add_history(ret);

    stripwhite(ret);
   
    return ret;
}

void stripwhite(char* string)
{
    register char* start = string;
    register char* end = string;

    while (*(end = find_next_string(end)))
	while (!isblank(*start++ = *end) && *end != '\0')
	    ++end;

    if (*--start != '\0')
	*start = '\0';
}


void initialize_readline()
{
    // allow conditional parsing of the ~/.inputrc file
    rl_readline_name = "ftp-cmd";

    // tell the completer that want a crack first.
    rl_attempted_completion_function = shell_completion;
}


char** shell_completion(const char* text, int start, int end)
{
    char** matches = NULL;

    // first run
    if (start == 0)
	matches = rl_completion_matches(text, command_generator);

    return matches;
}


char* command_generator(const char* text, int state)
{
    static int command_list_index, len;
    char* name;

//    extern 

	/* if this is a new word to complete, initialize now. This
	 * includes saving the length of TEXT for efficiency, and
	 * initializing the index variable to 0.
	 */
	if (!state) {
	    command_list_index = 0;
	    len = strlen(text);
	}

    // return the next name which parially matches from the command
    // list
    while ((name = bin_cmd_list[command_list_index])) {
	command_list_index++;

	if (strncmp(name, text, len) == 0)
	    return strdup(name);
    }

    return NULL;
}
