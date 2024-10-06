#include "pre_process.h"

/* ___The pre-processor___ */
/* Returns:
   - 0 (SUCCESS): The pre-processing stage was completed with no errors, and the AM file was created.
   - -1 (QUIT_UPON_ERROR): An error occurred (related to malloc, files, syntax errors in the source file, etc.), and the AM file is to be deleted.
   - -2 (DETECT_MORE_ERRORS): A line longer than the buffer was detected, the AM file was created, but the assembler will not make additional output 	 			     files.
*/
int pre_process(char *baseName, char *resNames)
{
	/* ___Declarations___ */

	/* a flag to indicate the current macro definition stage */
	int mcrDef = OUTSIDE_MCR;

	/* macros will be stored in a linked list */
	mcrNode *head = NULL;

	/* input, output file pointers */
	FILE *ip = NULL, *op = NULL;

	/* a long to record file positioning */
	long filePos = 0;

	/* additional */
	int lineIndex = 0,
		mcrLineCount = 0,
		longLineFlag = FALSE, /* a flag indicating that the current line is longer than the buffer */
		longLineCount = 0;	  /* counts the total number of long lines */

	const char *stage = "pre processing";
	char *ipName = NULL,
		 *opName = NULL,
		 *tempWord = NULL,
		 sourceLine[MAX_LINE_LENGTH + 1] = "",
		 lineCopy[MAX_LINE_LENGTH + 1] = "";

	/* ___Adding extensions to the file names___ */
	if ((ipName = add_ext(baseName, ".as")) == NULL ||
		(opName = add_ext(baseName, ".am")) == NULL)
	{
		/* malloc failed */
		err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
		PP_CLOSE(head, FALSE, resNames)
		return QUIT_UPON_ERROR;
	}

	/* ___Opening the files___ */
	/* input: for reading */
	if ((ip = fopen(ipName, "r")) == NULL)
	{
		/* file opening failed */
		err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], ipName);
		PP_CLOSE(head, FALSE, resNames)
		return QUIT_UPON_ERROR;
	}
	/* output: for writing */
	if ((op = fopen(opName, "w")) == NULL)
	{
		/* file making failed */
		err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], opName);
		PP_CLOSE(head, FALSE, resNames)
		return QUIT_UPON_ERROR;
	}

	/* ___Processing the lines of the source file___ */
	while (fgets(sourceLine, MAX_LINE_LENGTH + 1, ip) != NULL)
	{
		lineIndex++;

		/* detecting if the source line is longer than the buffer */
		/* if it is, the line will not be proccessed */
		if (is_long_line(ERR_DETAILS, sourceLine, MAX_LINE_LENGTH, ip))
			longLineFlag = TRUE;

		/* processing the line's contents */
		remove_edge_ws(sourceLine);
		strcpy(lineCopy, sourceLine);

		/* tempWord will initially hold the first token of the line */
		tempWord = strtok(lineCopy, DELIM);

		/* will process only if the line is of valid lenght, not empty, and not a comment */
		if (tempWord != NULL && tempWord[0] != ';' && !longLineFlag)
		{
			switch (mcrDef)
			{
			case OUTSIDE_MCR:
			{
				if (strcmp(tempWord, "mcr") == 0)
				{ /* an mcr definition command was found */
					tempWord = strtok(NULL, DELIM);
					if (tempWord == NULL)
					{
						err_with_line(ERR_DETAILS, ppErrList[PP_ERR_MISSING_MCR_NAME], NULL);
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}
					if (strtok(NULL, DELIM) != NULL)
					{
						err_with_line(ERR_DETAILS, ppErrList[PP_ERR_EXTRA_MCR_TEXT], NULL);
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}
					if (valid_mcr(ERR_DETAILS, tempWord, resNames) != TRUE)
					{
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}
					/* otherwise, the macro name is valid */
					/* entering the line counting stage */
					mcrDef = COUNTING_MCR_LINES;
					filePos = ftell(ip);
					mcrLineCount = 0;

					/* setting the new macro as the head of the list */
					head = new_mcr(stage, tempWord, head);
					if (head == NULL)
					{
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}
				}
				else if (print_if_mcr(head, tempWord, op))
				{ /* mcr name was found */
					continue;
				}
				else
				{ /* copy the line to the am file normally */
					fprintf(op, "%s\n", sourceLine);
				}
				break;
			}
			case COUNTING_MCR_LINES:
			{
				if (strcmp(tempWord, "endmcr") == 0)
				{
					if (strtok(NULL, DELIM) != NULL)
					{
						err_with_line(ERR_DETAILS, ppErrList[PP_ERR_EXTRA_ENDMCR_TEXT], NULL);
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}
					/* the line counting is done */
					/* proceeding the the line addition stage */
					mcrDef = ADDING_MCR_LINES;
					filePos = fseek(ip, filePos, SEEK_SET);
					head->lineCount = mcrLineCount;

					/* allocating memory for the macro's lines */
					head->lines = (char(*)[MAX_LINE_LENGTH + 1]) malloc(mcrLineCount * (MAX_LINE_LENGTH + 1) * sizeof(char));
					if (head->lines == NULL)
					{
						err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
						PP_CLOSE_AND_REMOVE_AM
						return QUIT_UPON_ERROR;
					}

					/* resetting the line count which will be used as an index in the line addition process */
					mcrLineCount = 0;
				}
				else
					mcrLineCount++;
				break;
			}
			case ADDING_MCR_LINES:
			{
				/* returning to normal processing after finding the endmcr command */
				if (strcmp(tempWord, "endmcr") == 0)
				{
					mcrDef = OUTSIDE_MCR;
					filePos = 0;
					mcrLineCount = 0;
				}
				/* else, adding the current line to the macro's line list */
				else
				{
					strcat(sourceLine, "\n");
					strcpy(head->lines[mcrLineCount], sourceLine);
					mcrLineCount++;
				}
				break;
			}
			default:
				break;
			}
		}

		/* preparing for the next iteration */
		strcpy(sourceLine, ""); /* clearing the sourceLine */
		longLineCount += longLineFlag;
		longLineFlag = 0;

		if (feof(ip))
			break;
	}

	/* ___Closing all files and freeing allocated memory___ */
	/* in the case that no errors were detected, the .am file will not be deleted */
	/* and the names of the existing macros will be added to the reserved names list */
	PP_CLOSE(head, TRUE, resNames)

	if (longLineCount > 0)
		return DETECT_MORE_ERRORS;

	printf(">>> A Macro-expanded file (.am) was added to the directory.\n");
	return SUCCESS;
}

