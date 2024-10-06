/* ___The assembler's library___ */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

/* ___Include___ */
#include "general_lib.h"

/* ___Definitions___ */
#define MIN_ARGS 2
#define SEPERATOR printf("================================================================================\n");


/* ___Enums___ */
/* Enum defining error indices related to the assembling stage. */
enum asmblrErrIndex
{
	ASMBLR_ERR_MISSING_ARGS, /* Missing command line arguments */
	ASMBLR_ERR_FILE_CREATION /* Failed to create a file */
};

/* ___Macro definitions for lists___ */
#define OC_LIST_DEC 									\
/* name, index, label, op, source addressing, destination adressing */ \
	Opcodes ocList[] = {								\
		{"mov", Element_mov, TRUE, 2, "1111", "0111"},	\
		{"cmp", Element_cmp, TRUE, 2, "1111", "1111"},	\
		{"add", Element_add, TRUE, 2, "1111", "0111"},	\
		{"sub", Element_sub, TRUE, 2, "1111", "0111"},	\
		{"not", Element_not, TRUE, 1, "0000", "0111"},	\
		{"clr", Element_clr, TRUE, 1, "0000", "0111"},	\
		{"lea", Element_lea, TRUE, 2, "0110", "0111"},	\
		{"inc", Element_inc, TRUE, 1, "0000", "0111"},	\
		{"dec", Element_dec, TRUE, 1, "0000", "0111"},	\
		{"jmp", Element_jmp, TRUE, 1, "0000", "0101"},	\
		{"bne", Element_bne, TRUE, 1, "0000", "0101"},	\
		{"red", Element_red, TRUE, 1, "0000", "0111"},	\
		{"prn", Element_prn, TRUE, 1, "0000", "1111"},	\
		{"jsr", Element_jsr, TRUE, 1, "0000", "0101"},	\
		{"rts", Element_rts, TRUE, 0, "0000", "0000"},	\
		{"hlt", Element_hlt, TRUE, 0, "0000", "0000"}};

#define DIR_LIST_DEC 									\
	/* name, index, label */							\
	Directives dirList[] = {							\
		{".data", Element_data, TRUE},					\
		{".string", Element_string, TRUE},				\
		{".entry", Element_entry, FALSE},				\
		{".extern", Element_extern, FALSE},				\
		{".define", Element_define, FALSE}};

/*___Error list___ */
char *asmblrErrList[] =
{
	"No source files were given as arguments",
	"Could not create the following file"
};

/* ___Prototypes___*/
FILE *build_res_names(const char *stage, char *resNames, Opcodes ocList[], Directives dirList[]);
int pre_process(char *baseName, char *resNames);
int first_pass(char *baseName, char *resNames, short int codeImage[], short int **dataImage,
			   Opcodes ocList[], Directives dirList[], symbolNode **symbolHead, needLabelNode **needLHead);
int second_pass(char *baseName, char *resNames, short int codeImage[], short int *dataImage,
				Opcodes ocList[], Directives dirList[], symbolNode **symbolHead, needLabelNode **needLHead, int makeOutput, int IC);
void reset_arr(short int arr[], int arrSize);
void free_need_label_list(needLabelNode *head);
void free_symbol_list(symbolNode *head);

#endif
