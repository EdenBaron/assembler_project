#include "second_pass.h"

/* ___The second pass___ */
/* Returns:
   - 0 (SUCCESS): The second pass was complete with no errors.
   - -1 (QUIT_UPON_ERROR) : An error occurred throughout the program, and no output files wew made.
*/
int second_pass(char *baseName, char *resNames, short int codeImage[], short int *dataImage,
                Opcodes ocList[], Directives dirList[], symbolNode **symbolHead, needLabelNode **needLHead, int makeOutput, int IC)
{

    /* ___Declarations___ */

    /* symbols have been stored in a linked list */
    symbolNode *head = *symbolHead,
               *tempNode;

    needLabelNode *nlHead = *needLHead,
                  *current;

    FILE *ip = NULL,
         *ob = NULL,
         *ext = NULL,
         *ent = NULL;
         
    char *ipName = NULL,
         *obName = NULL,
         *extName = NULL,
         *entName = NULL,
         *baseNamePrint = NULL;

    int myDc = 0,
        i = 0,
        foundErrorFlag = (!makeOutput),
        extFlag = FALSE,
        entFlag = FALSE;

    short int value = 0;

    const char *stage = "second pass";

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
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }

    /* going over the linked list "need label" */

    /* the extern file will be created simultaniously, and deleted if an error was detected */
    if ((extName = add_ext(baseName, ".ext")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }
    if ((ext = fopen(extName, "w")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }

    /* for every node, we search for the label needed in the symbolNode list */
    /* If we find it, we put its value (address) in the codeImage array in its binary form */
    current = nlHead;
    while (current != NULL)
    {
        tempNode = is_symbol(head, current->labelName);
        if (tempNode == NULL)
        {
            err_with_line(stage, current->readInLine, ipName, spErrList[SP_ERR_LABEL_NOT_FOUND], current->labelName);
            foundErrorFlag = TRUE;
        }
        else if (tempNode->type == symbolType_extern)
        {
            /* for externs, the value is always 0 + its A, R, E value (always E)*/
            value |= tempNode->ARE;
            codeImage[current->IC] = value;

            /* writing the extern value to the .ext file */
            extFlag = TRUE;
            fprintf(ext, "%s\t%04d\n", current->labelName, current->IC);
            
        }
        else if (tempNode->type != symbolType_mdefine)
        {
            /* building the label's binary code */
            value |= tempNode->value << LABEL_ADD_MOVE;
            value |= tempNode->ARE;

            codeImage[current->IC] = value;
        }

        /* preparing for the next iteration */
        value = 0;
        current = current->next;
    }

    /* removing the .ext file if there was an error, or if there was no extern value */
    if (foundErrorFlag || !extFlag)
    {
        remove(extName);
    }

    /* proceeding only if there were no errors found */
    if (foundErrorFlag)
    {
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }

    /* making strings representing the names of the output files */
    /* object */
    if ((obName = add_ext(baseName, ".ob")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }
    /* entry */
    if ((entName = add_ext(baseName, ".ent")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_MALLOC], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }

    /* ___Opening the files___ */
    /* object */
    if ((ob = fopen(obName, "w")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }
    /* entry */
    if ((ent = fopen(entName, "w")) == NULL)
    {
        err_wo_line(stage, generalErrList[GEN_ERR_FOPEN], NULL);
        SP_CLOSE
        return QUIT_UPON_ERROR;
    }

    /* ___Making the object file___ */
    while (dataImage[myDc] < PLACEHOLDER)
    {
        myDc++; /* counting the amount of data words */
    }

    /* printing the .ob header */
    fprintf(ob, "  %d %d\n", IC, myDc);

    /* printing the instructions section to the .ob file */
    for (i = IMAGE_OFFSET; i < (IC + IMAGE_OFFSET); i++)
    {
        fprintf(ob, "%04d ", i);
        print_encoded_4(codeImage[i], ob);
    }

    /* printing the data section to the .ob file */
    i = 0;
    while (dataImage[i] < PLACEHOLDER)
    {
        fprintf(ob, "%04d ", (i + IC + IMAGE_OFFSET));
        print_encoded_4(dataImage[i], ob);
        i++;
    }
    if (strrchr(baseName, '/') != NULL)
    {   /* the base name is a file path */
        /* we will only print the file's name */
        baseName = strrchr(baseName, '/') + 1;
    }
    printf(">>> An Object file (.ob) was added to the directory.\n");

    /* ___Making the .ent file___ */
    entFlag = write_ent(head, ent);

    /* ___Removing the empty files____ */
    if (!extFlag)
        remove(extName);
    else
        printf(">>> An Extern file (.ext) was added to the directory.\n");
  
    if (!entFlag)
        remove(entName);
    else
        printf(">>> An Entry file (.ent) was added to the directory.\n");
    
    SP_CLOSE
    return SUCCESS;
}

/* ___Helper functions____ */

/* Prints the encoded representation of a 14-bit long memory word using a custom 4-base encoding scheme.
   Parameters:
   - toPrint: The short integer to be encoded and printed.
   - ob: Pointer to the FILE object where the encoded representation will be printed.

   Explanation:
   This function takes a short integer and prints its encoded representation using a custom 4-base encoding scheme.
   Each 2 bits of the input integer are represented by one of four symbols: '*', '#', '%', or '!'.
   The encoded symbols are printed to the specified file object 'ob'.

   For example:
   - Binary 00 is represented as '*'.
   - Binary 01 is represented as '#'.
   - Binary 10 is represented as '%'.
   - Binary 11 is represented as '!'.

   The function processes the input integer bitwise, printing the encoded symbols one by one from right to left to the .ob file.
*/
void print_encoded_4(short int toPrint, FILE *ob)
{
    int res,
        index;

    char code[WORD_LENGTH_4_ENCODED + 1] = "";

    for (index = WORD_LENGTH_4_ENCODED - 1; index >= 0; index--)
    {
        /* proccessing every 2 rightmost bits at a time */
        res = MASK & toPrint;
        switch (res)
        {
        case 0:
        {
            code[index] = '*';
            break;
        }
        case 1:
        {
            code[index] = '#';
            break;
        }
        case 2:
        {
            code[index] = '%';
            break;
        }
        case 3:
        {
            code[index] = '!';
            break;
        }
        default:
            break;
        }
        /* discard the printed bits and move to the next two */
        toPrint = toPrint >> AFTER_PRINT_MOVE;
    }
    fprintf(ob, "%s\n", code);
}

int write_ent(symbolNode *head, FILE *ent)
{
    symbolNode *current = head;

    int entFlag = FALSE;

    while (current != NULL)
    {
        if (current->type == symbolType_entry)
        {
            entFlag = TRUE;
            fprintf(ent, "%s\t%04d\n", current->symbolName, current->value);
        }
        current = current->next;
    }
    return entFlag;
}