/* ___Helper functions___ */

/* Checks whether a given line is longer than a desired buffer.
   Parameters:
   - ERR_DETAILS_SIG: Signature for error details, typically including stage, line index, and IP name.
   - lineToCheck: The line to check for length.
   - buffer: The desired buffer size.
   - fp: The file pointer to the input file.

   Returns:
   - 1 (TRUE) if the line is longer than the buffer.
   - 0 (FALSE) if the line is of valid length.

   Behavior:
   - Assumes the line ends with a newline character.
   - Checks if the last character before the newline is not a newline character, and if the length equals the buffer size.
   - If the line is longer than the buffer, it flushes excess characters from the file pointer.
*/
int is_long_line(ERR_DETAILS_SIG, char *lineToCheck, int buffer, FILE *fp)
{
	if (lineToCheck[buffer - 1] != '\n' && strlen(lineToCheck) == buffer)
	{
		int c;

		err_with_line(ERR_DETAILS, ppErrList[PP_ERR_LONG_LINE], NULL);

		/* flushing excess chars */
		while ((c = fgetc(fp)) != '\n' && c != EOF)
			;
		return TRUE;
	}
	return FALSE;
}

/* A function that receives a string and checks if it makes for a valid macro name.
   Parameters:
   - ERR_DETAILS_SIG: Signature for error details, typically including stage, line index, and IP name.
   - toCheck: The string to check if it's a valid macro name.
   - resNames: The file containing reserved names, used for checking if the name is reserved.

   Returns:
   - -1 (FUNC_ERROR) if there was an error opening the reserved names file or another function-related error.
   - 1 (TRUE) if the string is a valid macro name.
   - 0 (FALSE) if the string is not a valid macro name.

   Behavior:
   - Checks the length of the string and if it's longer than MAX_LABEL_LENGTH, returns an error.
   - Checks if the string is a reserved name, and if so, returns an error.
   - Checks if the first character of the string is alphabetic.
   - Checks if the rest of the characters are printable.
*/
int valid_mcr(ERR_DETAILS_SIG, char *toCheck, char *resNames)
{
	int index,
		len = strlen(toCheck),
		resNameVal = is_reserved_name(toCheck, resNames);

	if (len > MAX_LABEL_LENGTH)
	{
		err_with_line(ERR_DETAILS, ppErrList[PP_ERR_LONG_MCR_NAME], NULL);
		return FALSE;
	}

	if (resNameVal == FUNC_ERROR)
	{
		err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], resNames);
		return FUNC_ERROR;
	}

	if (resNameVal == TRUE)
	{
		err_with_line(ERR_DETAILS, ppErrList[PP_ERR_MCR_NAME_RES], toCheck);
		return FALSE;
	}

	if (!isalpha(toCheck[0])) /* first char is supposed to be an alphabetic character */
	{
		err_with_line(ERR_DETAILS, ppErrList[PP_ERR_INVALID_MCR_NAME], toCheck);
		return FALSE;
	}

	for (index = 1; index < len; index++)
	{
		if (!isprint(toCheck[index])) /* the rest of the characters are supposed to be printable */
		{
			err_with_line(ERR_DETAILS, ppErrList[PP_ERR_INVALID_MCR_NAME], toCheck);
			return FALSE;
		}
	}
	return TRUE;
}

