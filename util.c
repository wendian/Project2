#include <stdio.h>
#include <string.h>
#include "util.h"

/*
 * Print shell prompt. Pass the username as argument.
 */
void print_prompt(char *name)
{
	printf("%s >> ", name);
}

/*
 * Checks if the first string starts with the second. Return 1 if true.
 */
int starts_with(const char *a, const char *b)
{
	if (strncmp(a, b, strlen(b)) == 0)
		return 1;
	else
		return 0;
}


/*
 * Read a line from stdin.
 */
char *sh_read_line(void)
{
	char *line = NULL;
	ssize_t bufsize = 0;
	getline(&line, &bufsize, stdin);
	return line;
}


int is_empty(char *line)
{
	while (*line != '\0') {
		if (!isspace(*line))
			return 0;
		line++;
	}
	return 1;
}
