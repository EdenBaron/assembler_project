/* ___A general library to be used across all of the source files___ */
#ifndef GENERAL_LIB_H
#define GENERAL_LIB_H

/* ___Standard libraries___ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* ___Definitions___ */
/* lengths */
#define RAM_SIZE 4096
#define MAX_LINE_LENGTH 81
#define MAX_LABEL_LENGTH 31
#define EXT_LENGTH 4
#define MAX_DIR_LENGTH 10
#define MAX_OC_LENGTH 3
#define REG_NAME_LENGTH 2
#define OC_INT_BIT_LENGTH 12
#define DIR_INT_BIT_LENGTH 14
#define IMAGE_OFFSET 100
#define INT_FUNC_ERROR (MAX_DIR_NUM + 1)
#define LABEL_IS_ENTRY 2
#define LABEL_EXISTS 3

/* return values for stages */
#define SUCCESS 0
#define QUIT_UPON_ERROR -1
#define DETECT_MORE_ERRORS -2
#define FAILURE -1

/* return values for functions */
#define FUNC_ERROR -1
#define FALSE 0
#define TRUE 1

/* values for bitwise << operation */
#define OPCODE_MOVE 6
#define SRC_OP_MOVE 4
#define DEST_OP_MOVE 2
#define OP_VALUE_MOVE 2

/* other */
#define PLACEHOLDER (MAX_DIR_NUM + 1)
#define ADD_METHODS_NUM 4

#define FIRST_REG_NUM 0
#define LAST_REG_NUM 7

#define MAX_OP_NUM 2047
#define MIN_OP_NUM (-2048)

#define MAX_DIR_NUM 8191
#define MIN_DIR_NUM (-8192)

/* ___Macros____*/
#define DELIM " \t\n\r\f"
#define DELIM_WITH_COMMA " \t\n\r\f,"

#define ERR_DETAILS \
	stage, lineIndex, ipName

#define ERR_DETAILS_SIG \
	const char *stage, int lineIndex, char *ipName

#define ERROR_WO_LINE \
	fprintf(stderr, "\n>>> Error (during the %s stage): \n", stage);

#define ERROR_WITH_LINE                                            \
	do                                                             \
	{                                                              \
		ERROR_WO_LINE                                              \
		fprintf(stderr, "\"%s\", line #%d:\t", ipName, lineIndex); \
	} while (0);

/* ___Enums___ */

enum addMethod
{
	addMethod_immediate,
	addMethod_direct,
	addMethod_constInd,
	addMethod_directReg
};

enum symbolType
{
	symbolType_mdefine,
	symbolType_code,
	symbolType_data,
	symbolType_entry,
	symbolType_extern,
	symbolType_entryTemp,
	symbolType_entryDir
};

enum ARE
{
	ABSOLUTE,
	EXTERNAL,
	RELOCATABLE
};

enum elementType
{
	elementType_dir,
	elementType_oc

};

enum Element /* indices of instructions and directives */
{
	/* marking the beginning of the instructions */
	Element_mov,
	Element_cmp,
	Element_add,
	Element_sub,
	Element_not,
	Element_clr,
	Element_lea,
	Element_inc,
	Element_dec,
	Element_jmp,
	Element_bne,
	Element_red,
	Element_prn,
	Element_jsr,
	Element_rts,
	Element_hlt,
	Element_instructionEnd,
	/* marking the end of the instructions, and the beginning of the directives*/
	Element_data,
	Element_string,
	Element_entry,
	Element_extern,
	Element_define,
	Element_directiveEnd
};

/* Enum definitions for general errors of any stage */
enum generalErrIndex
{
	GEN_ERR_MALLOC,				/* Error allocating memory */
	GEN_ERR_FOPEN,				/* Error opening file */
	GEN_ERR_UNKNOWN_ELEMENT,	/* Invalid element encountered */
	GEN_ERR_UNKNOWN_ADD_METHOD, /* There was an operand that used an unrecognized addressing method */
	GEN_ERR_OUT_OF_RANGE_INT,	/* There was an int exceeding the appropriate length */
	GEN_ERR_TAKEN_LABEL_NAME,	/* The label name is taken */
	GEN_ERR_LONG_LABEL_NAME,	/* label name exceeds maximum length */
	GEN_ERR_LABEL_NAME_RES,		/* detected an attempt to name a label with a reserved name */
	GEN_ERR_INVALID_LABEL_NAME, /* Invalid label name */
	GEN_ERR_INVALID_IMM_ARG		/* The argument coming after '#' is not a digit */
};

/* ___Constants___*/
extern const char *generalErrList[];

/* ___Typedef___ */
typedef struct Opcodes
{
	char name[MAX_OC_LENGTH + 1];
	int index,
		isLabelAllowed,
		maxOperands;
	char srcOpAdd[ADD_METHODS_NUM + 1],
		destOpAdd[ADD_METHODS_NUM + 1];
} Opcodes;

typedef struct Directives
{
	char name[MAX_DIR_LENGTH + 1];
	int index,
		isLabelAllowed;
} Directives;

typedef struct symbolNode
{
	char symbolName[MAX_LABEL_LENGTH + 1];
	int type,
		value,
		ARE;
	struct symbolNode *next;
} symbolNode;

typedef struct needLabelNode
{
	char labelName[MAX_LABEL_LENGTH + 1];
	int IC,
		readInLine;
	struct needLabelNode *next;
} needLabelNode;

/* ___Prototypes___*/
void err_with_line(const char *stage, int lineIndex, char *ipName, const char *text, char *specifier);
void err_wo_line(const char *stage, const char *text, char *specifier);
char *add_ext(char *baseName, char *ext);
char *remove_edge_ws(char *string);
int is_reserved_name(char *toCheck, char *resNames);
symbolNode *is_symbol(symbolNode *head, char *toCheck);
int find_element_type(ERR_DETAILS_SIG, char *toCheck, Directives dirList[], Opcodes ocList[]);
int find_addressing(ERR_DETAILS_SIG, char *operand, symbolNode *head, char *resNames, int toPrint);
int int_from_abs_arg(char *string, symbolNode *head);
int is_string_valid_int(char *toCheck);
int valid_label(ERR_DETAILS_SIG, char *toCheck, char *resNames, int toPrint, symbolNode *head);

#endif
