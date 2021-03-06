#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"
#include "def.h"

#define MAX_GUIDE_LEN 6 /* maximum length of guidance ("string" is the longest) */
#define MIN(a,b) (a < b) ? (a) : (b)
#define MAXER_INT 8388608 /* 2^23 */
#define MINER_INT -8388608 /* -2^23 */
#define MAX_CHAR 126 /* MAX, MIN are range of visible characters */
#define MIN_CHAR 32
#define isVaidChar(c) ((c < MAX_CHAR) && (c > MIN_CHAR))

/* addArg - subfunction of parseCommand. Called to add argument into argv
 * INPUT: a command, indexes of the arg (argStart -> i), argv, argc
 * OUTPUT: argc value
 */
static int addArg(char* cmd, char *argv[MAX_OP_NUM], int i, int argStart, int argc);

/* flushArgv - flush argv for another use
 * INPUT: argv
 * OUTPUT: clean argv (all elements becomes empty strings)
 */
static void flushArgv(char *argv[MAX_OP_NUM]);
int lineNumber;

/* getLabel - extract a label from a line
 * INPUT: a line
 * OUTPUT: a string (char*) of the label, or NULL if there is no labal in the line
 * METHOD: return the part in the line from the first non-white character space until ':'
 */
char *getLabel(char *line) {
	char *label = calloc(MAX_LN_LEN + 1, sizeof(char));
	char *startLabel = line, *endLabel;

	if ((endLabel = strchr(line, ':')) == NULL) /* no label in line */
		return NULL;

	while ((*startLabel == ' ') || (*startLabel == '\t'))
		++startLabel;

	strncpy(label, startLabel, endLabel - startLabel); /* copy label */
	label[endLabel - startLabel] = '\0';
	return label;
}

/* getLabel - extract a label after entry or extern guide, from a line
 * INPUT: a line (after ".entry" or ".extern" guide was encountered)
 * OUTPUT: a string (char*) of the label, or NULL if there is no labal in the line
 */
char *getAnotherLabel(char *line) {
	char *label = calloc(MAX_LN_LEN + 1, sizeof(char));
	char *startLabel = line, *endLabel;

	while ((*startLabel == ' ') || (*startLabel == '\t'))
		++startLabel;

	endLabel = startLabel;
	while (!isspace(*endLabel))
		++endLabel;

	strncpy(label, startLabel, endLabel - startLabel); /* copy label */
	label[endLabel - startLabel] = '\0';
	return label;
}


/* labelValid - check validity of label
 * INPUT: a label
 * OUTPUT: validity of the label (start with a letter, contains only nums and letters, in a proper length)
 */
int labelValid(char *label) {
	int validity = 0;

	/* check if the label is in a proper length */
	if (strlen(label) > MAX_LBL_SZ) {
		printf("error in %d: label is too long, maximum length is %d", lineNumber, MAX_LBL_SZ);
		validity = ERROR_CODE;
	}

	/* check if label starts with a letter */
	if (!isalpha(*label)) {
		printf("error in %d:: label should start with a letter", lineNumber);
		validity = ERROR_CODE;
	}

	/* check if label contains only numbers and letters */
	while (*(++label))
		if (!isalnum(*label)) {
			printf("error in %d:: label should contain only letters and numbers", lineNumber);
			validity = ERROR_CODE;
		}

	return validity;
}

/* getGuideType - check if the line contain guide statement, if it is, return its type
 * INPUT: a line to parse
 * OUTPUT: type of guide (NO_GUIDE if not a guidance statement)
 */
