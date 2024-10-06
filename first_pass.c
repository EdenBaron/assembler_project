#include "first_pass.h"

/* ___The first pass___ */
/* Returns:
   - 0+ (IC): The first pass was complete with no errors, returning the IC.
   - -1 (QUIT_UPON_ERROR): An error occurred (related to malloc, files, syntax errors in the source file, etc).
   - -2 (DETECT_MORE_ERRORS): An error in the source file's syntax was detected.
*/
int first_pass(char *baseName, char *resNames, short int codeImage[], short int **dataImage,
			   Opcodes ocList[], Directives dirList[], symbolNode **symbolHead, needLabelNode **needLHead)
{
	/* ___Declarations___ */

	/* symbols will be stored in a linked list */
	symbolNode *head = *symbolHead;

	/* a linked list that stores the codeImage cells that need labels for the second pass */
	needLabelNode *nlHead = *needLHead;

	/* input file pointer */
	FILE *ip = NULL;

	/* additional */
	short int *myDataImage = *dataImage,
			  *temp = NULL;

	int IC = 0, DC = 0,
		lineIndex = 0,
		foundErrorFlag = FALSE,
		foundLabelFlag = FALSE,
		labelIsEntryFlag = FALSE,
		currentSymbolType = 0,
		currentSymbolVal = 0,
		funcRes = 0;

	const char *stage = "first pass";
	char *ipName = NULL,
		 *tempWord = NULL,
		 *currentLabelName = NULL,
		 sourceLine[MAX_LINE_LENGTH + 1] = "",
		 lineCopy[MAX_LINE_LENGTH + 1] = "";

	/* ___Adding the .am extension to the file's name___ */
	if ((ipName = add_ext(baseName, ".am")) == NULL)
	{
		err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
		return QUIT_UPON_ERROR;
	}

	/* ___Opening the file___ */
	if ((ip = fopen(ipName, "r")) == NULL)
	{
		err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], NULL);
		FP_CLOSE
		return QUIT_UPON_ERROR;
	}

	while (fgets(sourceLine, MAX_LINE_LENGTH + 1, ip) != NULL)
	{
		lineIndex++;
		/* copying the sourceLine to lineCopy to avoid it being changed by strtok */
		strcpy(lineCopy, sourceLine);

		/* tempWord will initially hold the first token of the line, which will be identified by the first_token_proc function */
		tempWord = strtok(lineCopy, DELIM);

		/* checking if the first token is a label */
		if (tempWord[strlen(tempWord) - 1] == ':')
		{
			tempWord[strlen(tempWord) - 1] = '\0';

			funcRes = valid_label(ERR_DETAILS, tempWord, resNames, TRUE, head);
			if (funcRes == FUNC_ERROR)
			{
				FP_CLOSE
				return QUIT_UPON_ERROR;
			}
			if (funcRes == LABEL_IS_ENTRY)
				labelIsEntryFlag = TRUE;

			else if (funcRes != TRUE)
			{
				foundErrorFlag = TRUE;
				continue;
			}

			foundLabelFlag = TRUE;
			currentLabelName = tempWord;

			/* tempWord will hold the next token */
			tempWord = strtok(NULL, DELIM);
		}

		funcRes = find_element_type(ERR_DETAILS, tempWord, dirList, ocList);

		/* the element is an instruction/opcode according to the index range */
		if (funcRes >= 0 && funcRes < Element_instructionEnd)
		{
			/* making a distinction between an opcode and a directive entry */
			if (labelIsEntryFlag)
				labelIsEntryFlag = ENTRY_FROM_OC;

			currentSymbolType = symbolType_code;
			currentSymbolVal = IC + IMAGE_OFFSET;
			funcRes = process_opcode(ERR_DETAILS, ocList[funcRes], sourceLine, foundLabelFlag, head, &nlHead,
									 resNames, codeImage, &IC);
		}

		/*the element is a directive according to the index range*/
		else if (funcRes > Element_instructionEnd && funcRes < Element_directiveEnd)
		{
			/* making a distinction between an opcode and a directive entry */
			if (labelIsEntryFlag)
				labelIsEntryFlag = ENTRY_FROM_DIR;

			currentSymbolType = symbolType_data;
			currentSymbolVal = DC;
			funcRes = process_dir(ERR_DETAILS, dirList[funcRes - (Element_instructionEnd + 1)], sourceLine, foundLabelFlag, &head,
								  resNames, &myDataImage, &DC);
		}

		if (funcRes == FUNC_ERROR)
			foundErrorFlag = TRUE;

		else if (labelIsEntryFlag)
		{
			symbolNode *toUpdate = is_symbol(head, currentLabelName);

			if (toUpdate == NULL)
			{
				err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_ENTRY], tempWord);
				foundErrorFlag = TRUE;
				continue;
			}
			toUpdate->value = currentSymbolVal;
			toUpdate->type = symbolType_entry;

			if (labelIsEntryFlag == ENTRY_FROM_DIR)
			{
				toUpdate->type = symbolType_entryDir;
			}
		}
		else if (foundLabelFlag)
		{
			head = new_symbol(stage, currentLabelName, currentSymbolType, currentSymbolVal, RELOCATABLE, head);
			if (head == NULL)
			{
				err_with_line(ERR_DETAILS, fpErrList[FP_ERR_SYMB_ADD], NULL);
				FP_CLOSE
				return QUIT_UPON_ERROR;
			}
		}

		/* preparing for the next iteration */
		strcpy(sourceLine, ""); /* clearing the sourceLine */
		currentLabelName = NULL;
		foundLabelFlag = FALSE;
		labelIsEntryFlag = FALSE;
		funcRes = 0;

		if (feof(ip))
			break;
	}

	/* update the data symbols' values */
	{
		symbolNode *currentNode = head;

		while (currentNode != NULL)
		{
			if (currentNode->type == symbolType_data)
				currentNode->value += (IC + IMAGE_OFFSET);
			else if (currentNode->type == symbolType_entryDir)
			{
				currentNode->value += (IC + IMAGE_OFFSET);
				currentNode->type = symbolType_entry;
			}

			currentNode = currentNode->next;
		}
	}

	temp = (short int *)realloc(myDataImage, (DC + 1) * sizeof(short int));
	if (temp == NULL)
	{
		err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
		return FUNC_ERROR;
	}
	*dataImage = myDataImage = temp;
	temp = NULL;

	myDataImage[DC] = PLACEHOLDER;

	*symbolHead = head;
	*needLHead = nlHead;
	*dataImage = myDataImage;
	FP_CLOSE

	if (foundErrorFlag)
		return DETECT_MORE_ERRORS;

	return IC;
}

