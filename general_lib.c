#include "general_lib.h"

/* Prints an error message with contextual information about a specific line.
   Parameters:
   - stage: The stage in which the error occurred.
   - lineIndex: The index of the line in the input file, where the error occurred.
   - ipName: The input file name.
   - text: The error message text.
   - specifier: Optional specifier for additional error details.
*/
void err_with_line(const char *stage, int lineIndex, char *ipName, const char *text, char *specifier)
{
	ERROR_WITH_LINE

	if (specifier != NULL)
		fprintf(stderr, "%s: \"%s\".\n", text, specifier);
	else
		fprintf(stderr, "%s.\n", text);
}

/* Prints an error message without specific line information.
   Parameters:
   - stage: The stage in which the error occurred.
   - text: The error message text.
   - specifier: Optional specifier for additional error details.
*/
void err_wo_line(const char *stage, const char *text, char *specifier)
{
	ERROR_WO_LINE

	if (specifier != NULL)
		fprintf(stderr, "%s: \"%s\".\n", text, specifier);
	else
		fprintf(stderr, "%s.\n", text);
}

/* Adds an extension to a base filename.
   Parameters:
   - baseName: The base filename.
   - ext: The extension to add.

   Returns:
   - A dynamically allocated string containing the base filename with the extension added.
   - NULL if memory allocation for the new string fails.
*/
char *add_ext(char *baseName, char *ext)
{
	char *nameWithExt;

	nameWithExt = (char *)malloc(strlen(baseName) + EXT_LENGTH + 1);

	if (nameWithExt == NULL)
	{
		return NULL;
	}

	strcpy(nameWithExt, baseName);
	strcat(nameWithExt, ext);

	return nameWithExt;
}

/* Removes leading and trailing white spaces from a string.
   Parameters:
   - string: The string to modify.

   Returns:
   - A pointer to the modified string with leading and trailing white spaces removed.
   - NULL if the input string is NULL.
*/
char *remove_edge_ws(char *string)
{
	if (string != NULL)
	{
		/* starting from the end of the string */
		int len = strlen(string),
			index = len - 1;

		while (isspace(string[index]) && index > 0)
			index--;

		/* putting a null terminator right after the last non-whitespace char of the string */
		string[index + 1] = '\0';

		/* resetting the index */
		index = 0;

		while (isspace(string[index]) && string[index] != '\0')
			index++;

		/* moving the corrected string to the index */
		memmove(string, string + index, strlen(string + index) + 1);
		return string;
	}
	return NULL;
}

/* A function that checks if a string is a reserved name.
   Parameters:
   - toCheck: The string to check.
   - resNames: The file containing reserved names.

   Returns:
   - -1 (FUNC_ERROR) if the reserved names file could not be opened.
   - 1 (TRUE) if the string is a reserved name.
   - 0 (FALSE) if the string is not found in the reserved names file.
*/
int is_reserved_name(char *toCheck, char *resNames)
{
	char currentLine[MAX_LABEL_LENGTH + 1];

	FILE *fp = fopen(resNames, "r");
	if (fp == NULL)
	{
		return FUNC_ERROR;
	}

	while (fgets(currentLine, MAX_LABEL_LENGTH + 1, fp) != NULL)
	{
		remove_edge_ws(currentLine);
		if (strcmp(currentLine, toCheck) == 0)
		{
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);
	return FALSE;
}

/* A function that checks if a string is a symbol's name.
   Parameters:
   - head: Pointer to the head of the symbol table linked list.
   - toCheck: The string to check if it's a symbol's name.

   Returns:
   - Pointer to the symbol's node if the string is a symbol's name.
   - NULL if the string is not a symbol's name.
*/
symbolNode *is_symbol(symbolNode *head, char *toCheck)
{
	symbolNode *current = head;

	while (current != NULL)
	{
		if (strcmp(toCheck, current->symbolName) == 0)
		{
			/* there is a symbol with the same name */
			return current;
		}
		current = current->next;
	}
	return FALSE;
}

/* A function that processes assembly code lines and determines the element type.

   Parameters:
   - stage: The stage where the error occurred.
   - lineIndex: The line index where the error occurred.
   - ipName: The input file name.
   - toCheck: The string to check for its element type.
   - resNames: The reserved names file name.
   - dirList: An array of directives.
   - ocList: An array of opcodes.

   Returns:
   - -1 (FUNC_ERROR): Error - unrecognized element.
   - 0-15: Index corresponding to opcodes.
   - 18-22: Index corresponding to directives.
*/
int find_element_type(ERR_DETAILS_SIG, char *toCheck, Directives dirList[], Opcodes ocList[])
{
	int index;

	if (toCheck == NULL)
		return FUNC_ERROR;

	/* going through the oclist array*/
	if (ocList != NULL)
		for (index = 0; index < Element_instructionEnd; index++)
		{
			if (strcmp(toCheck, ocList[index].name) == 0)
				return index;
		}
	/* going through the dirlist array */
	if (dirList != NULL)
		for (index = 0; index < Element_directiveEnd - 1 - Element_instructionEnd; index++)
		{
			if (strcmp(toCheck, dirList[index].name) == 0)
				return index + Element_instructionEnd + 1;
		}
	err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_UNKNOWN_ELEMENT], toCheck);
	return FUNC_ERROR;
}