int getGuideType(char* linePtr) {
	char *guide = calloc(MAX_GUIDE_LEN +1 , sizeof(char));
	int length = 0; /* length of guide, if exist */

	while ((*linePtr == ' ') || (*linePtr == '\t'))
		++linePtr;

	if (*(linePtr++) != '.')
		return NO_GUIDE; /* no guidance is this line */

	/* calculate length of guidance statement */
	while (!isspace(*(linePtr + length)))
		length += sizeof(char);

	strncpy(guide, linePtr, MIN(length, MAX_GUIDE_LEN));
	guide[MIN(length, MAX_GUIDE_LEN)] = '\0';

	if (!strcmp(guide, "data"))
		return DATA;
	else if (!strcmp(guide, "string"))
		return STR;
	else if (!strcmp(guide, "entry"))
		return ENTRY;
	else if (!strcmp(guide, "extern"))
		return EXTERN;

	printf("error in %d: %s: guide name is not valid\n", lineNumber, guide);
	return ERROR_CODE; /* error code */
}


/* getAddMthd - analize the operand to get its addressing method
 * INPUT: an operand (as string)
 * OUTPUT: indication of its addressing method (NON = no operand)
 */
int getAddMthd(char* op) {

	if (!strlen(op)) /* no operand */
		return NON;

	else if (op[0] == '#')
		return IMM;

	else if (op[0] == '&')
		return REL;

	else if ((op[0] == 'r') && (op[1] >= '0') && (op[1] <= '7') && (strlen(op) == 2))
		return REG;

	/* there is no specific character for DIR. if the label is not valid, error will be detected in second scan */
	return DIR;
}


/* getNumbers - extracts numbers into array of integers inside a dataNode
 * INPUT: a part of line (after ".data" guide encountered) to parse
 * OUTPUT: a node with data for success, NULL in case of error
 */
dataNode *getNumbers(char* line) {

	dataNode *node = (dataNode*) malloc(sizeof(dataNode));
	long int tempNum; /* long for big numbers */
	int i = 0, errorFlag = 0, sign = 1;
	enum isComma {NEED_COMMA, WAS_COMMA} ic = WAS_COMMA; /* enforce right use in commas */
	char *ptr;
	node->length = 1;
	node->next = NULL;
	node->type = DATA;

	/* count how many numbers should be in line */
	for (ptr = line; *ptr != '\0'; ptr++)
		node->length += (*ptr == ',');

	/* memory allocation for store the numbers */
	node->data.intPtr = calloc(node->length, sizeof(int));
	ptr = line;

	while (i < node->length) {

		while (isspace(*ptr)) /* skip white spaces */
			ptr++;

		/* right use in commas */
		if ((ic == WAS_COMMA) && (*ptr == ',')) { /* if comma already was - a comma is an error */
			printf("error in %d: multiple commas\n", lineNumber);
			errorFlag = ERROR_CODE;
			break;
		}
		else if (ic == NEED_COMMA) { /* if comma is needed - not a comma is an error */
			if (*ptr == ',') {
				ic = WAS_COMMA;
				ptr++;
			}
			else {
				printf("error in %d: comma expected but not found\n", lineNumber);
				errorFlag = ERROR_CODE;
			}
		}

		while (isspace(*ptr)) /* skip white spaces */
			ptr++;

		tempNum = 0, sign = 1; /* reset these variables to get the next number */

		if ((*ptr == '-') || (*ptr == '+')) { /* case of negetive number */
			sign = 44 - *ptr; /* if *ptr is +, sign is 1. if *ptr is -, sign is -1, beacuse + is 43 and - is 45 in ascii */
			ptr++;
		}

		while (isdigit(*ptr)) { /* convert string of number to int */
			tempNum = 10 * tempNum + (*(ptr++) - '0');
			ic = NEED_COMMA; /* ready for another comma */

			if ((tempNum > MAXER_INT) || (tempNum < MINER_INT)) { /* check every iteration to prevent overflow */
				printf("error in %d: a number does not fit the memory, should be from %d to %d\n", lineNumber, MINER_INT, MAXER_INT);
				errorFlag = ERROR_CODE;

				while (!isspace(*ptr) && !(*(ptr) == ',')) /* skip until next space or comma for prevent multiple errors*/
					ptr++;
				break;
			}
		}

		node->data.intPtr[i++] = (int) tempNum * sign;

		/* case of non-digit character in the "number" */
		if (!isdigit(*ptr) && !isspace(*ptr) && !(*ptr == ',') && !(*ptr == '\0')) {
			printf("error in %d: invalid character (%c) in number - not a digit\n", lineNumber, *ptr);
			errorFlag = ERROR_CODE;
			ic = NEED_COMMA; /* ready for another comma */
			while (!isspace(*ptr) && !(*(ptr) == ',')) /* skip until next space or comma for prevent multiple errors*/
				ptr++;
		}
	}

	/* check if there are more arguments than commas */
	if (errorFlag != ERROR_CODE) {
		while ((*ptr != '\0') && (*ptr != '\n') && (*ptr != EOF)) {
			if (!isspace(*(ptr++))) {
				printf("error in %d: comma expected but not found\n", lineNumber);
				errorFlag = ERROR_CODE;
				break;
			}
		}
	}
	else {/* free memory allocation */
		free(node->data.intPtr);
		free(node);
		return NULL;
	}

	/* if node.data is NULL, an error detected. otherwise, succeed */
	return node;
}