/* ___Helper functions___ */

/* A function that receives a full assembly line,
   and ensures that the tokens are separated by commas properly.
   Parameters:
   - ERR_DETAILS_SIG: Error details such as stage, line index, etc.
   - toCheck: The assembly line string to check for token separation.

   Returns:
   - -1 (FUNC_ERROR) if the string is not structured properly.
   - 0+ (TRUE) indicating the number of arguments if the string is structured properly.
*/
int is_valid_line(ERR_DETAILS_SIG, char *sourceLine, int foundLabelFlag)
{
	int index,
		len,
		tokenC = 0,		  /* counter of tokens */
		commaC = 0,		  /* counter of commas */
		newTokenF = TRUE; /* flag indicating that a new token is expected after a comma or a white space */

	char current,
		*toCheck,
		lineCopy[MAX_LINE_LENGTH + 1];

	/* copying to toCheck as to not change the source line */
	strcpy(lineCopy, sourceLine);
	toCheck = lineCopy;

	if (foundLabelFlag)
	{
		toCheck = remove_edge_ws(toCheck);
		toCheck = strpbrk(toCheck, DELIM); /* points to the first white space following the label */
	}

	toCheck = remove_edge_ws(toCheck);
	toCheck = strpbrk(toCheck, DELIM); /* points to the first white space following the element */
	toCheck = remove_edge_ws(toCheck); /* points to the first arg */

	if (toCheck == NULL)
	{
		return tokenC; /* 0 arguments */
	}

	len = strlen(toCheck);
	for (index = 0; index < len; index++)
	{
		current = toCheck[index];

		if (!isspace(current))
		{
			/* the char is the beginning of a new token */
			if (current != ',')
			{
				if (newTokenF)
				{
					tokenC++;
					newTokenF = FALSE;
				}
			}
			/* found a comma, expecting to get a new token */
			else
			{
				commaC++;
				newTokenF = TRUE;
			}
		}
		else
			newTokenF = TRUE;

		/* checking the validity of the commaC, tokenC difference */
		if (commaC > tokenC)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_ILLEGAL_COMMA], NULL);
			return FUNC_ERROR;
		}
		else if (tokenC - commaC > 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_MISSING_COMMA], NULL);
			return FUNC_ERROR;
		}
	}
	if (commaC == tokenC)
	{
		err_with_line(ERR_DETAILS, fpErrList[FP_ERR_LAST_ARG_COMMA], NULL);
		return FUNC_ERROR;
	}

	return tokenC;
}

