#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"

#define NUM_CMD 16 /* number of commands */
#define MAX_ARG_LEN 4 /* maximum length of command */
#define NUM_ADD_MTH 4 /* number of addressing methods */
#define A_Z 26

typedef struct cmd {
	int hashName; /* hash value of the name of command */
	int opCode; /* opCode of command, defined in p. 34 */
	int funct; /* funct of command, defined in p. 34. 0 if no funct */
	unsigned int firstOpAdd: 4; /* address methods for first operand, 0-3 */
	unsigned int secOpAdd : 4; /* address methods for second operand, 0-3 */
} cmd;

static cmd legalTable[NUM_CMD];

/* hashCmdName - calculate hash value of command name, hash function is sum of cmd[i]*26^i
 * INPUT: a string (should be a command name)
 * OUTPUT: command hash value. -1 if the input is invalid (too long or contains invalid chars)
 */
static int hashCmdName(char *cmd) {
	int hashValue = 0, i;

	if (strlen(cmd) > MAX_ARG_LEN) /* cmd is too long, doesn't fit to any cmd */
		return -1;

	for (i = 0; cmd[i]; i++) {
		if (!islower(cmd[i])) /* check that the character is valid */
			return -1;
		hashValue += (cmd[i] - 'a')*pow(A_Z, i); /* calculate hash value: sum of cmd[i]*26^i */
	}

	return hashValue;
}


/* initLegalTable - init the legal table (all data stored in file)
 * INPUT: no input. read data from file in known name
 * OUTPUT: init the legal table
 */
int initLegalTable() {
	int i, j; /* indexes */
	FILE *fd; /* file descriptor */
	char *tempName = (char*)calloc(sizeof(char), MAX_ARG_LEN + 1); /* read cmd name */
	char *tempOP1 = (char*)calloc(sizeof(char), NUM_ADD_MTH + 1); /* read valid address method for op1 */
	char *tempOP2 = (char*)calloc(sizeof(char), NUM_ADD_MTH + 1); /* read valid address method for op2 */

	if ((fd = fopen("./legalTableData", "r")) == NULL) {
		printf("fatal error: data file of legal table not found\n");
		return -1; /* failure */
	}

	for (i = 0; i < NUM_CMD; i++) {
		/* read data from file */
		fscanf(fd, "%s %d %d %s %s", tempName, &(legalTable[i].opCode), &(legalTable[i].funct), tempOP1, tempOP2);

		/* calculate hash value of command name and store */
		legalTable[i].hashName = hashCmdName(tempName);

		/* calculate valid addressing methods. in file it is stored as series of numbers (e.g. 013)
		 * the i-th bit in legalTable.firstOpAdd indicate the validity of this addressing method
		 * the char '*' means that this has no addressing mode */
		if (tempOP1[0] == '*')
			legalTable[i].firstOpAdd = 0;
		else {
			for (j = 0; tempOP1[j]; j++)
				legalTable[i].firstOpAdd += (tempOP1[j] - '0')*pow(2,j);
		}

		/* same of second operand */
		if (tempOP2[0] == '*')
			legalTable[i].firstOpAdd = 0;
		else {
		for (j = 0; tempOP2[j]; j++)
			legalTable[i].secOpAdd += (tempOP2[j] - '0')*pow(2,j);
		}

	}
	return 0; /* success */
}

/* isCmdValid - check if command is valid and operands are valid
 * INPUT: a command (not assumed to be valid) and address method of op1, op2
 * OUTPUT: opCode if valid, -1 if invalid
 */
int isCmdValid(char *cmd, int addOP1, int addOP2) {
	int i = 0;

	/* check that command is valid */
	while ((hashCmdName(cmd) != legalTable[i].hashName) && (i++ < NUM_CMD))
		; /* look for command with same hash value */

	/* no command with same hash value found */
	if (i == NUM_CMD) {
		printf("error in %s: invalid command", cmd);
		return -1;  /* invalid command, cannot continue */
	}

	/* check that first operand is in valid addressing method */
	if (addOP1 & legalTable[i].firstOpAdd) {
		printf("error in %s: invalid addressing method for operand 1\n", cmd);
		return -1; /* invalid command */
	}

	/* check that second operand is in valid addressing method */
	if (addOP2 & legalTable[i].secOpAdd) {
		printf("error in %s: invalid addressing method for operand 2\n", cmd);
		return -1; /* invalid command */
	}

	/* return combination of opCode and funct, the caller function can calculate these values
	 * the combination proves that each return value is unique */
	return legalTable[i].opCode * NUM_CMD + legalTable[i].funct;
}