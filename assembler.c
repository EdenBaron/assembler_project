#include "assembler.h"

/* ___The Assembler___ */
int main(int argc, char *argv[])
{
	/* ___Declarations___ */
	/* the array that will represent the code image */
	short int codeImage[RAM_SIZE] = {PLACEHOLDER};

	/* the array that will represent the data image - dynamically allocated */
	short int *dataImage = NULL;

	/* symbols will be stored in a linked list */
	symbolNode *head = NULL;

	/* a linked list that stores the codeImage cells that need labels for the second pass */
	needLabelNode *nlHead = NULL;

	/* a pointer to a new file containing reserved names */
	FILE *resNamesP = NULL;

	/* additional */
	int index,
		ppRes,
		fpRes,
		spRes;

	const char *stage = "assembler";
	char	resNames[] = "reserved_names";

	/* ocList, dirList - definitions of the opcodes and directives that will be referenced throughout the program  */
	OC_LIST_DEC
	DIR_LIST_DEC

	/* ___Starting to process the command line input___ */
	if (argc < MIN_ARGS)
	{
		err_wo_line(stage, asmblrErrList[ASMBLR_ERR_MISSING_ARGS], NULL);
		return QUIT_UPON_ERROR;
	}

	/* ___Processing each file of the given arguments___ */
	for (index = 1; index < argc; index++)
	{
		char    *woExtension = NULL,
		        *nameToPrint = NULL;
		
		/* ___Creating a reserved names list, individual to the current file ___ */
		if ((resNamesP = build_res_names(stage, resNames, ocList, dirList)) == NULL)
		{
			err_wo_line(stage, asmblrErrList[ASMBLR_ERR_FILE_CREATION], resNames);
			return QUIT_UPON_ERROR;
		}
		fclose(resNamesP);
		resNamesP = NULL;
		
		SEPERATOR
		/* if the given argument is a file path, we'll extract the file's name */
		if (strrchr(argv[index], '/') != NULL)
		{
			woExtension = (char *)malloc(strlen(strrchr(argv[index], '/')));
			if (woExtension == NULL)
			{
				err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
				break;
			}
			strcpy(woExtension, strrchr(argv[index], '/'));
			nameToPrint = woExtension+1;
			
			printf("Now processing \"%s.as\":\n", nameToPrint);
		}
		else
			printf("Now processing \"%s.as\":\n", argv[index]);

		ppRes = pre_process(argv[index], resNames);
		if (ppRes == QUIT_UPON_ERROR)
			continue;

		fpRes = first_pass(argv[index], resNames, codeImage, &dataImage, ocList, dirList, &head, &nlHead);
		spRes = second_pass(argv[index], resNames, codeImage, dataImage, ocList, dirList, &head, &nlHead, !((ppRes < 0) || (fpRes < 0)), fpRes);

		if (spRes == SUCCESS)
			printf("\nThe file was successfully processed with no errors detected.\n");
		else 
			printf("\nThe file processing is finished, please review the listed errors.\n");

		if (index != (argc - 1))
			printf("Moving to the next file.\n");
		else
		{	SEPERATOR
			printf("\nDone processing for all files.\n");
		}

		printf("\n");

		/* resetting data image */
		if (dataImage != NULL)
			free(dataImage);
		dataImage = NULL;

		/* resetting code image */
		reset_arr(codeImage, sizeof(codeImage));

		free_symbol_list(head);
		head = NULL;

		free_need_label_list(nlHead);
		nlHead = NULL;

		remove(resNames);

		if (woExtension != NULL)
                        free(woExtension);
	}

	return spRes;
}

/* ___Helper functions___ */

/* A function that builds a file containing reserved names based on opcode and directive lists.
   Parameters:
   - stage: The stage in which the error occurred.
   - resNames: The filename to create and write the reserved names to.
   - ocList: Array of opcode structures.
   - dirList: Array of directive structures.

   Returns:
   - Pointer to the file stream if the file was successfully created and written.
   - NULL if there was an error during file creation.
*/
FILE *build_res_names(const char *stage, char *resNames, Opcodes ocList[], Directives dirList[])
{
	int index;
	FILE *fileP;

	if ((fileP = fopen(resNames, "w")) == NULL)
	{
		err_wo_line(stage, asmblrErrList[ASMBLR_ERR_FILE_CREATION], resNames);
		return NULL;
	}
	for (index = FIRST_REG_NUM; index <= LAST_REG_NUM; index++)
	{
		fprintf(fileP, "r%d\n", index);
	}
	for (index = 0; index < Element_instructionEnd; index++)
	{
		fprintf(fileP, "%s\n", ocList[index].name);
	}
	for (index = 0; index < (Element_directiveEnd - 1 - Element_instructionEnd); index++)
	{
		fprintf(fileP, "%s\n", dirList[index].name);
	}
	return fileP;
}

/* Resets an array by replacing all elements with a placeholder value.
Parameters:
- arr: The array to be reset.
- arrSize: The size of the array.

Notes:
- Replaces all elements in the array with the placeholder value.
- Will be used to reset the codeImage array between files.
*/
void reset_arr(short int arr[], int arrSize)
{
	int index = 0;

	for (index = 0; index < arrSize; index++)
	{
		if (arr[index] == PLACEHOLDER)
			return;

		arr[index] = PLACEHOLDER;
	}
}

/* Frees memory allocated for a linked list of needLabelNode structures.
Parameters:
- head: Pointer to the head of the needLabelNode list.

Notes:
- Frees memory for each node in the linked list starting from the head.
*/
void free_need_label_list(needLabelNode *head)
{
	needLabelNode *current = head,
				  *nextNode;

	if (head == NULL)
		return;

	while (current->next != NULL)
	{
		nextNode = current->next; /* save the next pointer */
		free(current);			  /* free the current node */
		current = nextNode;		  /* move to the next node */
	}
	free(current);
}

/* Frees memory allocated for a linked list of symbolNode structures.
Parameters:
- head: Pointer to the head of the symbolNode list.

Notes:
- Frees memory for each node in the linked list starting from the head.
*/
void free_symbol_list(symbolNode *head)
{
	symbolNode *current = head,
			   *nextNode;

	if (head == NULL)
		return;

	while (current->next != NULL)
	{
		nextNode = current->next; /* save the next pointer */
		free(current);			  /* free the current node */
		current = nextNode;		  /* move to the next node */
	}
	free(current);
}