/*
   Determines the addressing method code based on the given operand string.

   Parameters:
   - ERR_DETAILS_SIG: Error details signature for error reporting.
   - operand: The string representing an operand.
   - head: Pointer to the head of the symbol table.
   - resNames: The file name for reserved names.

   Returns:
   - -1 (ERROR) if the addressing method is not recognized.
   - 0-3 according to the addressing method that was detected.
*/
int find_addressing(ERR_DETAILS_SIG, char *operand, symbolNode *head, char *resNames, int toPrint)
{
	int index,
		len = strlen(operand);

	char opCopy[MAX_LINE_LENGTH + 1],
		*temp;

	if (operand == NULL)
		return FUNC_ERROR;

	remove_edge_ws(operand);
	strcpy(opCopy, operand);
	temp = opCopy;

	/* 00 - immediate addressing - the operand is an #mdefine || #int*/
	if (temp[0] == '#')
	{
		int value = int_from_abs_arg(++temp, head); /* temp will be read from the char coming after # */

		if (value == INT_FUNC_ERROR)
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_UNKNOWN_ADD_METHOD], operand);
			return FUNC_ERROR;
		}
		if (value > MAX_OP_NUM || value < MIN_OP_NUM)
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_OUT_OF_RANGE_INT], temp);
			return FUNC_ERROR;
		}
		return addMethod_immediate;
	}
	if (temp[0] == '#')
	{
		int value = int_from_abs_arg(++temp, head); /* temp will be read from the char coming after # */

		if (!isdigit(*temp))
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_INVALID_IMM_ARG], operand);
			return FUNC_ERROR;
		}
		if (value == INT_FUNC_ERROR)
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_UNKNOWN_ADD_METHOD], operand);
			return FUNC_ERROR;
		}
		if (value > MAX_OP_NUM || value < MIN_OP_NUM)
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_OUT_OF_RANGE_INT], temp);
			return FUNC_ERROR;
		}
		return addMethod_immediate;
	}
	/* 01 - direct addressing - the operand is a label */
	if (valid_label(ERR_DETAILS, temp, resNames, FALSE, head) > FALSE)
		return addMethod_direct;

	/* 10 - constant index addressing - the operand is a label[mdefine || int] */
	if (strchr(temp, '[') != NULL)
	{
		/* left, right bracket counters */
		int lbC = 0,
			rbC = 0;

		char *checkContents;

		symbolNode *tempNode;
		for (index = 0; index < len; index++)
		{
			if (temp[index] == '[' && lbC == 0)
			{
				if (index == 0)
					/* there's no potential label before the left bracket */
					break;
				lbC = 1;
				temp[index] = '\0'; /* making the string preceeding the brackets accessible */

				if (temp[index + 1] == '\0')
					/* there are no more characters following the left bracket */
					break;

				checkContents = &temp[index + 1];
			}
			if (temp[index] == ']' && rbC == 0)
			{
				if (temp[index + 1] != '\0')
					/* there are more characters following the right bracket */
					break;

				rbC = 1;
				temp[index] = '\0'; /* making the string inside the brackets accessible */
			}

			if (rbC > lbC) /* not allowing the right square bracket to appear before the left */
				break;
		}

		if (rbC == 1 && lbC == 1)
		{
			/* now, checkContents holds the argument inside the brackets */
			tempNode = is_symbol(head, checkContents);

			if ((tempNode != NULL && tempNode->type == symbolType_mdefine) || is_string_valid_int(checkContents))
			{
				/* check if the string preceeding the brackets makes for a valid label */
				if (valid_label(ERR_DETAILS, temp, resNames, FALSE, head) > FALSE)
					return addMethod_constInd;
			}
		}

		/* otherwise,  the operand cannot match another addressing method (only this one will allow [), */
		/* but it's invalid */
		if (toPrint)
			err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_UNKNOWN_ADD_METHOD], temp);
		return FUNC_ERROR;
	}
	/* 11 - direct register addressing - the operand is a register */
	if (temp[0] == 'r')
		if ((temp[1] - '0') >= FIRST_REG_NUM && (temp[1] - '0') <= LAST_REG_NUM && len == REG_NAME_LENGTH)
			return addMethod_directReg;

	/* otherwise, */
	if (toPrint)
		err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_UNKNOWN_ADD_METHOD], temp);
	return FUNC_ERROR;
}