/* Creates a new symbol node with the given parameters.
   Parameters:
   - stage: The stage in which the error occurred.
   - name: The name of the symbol.
   - type: The type of the symbol.
   - value: The value associated with the symbol.
   - next: Pointer to the next symbol node in the linked list.

   Returns:
   - A pointer to the head of the symbol list.
   - NULL if memory allocation for the new node fails.
*/
symbolNode *new_symbol(const char *stage, char *name, int type, int value, int ARE, symbolNode *head)
{
	/* new node */
	symbolNode *current,
		*newNode = (symbolNode *)malloc(sizeof(symbolNode));

	if (newNode == NULL)
	{
		err_wo_line(stage, fpErrList[FP_ERR_SYMB_ADD], name);
		return NULL;
	}

	strcpy(newNode->symbolName, name);
	newNode->type = type;
	newNode->value = value;
	newNode->ARE = ARE;
	newNode->next = NULL;

	if (head == NULL)
		/* the list is empty, return the new node */
		return newNode;

	/* traverse the list to find the last node */
	current = head;
	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = newNode;

	return head;
}

/* Creates a new "need label" node (for the second pass) with the given parameters.
   Parameters:
   - stage: The stage in which the error occurred.
   - name: The name of the label needed.
   - location: IC+100
   - next: Pointer to the next symbol node in the linked list.

   Returns:
   - A pointer to the head of the "need label" list.
   - NULL if memory allocation for the new node fails.
*/
needLabelNode *new_need_label(const char *stage, char *name, int location, int readInLine, needLabelNode *head)
{
	/* new node */
	needLabelNode *current,
		*newNode = (needLabelNode *)malloc(sizeof(needLabelNode));

	if (newNode == NULL)
	{
		err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);

		return NULL;
	}

	strcpy(newNode->labelName, name);
	newNode->IC = location + 100;
	newNode->readInLine = readInLine;
	newNode->next = NULL;

	if (head == NULL)
		/* the list is empty, set the new node as the head */
		return newNode;

	/* traverse the list to find the last node */
	current = head;
	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = newNode;

	return head;
}