/* getStirng - extracts string into array of chars inside a dataNode
 * INPUT: a part of line (after ".string" guide encountered) to parse
 * OUTPUT: a node with data for success, NULL in case of error
 */
dataNode *getString(char* line) {

	dataNode *node = (dataNode*) malloc(sizeof(dataNode));
	int i = 0, errorFlag = 0;
	char *strStart = line, *strEnd = line + strlen(line); /* start points to the first on line, end points to its end */
	node->next = NULL;
	node->type = STR;

	while (isspace(*strStart)) /* skip white spaces */
		strStart++;

	/* case of no quotation mark in line at all */
	if (*strStart != '\"') {
		printf("error in %d: string should start with quotation mark\n", lineNumber);
		errorFlag = 1;
	}

	while (isspace(*(--strEnd))); /* skip white spaces */

	/* case of string no quotation mark in the end of the string */
	if (*strEnd != '\"') {
		printf("error in %d: string should end with quotation marks\n", lineNumber);
		errorFlag = 1;
	}

	if (!errorFlag) { /* if errorFlag was set, there is no need to parse the string */
		node->data.strPtr = calloc((strEnd - strStart + 1), sizeof(char)); /* stringPtr points to the first char in the string */

		/* copy the string from the line to the node */
		while (strStart + ++i != strEnd) {

			node->data.strPtr[i - 1] = strStart[i];
			if (!isVaidChar(*strStart)) { /* invalid char in string */
				printf("error in %d: invalid char in string\n", lineNumber);
				errorFlag = 1;
			}
		}
		node->data.strPtr[i - 1] = '\0'; /* end of string */
		node->length = strlen(node->data.strPtr) + 1;

		/* check for no extraneous text end of the string */
		while (*(++strEnd) != '\0') {
			if (!isspace(*strEnd)) { /* here, strEnd doesn't points to the string end, but to the rest chars in the line */
				printf("error in %d: extraneous text after end of string\n", lineNumber);
				errorFlag = 1;
				break; /* there is no reason to print multiple errors of that */
			}
		}

		if (errorFlag) /* free allocation of the string */
			free(node->data.strPtr);
	}

	if (errorFlag) {/* free allocation of the node */
		free(node);
		return NULL;
	}

	return node;
}

/* parseCommand - take command and extract the args
 * parseCommand check the command syntax (commas, no more than 3 args, right use in commas, etc.)
 * but NOT the command context (correct command to run, number of args correlate to command, types of args etc.)
 * INPUT: a command (cmd) and array to store the arguments
 * OUTPUT: argument count (argc), ERROR_CODE for invalid syntax. the arguments in argv
 */
