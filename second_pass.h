/* ___The first pass's library___ */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

/* ___Include___ */
#include "general_lib.h"

/* ___Define___ */
#define MAX_OPERANDS 2
#define MAX_EMPTY_CELLS 5
#define WORD_LENGTH_4_ENCODED 7 

#define LABEL_ADD_MOVE 2
#define AFTER_PRINT_MOVE 2
#define MASK 0x0003

/* ___Macros___ */
#define SP_CLOSE                   \
	do                             \
	{                              \
		if (ob != NULL)            \
			fclose(ob);            \
		if (obName != NULL)        \
			free(obName);          \
                                   \
		if (ext != NULL)           \
			fclose(ext);           \
		if (extName != NULL)       \
			free(extName);         \
                                   \
		if (ent != NULL)           \
			fclose(ent);           \
		if (entName != NULL)       \
			free(entName);         \
                                   \
		if (ip != NULL)            \
			fclose(ip);            \
		if (ipName != NULL)        \
			free(ipName);          \
                                   \
		if (baseNamePrint != NULL) \
			free(baseNamePrint);   \
                                   \
	} while (0);

/* ___Enums___ */
/* Enum defining error indices related to the second pass. */
/* prefix 'SP' indicates second pass context */
enum spErrIndex
{
	SP_ERR_LABEL_NOT_FOUND /* Could not find an expected label in the symbol list */
};

/* ___Constants___ */
const char *spErrList[] =
{
	"The following label could not be found"
};

/* ___Prototypes___*/
void print_encoded_4(short int toPrint, FILE *ob);
int write_ent(symbolNode *head, FILE *ent);
int calc_L_for_operands(ERR_DETAILS_SIG, int destAddRess, int srcAddRess, symbolNode *head, char *resNames);

#endif