/* Builds the memory word for an operand, and adds it to the data image array.
   The function receives operands that have been confirmed to align with the opcode's allowed addressing methods.

   Parameters:
   - ERR_DETAILS_SIG: Error details signature for error reporting.
   - codeImage: The data image array to store machine code.
   - IC: Pointer to the instruction counter.
   - currentOc: The current opcode being processed.
   - currentOp: The string representing the current operand.
   - currentAddRes: The addressing method code.
   - head: Pointer to the head of the symbol table.
   - resNames: The file name for reserved names.

   Returns:
	-  1 (TRUE) if the word was successfully made and added.
	-  0 (FALSE) if the complete word was not yet made, due to addressing method restrictions (which is valid for operands containing labels).
	- -1 (FUNC_ERROR) if there was a problem adding a need label node.

	Notes:
	- 	This function is called only after the line has been conirmed to be valid.
	-	Thus, it relies on its validity, works while assuming the word's structure is proper, and skips test that were done beforehand.

*/
int build_operand_word(ERR_DETAILS_SIG, short int codeImage[], int *IC, char *currentOp, int currentAddRes, int opType, symbolNode *head, needLabelNode **needLHead)
{
	short int base = 0,
			  value,
			  tempValue = INT_FUNC_ERROR; /* if tempValue will not be updated during the operation, it will hold an int that's out of range */

	needLabelNode *nlHead = *needLHead;

	if (currentOp == NULL || currentAddRes == FUNC_ERROR)
		return FALSE;

	switch (currentAddRes)
	{
	case addMethod_direct:
	{
		/* label, not known in the first pass */
		nlHead = new_need_label(stage, currentOp, (*IC), lineIndex, nlHead);
		if (nlHead == NULL)
		{
			err_wo_line(stage, fpErrList[FP_ERR_NL_ADD], NULL);
			return FUNC_ERROR;
		}
		(*IC)++; /* save space for this label's code in the code image  will be updated in the second pass */
		*needLHead = nlHead;
		return FALSE;
	}
	case addMethod_directReg:
	{
		/* currentOp[1]: the char that holds the register's number */

		/* this is the destination operand */
		if (opType == opType_destOp)
			value = (currentOp[1] - '0') << DEST_REG_MOVE;
		/* this is the source operand */
		else if (opType == opType_srcOp)
			value = (currentOp[1] - '0') << SRC_REG_MOVE;

		break;
	}
	case addMethod_constInd:
	{
		char *labelName,
			*inBrackets;

		/* finding the second word - mdefine or int, getting the value stored in the brackets  */
		inBrackets = strchr(currentOp, ']');
		inBrackets[0] = '\0';

		inBrackets = strchr(currentOp, '[');
		inBrackets[0] = '\0';

		/* finding the first word - label, not known in the first pass */
		labelName = currentOp;

		nlHead = new_need_label(stage, labelName, (*IC), lineIndex, nlHead);
		if (nlHead == NULL)
		{
			err_wo_line(stage, fpErrList[FP_ERR_NL_ADD], NULL);
			return FUNC_ERROR;
		}
		(*IC)++;
		*needLHead = nlHead;
		currentOp = inBrackets;
		/* no break, the rest is the same as the next case */
	}
	case addMethod_immediate:
	{
		/* the operand is an mdefine or an int */
		/* discarding the first char */
		/* (in immediate addressing - '#') */
		/* (in constInd addressing - '[' turned '\0') */
		currentOp++;

		tempValue = int_from_abs_arg(currentOp, head);

		if (tempValue < INT_FUNC_ERROR)
		{
			value = tempValue << IMM_OP_MOVE;
		}
		else
			return FALSE;

		break;
	}
	}
	/* combine base and value, then store in codeImage at current IC position */
	base |= value;
	codeImage[IMAGE_OFFSET + (*IC)++] = base;

	return TRUE;
}

/* Creates a binary representation for the first pass.
   Parameters:
   - ERR_DETAILS_SIG: Error details signature for error reporting.
   - codeImage: The data image array to store machine code.
   - IC: Pointer to the instruction counter.
   - currentOc: The current opcode being processed.
   - destOp: The destination operand string.
   - srcOp: The source operand string.
   - head: Pointer to the head of the symbol table.
   - resNames: The file name for reserved names.

   Returns:
   - TRUE if the machine code words were successfully built.
   - FUNC_ERROR (-1) if an error occurred during processing.
*/
int first_pass_binary(ERR_DETAILS_SIG, short int codeImage[], int *IC, Opcodes currentOc, char *destOp, char *srcOp, symbolNode *head, needLabelNode **needLHead, char *resNames)
{
	short int base = 0;

	int destAddRes = 0,
		srcAddRes = 0;

	needLabelNode *nlHead = *needLHead;

	/* finding the operands' addressing methods, and checking their validity */
	if (srcOp != NULL)
	{
		/* finding the addressing method of the source operand */
		srcAddRes = find_addressing(ERR_DETAILS, srcOp, head, resNames, TRUE);
		if (srcAddRes == FUNC_ERROR)
			return FUNC_ERROR;

		if (currentOc.srcOpAdd[srcAddRes] == FALSE)
		{
			/* the source operand is using an addressing method that's not allowed for this opcode */
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_ILLEGAL_ADD_METHOD], srcOp);
			return FUNC_ERROR;
		}
	}

	if (destOp != NULL)
	{
		destAddRes = find_addressing(ERR_DETAILS, destOp, head, resNames, TRUE);
		if (destAddRes == FUNC_ERROR)
			return FUNC_ERROR;

		if (currentOc.destOpAdd[destAddRes] == FALSE)
		{
			/* the destination operand is using an addressing method that's not allowed for this opcode */
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_ILLEGAL_ADD_METHOD], destOp);
			return FUNC_ERROR;
		}
	}

	/* building the first word using the information we have on the opcode, and addressing methods */
	base |= currentOc.index << OPCODE_MOVE;
	base |= srcAddRes << SRC_OP_MOVE;
	base |= destAddRes << DEST_OP_MOVE;

	/* adding the first word to the data image */
	codeImage[IMAGE_OFFSET + (*IC)++] = base;

	/* building the next words */
	base = 0;

	/* a special case where both operands are registers, and they will share the same word */
	if (srcAddRes == addMethod_directReg && destAddRes == addMethod_directReg)
	{
		/* the value is the register's index, which is held in the string in the index of 1 */
		base |= (srcOp[1] - '0') << SRC_REG_MOVE;
		base |= (destOp[1] - '0') << DEST_REG_MOVE;

		codeImage[IMAGE_OFFSET + (*IC)++] = base;
		return TRUE;
	}

	/* building the words of the individual operands */
	build_operand_word(ERR_DETAILS, codeImage, IC, srcOp, srcAddRes, opType_srcOp, head, &nlHead);
	build_operand_word(ERR_DETAILS, codeImage, IC, destOp, destAddRes, opType_destOp, head, &nlHead);

	*needLHead = nlHead;

	return TRUE;
}