/*
 * Converts a string to an integer.
 * The string could be either a whole number, or an mdefine.
 *
 * Parameters:
 * - string: The input string to convert to an integer.
 * - head: Pointer to the head of the symbol table.
 *
 * Returns:
 * - The converted integer value if successful.
 * - INT_FUNC_ERROR if the given string is neither an int nor an mdefine.
 */
int int_from_abs_arg(char *string, symbolNode *head)
{
	int value;

	symbolNode *tempNode;

	string = remove_edge_ws(string);

	if (string != NULL && is_string_valid_int(string))
		value = atoi(string);
	else
	{
		tempNode = is_symbol(head, string);
		if (tempNode != NULL && tempNode->type == symbolType_mdefine)
			value = tempNode->value;
		else
			/* the string is not an int and not an mdefine */
			return INT_FUNC_ERROR;
	}

	return value;
}

/*
   Checks if a string represents a valid integer.

   Parameters:
   - toCheck: The string to check for integer validity.

   Returns:
   - 1 (TRUE) if the string is a valid integer.
   - 0 (FALSE) if the string is not a valid integer.
*/
int is_string_valid_int(char *toCheck)
{
	int index,
		len;

	remove_edge_ws(toCheck);
	len = strlen(toCheck);

	/* only the first character is allowed to be either a minus/plus sign, or a digit */
	if (toCheck[0] != '-' && toCheck[0] != '+' && !isdigit(toCheck[0]))
		return FALSE;

	/* the rest of the characters are allowed to be only digits */
	for (index = 1; index < len; index++)
	{
		if (!isdigit(toCheck[index]))
			return FALSE;
	}
	return TRUE;
}

/* A function that checks if a string is a valid label name.

   Parameters:
   - stage: The stage in which the error occurred.
   - lineIndex: The line index where the error occurred.
   - ipName: The input file name.
   - toCheck: The string to check if it's a valid label name.
   - resNames: The reserved names file name.
   - toPrint: Flag to indicate whether error printing is enabled (this function is used in multiple scenarios, not all of them neccecitate error printing).

   Returns:
   - -1 (FUNC_ERROR): Error - could not open the reserved names file.
   -  0 (FALSE) if the string is not a valid label name.
   -  1 (TRUE) if the string is a valid label name.
   -  2 (LABEL_IS_ENTRY) if the string is the name of an existing entry label
   -  3 (LABEL_EXISTS) if the string is the name of an existing label
*/
int valid_label(ERR_DETAILS_SIG, char *toCheck, char *resNames, int toPrint, symbolNode *head)
{
	int index,
		len,
		resNameVal;

	symbolNode *tempNode;

	remove_edge_ws(toCheck);
	tempNode = is_symbol(head, toCheck);

	if (tempNode != NULL)
	{
		if (tempNode->type == symbolType_entryTemp)
			return LABEL_IS_ENTRY;

		if (toPrint)
			err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_TAKEN_LABEL_NAME], toCheck);
		return LABEL_EXISTS;
	}

	len = strlen(toCheck);
	if (len > MAX_LABEL_LENGTH)
	{
		if (toPrint)
			err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_LONG_LABEL_NAME], NULL);
		return FALSE;
	}

	resNameVal = is_reserved_name(toCheck, resNames);
	if (resNameVal == FUNC_ERROR)
	{
		if (toPrint)
			err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], resNames);
		return FUNC_ERROR;
	}

	if (resNameVal == TRUE)
	{
		if (toPrint)
			err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_LABEL_NAME_RES], toCheck);
		return FALSE;
	}

	if (!isalpha(toCheck[0])) /* first char is supposed to be an alphabetic character */
	{
		if (toPrint)
			err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_INVALID_LABEL_NAME], toCheck);
		return FALSE;
	}

	for (index = 1; index < len; index++)
	{
		if (!isalnum(toCheck[index])) /* the rest of the characters are supposed to be printable */
		{
			if (toPrint)
				err_with_line(ERR_DETAILS, generalErrList[GEN_ERR_INVALID_LABEL_NAME], toCheck);
			return FALSE;
		}
	}

	return TRUE;
}

/* ___Error List___ */
const char *generalErrList[] =
	{
		"Error in memory allocation",
		"Could not open the following file",
		"Unrecognized element",
		"The following operand's addressing method is not recognized",
		"The following number is outside the desired range",
		"The following label name is already a symbol",
		"The following label's name is longer than 31 characters",
		"The following label's name is conflicting with a reserved name",
		"The following label name is invalid"

};
