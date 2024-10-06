/* ___The first pass's library___ */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

/* ___Include___ */
#include "general_lib.h"

/* ___Define___ */
#define MAX_DIR_NAME 10
#define MAX_OC_NAME 3
#define ENTRY_FROM_DIR 1
#define ENTRY_FROM_OC 2

#define SRC_REG_MOVE 5
#define DEST_REG_MOVE 2
#define IMM_OP_MOVE 2

/* ___Macros___ */
#define FP_CLOSE            \
	do                      \
	{                       \
		if (ip != NULL)     \
			fclose(ip);     \
		if (ipName != NULL) \
			free(ipName);   \
	} while (0);

/* ___Enums___ */
enum opType
{
	opType_destOp,
	opType_srcOp
};

/* Enum defining error indices related to the first pass. */
/* prefix 'FP' indicates first pass context */
enum fpErrIndex
{
	FP_ERR_RES_NAMES,		   /* Error accessing the reserved names */
	FP_ERR_LABEL_NOT_ALLOWED,  /* There was a label definition for an element that does not allow it */
	FP_ERR_ILLEGAL_COMMA,	   /* There was an extra comma, or a comma followig the element's name */
	FP_ERR_MISSING_COMMA,	   /* There was a missing comma between two arguments */
	FP_ERR_LAST_ARG_COMMA,	   /* There was a comma following the last argument */
	FP_ERR_EXCESS_OPERANDS,	   /* Too many operands for the element type */
	FP_ERR_MISSING_OPERANDS,   /* Too little operands for the element type */
	FP_ERR_SYMB_ADD,		   /* Could not add the symbol to the linked list */
	FP_ERR_ILLEGAL_ADD_METHOD, /* There wad an operand that used an adressing method that's not allowed for the opcode */
	FP_ERR_INVALID_ENTRY,	   /* There was an invalid entry definition */
	FP_ERR_INVALID_EXTERN,	   /* There was an invalid extern definition */
	FP_ERR_INVALID_DEFINE,	   /* There was an Invlid define attempt */
	FP_ERR_INVALID_DEFINE_VAL, /* define got an invalid value */
	FP_ERR_INVALID_STRING,	   /* There was an invalid string given as data */
	FP_ERR_NL_ADD,			   /* Could not properly add a new need label node */
	FR_ERR_MISSING_DEFINE	   /* Could not find an mdefine in the time of its use */

};

/* ___Constants___ */
const char *fpErrList[] =
{
	"Could not access the reserved names list",
	"Label definition is not allowed for the element",
	"Detected an extranous comma",
	"Detected a missing comma between arguments",
	"A comma following the last argument is not allowed",
	"Too many operands for the element",
	"Missing operands for the element",
	"Unsuccessful symbol addition attempt for",
	"The following operand's addressing method is not allowed for this opcode",
	"Could not define the following symbol as an entry",
	"Could not define the following symbol as an extern",
	"Invalid define attempt",
	"The following value is invalid as a define operand",
	"Invalid string was given as an argument",
	"Unsuccessful need label node addition attempt",
	"The following element is not a number, nor a known define"
};

/* ___Prototypes___*/
int is_valid_line(ERR_DETAILS_SIG, char *toCheck, int foundLabelFlag);
symbolNode *new_symbol(const char *stage, char *name, int type, int value, int ARE, symbolNode *head);
needLabelNode *new_need_label(const char *stage, char *name, int location, int readInLine, needLabelNode *head);
int first_pass_binary(ERR_DETAILS_SIG, short int codeImage[], int *IC, Opcodes currentOc,
					  char *destOp, char *srcOp, symbolNode *head, needLabelNode **needLHead, char *resNames);
int build_operand_word(ERR_DETAILS_SIG, short int codeImage[], int *IC, char *currentOp,
					   int currentAddRes, int opType, symbolNode *head, needLabelNode **needLHead);
int process_opcode(ERR_DETAILS_SIG, Opcodes currentOc, char *currentLine, int foundLabelFlag,
				   symbolNode *head, needLabelNode **needLHead, char *resNames, short int codeImage[], int *IC);
int process_dir(ERR_DETAILS_SIG, Directives currentDir, char *currentLine, int foundLabelFlag,
				symbolNode **symbHead, char *resNames, short int **dataImage, int *DC);

#endif