/* Processes an opcode line, handling label definitions, operand validation, and generating machine code that goes into the codeImage array.
	Parameters:
	- ERR_DETAILS_SIG: Signature for error details.
	- currentOc: Opcode structure representing the current opcode.
	- currentLine: The current line being processed.
	- foundLabelFlag: Flag indicating whether a label was found in the line.
	- head: Pointer to the symbol node head.
	- needLHead: Pointer to the head of the list of needed labels.
	- resNames: Reserved names file name.
	- codeImage: Array to store the generated machine code.
	- IC: Instruction counter.

   Returns:
   - TRUE if the machine code words were successfully built.
   - FUNC_ERROR (-1) if an error occured/was detected during processing.

   Notes:
	- Handles label definitions and checks whether labels are allowed for the directive.
	- Validates the number of operands against the instructions's requirements.
	- Uses helper function from the general library included.
*/
int process_opcode(ERR_DETAILS_SIG, Opcodes currentOc, char *currentLine, int foundLabelFlag, symbolNode *head, needLabelNode **needLHead, char *resNames, short int codeImage[], int *IC)
{
	int index,
		numOfArgs;

	char *tempWord,
		*destOp = NULL,
		*srcOp = NULL;

	needLabelNode *nlHead = *needLHead;

	if (foundLabelFlag && !currentOc.isLabelAllowed)
	{
		/* found a label definition on an element that does not take labels */
		err_with_line(ERR_DETAILS, fpErrList[FP_ERR_LABEL_NOT_ALLOWED], currentOc.name);
		return FUNC_ERROR;
	}

	numOfArgs = is_valid_line(ERR_DETAILS, currentLine, foundLabelFlag);
	if (numOfArgs != currentOc.maxOperands)
	{
		if (numOfArgs > currentOc.maxOperands)
			/* the line is of valid structure, but has excess operands */
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_EXCESS_OPERANDS], currentOc.name);

		else if (numOfArgs < currentOc.maxOperands && numOfArgs > FUNC_ERROR)
			/* the line is of valid structure, but has missing operands */
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_MISSING_OPERANDS], currentOc.name);

		return FUNC_ERROR;
	}
	/* processing the operands */
	for (index = 0; index < currentOc.maxOperands; index++)
	{
		tempWord = strtok(NULL, DELIM_WITH_COMMA);

		if (index == currentOc.maxOperands - 1)
			/* the destination operand is always the last operand */
			destOp = tempWord;
		else
			srcOp = tempWord;
	}

	/* building the machine code for the opcode's line */
	first_pass_binary(ERR_DETAILS, codeImage, IC, currentOc, destOp, srcOp, head, &nlHead, resNames);

	*needLHead = nlHead;

	return TRUE;
}