int parseCommand(char *argv[MAX_OP_NUM], char *cmd) {

	enum inWord {OUTWORD, INWORD} iw = OUTWORD; /* mark end of arg */
	enum isComma {FORBID_COMMA, NEED_COMMA, WAS_COMMA} ic = FORBID_COMMA; /* enforce right use in commas */
	int i, argStart, argc = 0, errorFlag = 0;

	flushArgv(argv); /* flush (reset, clean) argv before another use */

	for (i = 0, argStart = 0; cmd[i] || (argc == MAX_OP_NUM ) ; ++i) {
		/* CASE: white space. DO: enter word into argv[argc++] */
		if (cmd[i] == ' ' || cmd[i] == '\t' || cmd[i] == '\n' || cmd[i] == EOF) {
			if (iw == INWORD) {
				if ((argc = addArg (cmd, argv, i, argStart, argc)) < 0)
					errorFlag = 1;
				ic = argc > 1 ? NEED_COMMA : FORBID_COMMA;
				iw = OUTWORD;
			}
			/* CASE: end of line */
			if (cmd[i] == '\n' || cmd[i] == EOF || cmd[i] == '\0')
				break;
		}

		/* CASE: no white space or EOL and argc = MAX: extraneous text after end of command */
		else if (argc >= MAX_OP_NUM ) {
			printf("error in %d: extraneous text after end of command\n", lineNumber);
			return ERROR_CODE; /* there is no need to continue */
		}

		/* CASE: comma. DO: same as white space but check for no double commas */
		else if (cmd[i] == ',') {
			if (iw == INWORD) { /* case of end of word */

				if ( (argc = addArg(cmd, argv, i, argStart, argc)) < 0 )
					errorFlag = ERROR_CODE; /* error code. error message printed in addArg */

				ic = argc > 1 ? NEED_COMMA : FORBID_COMMA;
				iw = OUTWORD;
			}

			if (ic == FORBID_COMMA){ /* error: case of wrong comma */
				printf("error in %d: illegal comma before first operand\n", lineNumber);
				ic = WAS_COMMA;
				errorFlag = ERROR_CODE;
			}
			else if (ic == WAS_COMMA){ /* error: case of double commas */
				printf("error in %d: multiple consecutive commas\n", lineNumber);
				errorFlag = ERROR_CODE;
			}
			else if (ic == NEED_COMMA)
				ic = WAS_COMMA;

		}

		/* CASE: part of operand */
		else {
			if (ic == NEED_COMMA) {
				printf("error in %d: missing comma between operands\n", lineNumber);
				errorFlag = ERROR_CODE; /* error code */
			}
			if (iw == OUTWORD) {
				iw = INWORD;
				argStart = i; /* the first char of the next arg */
			}
		}
	}

	if (ic == WAS_COMMA) {
		printf("error in %d: extraneous text after end of command\n", lineNumber);
		errorFlag = ERROR_CODE;
	}

	return errorFlag;
}


/* addArg - subfunction of parseCommand. Called to add argument into argv
 * INPUT: a command, indexes of the arg (argStart -> i), argv, argc
 * OUTPUT: argc value
 */
static int addArg(char* cmd, char *argv[MAX_OP_NUM ], int i, int argStart, int argc) {

	strncpy(argv[argc], &cmd[argStart], i - argStart);
	argv[argc++][i - argStart] = '\0';
	return argc;
}

/* flushArgv - flush argv for another use
 * INPUT: argv
 * OUTPUT: clean argv (all elements becomes empty strings)
 */
static void flushArgv(char *argv[MAX_OP_NUM]) {

	int i;
	for (i = 0; i < MAX_OP_NUM; i++)
		*argv[i] = '\0';
}

/* printArgv - aid function (not in use). print argv - usefull for debugging
 * INPUT: argv
 * OUTPUT: print argv
 */
void printArgv(char *argv[MAX_OP_NUM]) {
	int i = 0;
	while (i < MAX_OP_NUM)
		printf("%s, ", argv[i++]);

	putchar('\n');
}