/* A function that adds a new macro node to the macros' linked list.
   Parameters:
   - stage: A string representing the context or stage where the function is called.
   - name: The name of the macro to be added.
   - next: Pointer to the next node in the linked list.

   Returns:
   - Pointer to the newly created macro node.
   - NULL if there was an error during node creation.

   Behavior:
   - Allocates memory for a new macro node.
   - Copies the macro's name to the node.
   - Initializes the line counter to 0.
   - Sets the next pointer to the provided next node.
   - Returns a pointer to the new node or NULL if an error occurs.
*/
mcrNode *new_mcr(const char *stage, char *name, mcrNode *next)
{
	/* new node */
	mcrNode *newNode = (mcrNode *)malloc(sizeof(mcrNode));
	if (newNode == NULL)
	{
		err_wo_line(stage, ppErrList[PP_ERR_MCR_ADD], name);
		return NULL;
	}

	/* setting the macro's name */
	strcpy(newNode->mcrName, name);

	/* setting the line counter */
	newNode->lineCount = 0;

	/* setting the next node */
	newNode->next = next;

	return newNode;
}

/* A function that checks if a string is a macro's name, and prints its lines to a file if found.
   Parameters:
   - head: Pointer to the head of the linked list containing macros.
   - toCheck: String to check if it's a macro's name.
   - op: File pointer to the output file where macro lines will be printed.

   Returns:
   - 1 (TRUE) if the string is a macro name and its lines are printed.
   - 0 (FALSE) if the string is not a macro name.

   Behavior:
   - Iterates through the linked list of macros.
   - If the string matches a macro's name, prints its lines to the output file.
   - Returns TRUE if a match is found and lines are printed, otherwise FALSE.
*/
int print_if_mcr(mcrNode *head, char *toCheck, FILE *op)
{
	mcrNode *current = head;

	while (current != NULL)
	{
		if (strcmp(toCheck, current->mcrName) == 0) /* there is a macro with the same name */
		{
			int index;

			for (index = 0; index < current->lineCount; index++)
			{
				fprintf(op, "%s", current->lines[index]); /* printing the macro's lines to the output file */
			}
			return TRUE;
		}
		current = current->next;
	}
	return FALSE;
}

/* A function that frees the memory allocated for a linked list of macro nodes.
   Parameters:
   - head: Pointer to the head of the linked list.
   - saveMcrNames: Flag indicating whether to add the macro names to the resNames file -
		   TRUE if the pre-process stage had no errors.
   - resNames: The name of the file to append the macro names to if saveMcrNames is true.
*/
void free_linked_list(mcrNode *head, int saveMcrNames, char *resNames)
{
	mcrNode *current = head,
			*next;

	FILE *fp = fopen(resNames, "a");

	while (current != NULL)
	{
		/* adding the macro's name to the reserved names list if saveMcrNames is true and fp is not NULL */
		if (saveMcrNames && fp != NULL)
		{
			fprintf(fp, "%s\n", current->mcrName);
		}

		next = current->next;
		free(current->lines); /* free memory for lines array */
		free(current);		  /* free memory for current node */
		current = next;
	}

	fclose(fp);
}