/* Processes a directive line, handling label definitions, operand validation, and data/image generation.
Parameters:
	- ERR_DETAILS_SIG: Signature for error details.
	- currentDir: Directive structure variable representing the current directive.
	- currentLine: The current line being processed.
	- foundLabelFlag: Flag indicating whether a label was found in the line.
	- symbolHead: Pointer to the head of the symbol node.
	- resNames: The reserved names file name.
	- dataImage: Pointer to the data image array.
	- DC: Data counter (pointer).

Returns:
	- TRUE if the directive line is processed successfully.
	- FUNC_ERROR if an error occurs during processing.

Notes:
	- Handles label definitions and checks whether labels are allowed for the directive.
	- Validates the number of operands against the directive's requirements.
	- Processes operands and generates data image value (short int) for the directive's line.
	- Updates the symbol node head and data image array as necessary.
*/
int process_dir(ERR_DETAILS_SIG, Directives currentDir, char *currentLine, int foundLabelFlag, symbolNode **symbolHead, char *resNames, short int **dataImage, int *DC)
{
	int index = 0,
		tempValue,
		numOfArgs = 0;

	char *tempWord,
		*helper = NULL;

	symbolNode *head = *symbolHead,
			   *tempNode;

	short int *myDataImage = *dataImage,
			  *temp = NULL;

	if (foundLabelFlag && !currentDir.isLabelAllowed)
	{
		/* found a label definition on an element that does not take labels */
		err_with_line(ERR_DETAILS, fpErrList[FP_ERR_LABEL_NOT_ALLOWED], currentDir.name);
		return FUNC_ERROR;
	}

	/* checking line validity except for types defined using commas */
	if (currentDir.index != Element_define && currentDir.index != Element_string)
		numOfArgs = is_valid_line(ERR_DETAILS, currentLine, foundLabelFlag);

	if (numOfArgs == FUNC_ERROR)
		return FUNC_ERROR;

	/*setting the currentLine to start after the label, if it exists */
	if (foundLabelFlag)
	{
		currentLine = strchr(currentLine, ':');
		currentLine++;
		remove_edge_ws(currentLine);
	}

	switch (currentDir.index)
	{
	case Element_data:
	{
		if (numOfArgs < 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_MISSING_OPERANDS], currentDir.name);
			return FUNC_ERROR;
		}

		/* realloc to accommodate new data */
		temp = (short int *)realloc(myDataImage, ((*DC) + numOfArgs) * sizeof(short int));
		if (temp == NULL)
		{
			err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
			return FUNC_ERROR;
		}
		*dataImage = myDataImage = temp;
		temp = NULL;

		/* points to the .data directive */
		tempWord = strtok(currentLine, DELIM_WITH_COMMA);

		/* tempWord holds the current argument */
		while (index < numOfArgs)
		{
			/* could be an int, or an mdefine */
			tempWord = strtok(NULL, DELIM_WITH_COMMA);

			tempValue = int_from_abs_arg(tempWord, head);
			if (tempValue >= INT_FUNC_ERROR && is_symbol(head, tempWord) == NULL)
			{
				err_with_line(ERR_DETAILS, fpErrList[FR_ERR_MISSING_DEFINE], tempWord);
				return FUNC_ERROR;
			}

			myDataImage[index + (*DC)] = tempValue;

			index++;
		}
		(*DC) += numOfArgs;
		break;
	}
	case Element_string:
	{
		/* tempWord holds the string inside the parethesis */
		if ((tempWord = strchr(currentLine, '"')) != NULL)
		{
			tempWord++;
			helper = strchr(tempWord, '"');

			if (helper != NULL)
			{
				helper[0] = '\0';
			}
		}
		if (tempWord == NULL || helper == NULL)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_STRING], NULL);
			return FUNC_ERROR;
		}

		numOfArgs = strlen(tempWord) + 1; /* considering the null terminator */

		/* realloc to accommodate new data */
		temp = (short int *)realloc(myDataImage, ((*DC) + numOfArgs) * sizeof(short int));
		if (temp == NULL)
		{
			err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
			return FUNC_ERROR;
		}
		*dataImage = myDataImage = temp;
		temp = NULL;

		while (index < numOfArgs - 1)
		{
			myDataImage[index + (*DC)] = tempWord[index];
			index++;
		}
		myDataImage[index + (*DC)] = '\0';
		(*DC) += numOfArgs;
		break;
	}
	case Element_entry:
	{
		if (numOfArgs > 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_EXCESS_OPERANDS], currentDir.name);
			return FUNC_ERROR;
		}
		if (numOfArgs < 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_MISSING_OPERANDS], currentDir.name);
			return FUNC_ERROR;
		}

		/* points to the .entry directive */
		tempWord = strtok(currentLine, DELIM);
		/* points to the label */
		tempWord = strtok(NULL, DELIM);

		if (valid_label(ERR_DETAILS, tempWord, resNames, FALSE, head) == FUNC_ERROR)
		{
			err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], resNames);
			return FUNC_ERROR;
		}

		if (valid_label(ERR_DETAILS, tempWord, resNames, FALSE, head) == FALSE)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_ENTRY], tempWord);
			return FUNC_ERROR;
		}

		tempNode = is_symbol(head, tempWord);
		if (tempNode != NULL)
		{
			if (tempNode->type != symbolType_mdefine && tempNode->type != symbolType_extern)
				tempNode->type = symbolType_entry;
			else
			{
				err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_ENTRY], tempWord);
				return FUNC_ERROR;
			}
		}
		else
		{
			/* adding the label to the symbol tabel as an entry */
			head = new_symbol(stage, tempWord, symbolType_entryTemp, PLACEHOLDER, RELOCATABLE, head);
			if (head == NULL)
			{
				err_with_line(ERR_DETAILS, fpErrList[FP_ERR_SYMB_ADD], NULL);
				return FUNC_ERROR;
			}
			*symbolHead = head;
		}
		break;
	}
	case Element_extern:
	{
		int validlabelRes;

		if (numOfArgs > 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_EXCESS_OPERANDS], currentDir.name);
			return FUNC_ERROR;
		}
		if (numOfArgs < 1)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_MISSING_OPERANDS], currentDir.name);
			return FUNC_ERROR;
		}

		/* points to the .extern directive */
		tempWord = strtok(currentLine, DELIM);
		/* points to the label */
		tempWord = strtok(NULL, DELIM);

		validlabelRes = valid_label(ERR_DETAILS, tempWord, resNames, TRUE, head);

		if (validlabelRes != TRUE)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_EXTERN], tempWord);
			return FUNC_ERROR;
		}

		tempNode = is_symbol(head, tempWord);
		if (tempNode != NULL)
		{
			/* there is an existing symbol with the same name */
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_EXTERN], tempWord);
			return FUNC_ERROR;
		}

		/* adding the label to the symbol tabel as an extern */
		head = new_symbol(stage, tempWord, symbolType_extern, 0, EXTERNAL, head);
		if (head == NULL)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_SYMB_ADD], NULL);
			return FUNC_ERROR;
		}
		*symbolHead = head;
		break;
	}
	case Element_define:
	{
		tempWord = strchr(currentLine, '=');
		if (tempWord == NULL)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_DEFINE], NULL);
			return FUNC_ERROR;
		}
		tempWord[0] = '\0';
		tempWord++; /* now points on the right side of the '=' symbol */

		tempValue = int_from_abs_arg(tempWord, head);
		if (tempValue > MAX_DIR_NUM || tempValue < MIN_DIR_NUM)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_DEFINE_VAL], tempWord);
			return FUNC_ERROR;
		}

		/* points to the .define directive */
		tempWord = strtok(currentLine, DELIM);

		/* points to the label */
		tempWord = strtok(NULL, DELIM);

		if (valid_label(ERR_DETAILS, tempWord, resNames, FALSE, head) < TRUE)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_INVALID_DEFINE], NULL);
			return FUNC_ERROR;
		}

		head = new_symbol(stage, tempWord, symbolType_mdefine, tempValue, ABSOLUTE, head);
		if (head == NULL)
		{
			err_with_line(ERR_DETAILS, fpErrList[FP_ERR_SYMB_ADD], NULL);
			return FUNC_ERROR;
		}
		*symbolHead = head;
		break;
	}
	default:
		break;
	}
	*symbolHead = head;
	*dataImage = myDataImage;
	return TRUE;
}
