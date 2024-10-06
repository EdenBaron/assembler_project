/* ___The pre processor's library___ */
#ifndef PRE_PROCESS_H
#define PRE_PROCESS_H

/* ___Include___ */
#include "general_lib.h"

/* ___Macros___ */
#define PP_CLOSE(head, addMcr, resNames)              \
    do                                                \
    {                                                 \
        if (head != NULL)                             \
            free_linked_list(head, addMcr, resNames); \
        if (ip != NULL)                               \
            fclose(ip);                               \
        if (op != NULL)                               \
            fclose(op);                               \
        if (ipName != NULL)                           \
            free(ipName);                             \
        if (opName != NULL)                           \
            free(opName);                             \
    } while (0);

#define PP_CLOSE_AND_REMOVE_AM          \
    do                                  \
    {                                   \
        remove(opName);                 \
        PP_CLOSE(head, FALSE, resNames) \
    } while (0);

/* ___Enums___ */
enum mcrDefFlag
{
    OUTSIDE_MCR,
    COUNTING_MCR_LINES,
    ADDING_MCR_LINES
};

enum ppErrIndex
{
    PP_ERR_LONG_LINE,        /* Line exceeds maximum length */
    PP_ERR_MISSING_MCR_NAME, /* Missing macro name after "mcr" command */
    PP_ERR_EXTRA_MCR_TEXT,   /* Extranous text after macro definition */
    PP_ERR_LONG_MCR_NAME,    /* mcr name exceeds maximum length */
    PP_ERR_MCR_NAME_RES,     /* detected an attempt to name a macro with a reserved name */
    PP_ERR_INVALID_MCR_NAME, /* Invalid macro name */
    PP_ERR_MCR_ADD,          /* Error adding macro to list */
    PP_ERR_EXTRA_ENDMCR_TEXT /* Extra text after "endmcr" command */
};

/*___Error list___ */
const char *ppErrList[] =
    {
        "The line is longer than 80 characters",
        "Missing macro name after 'mcr' command",
        "Detected extranous characters following the macro's name",
        "The following macro's name is longer than 31 characters",
        "The following macro's name is conflicting with a reserved name",
        "The following macro name is invalid",
        "Unsuccessful macro addition attempt for",
        "Detected extranous characters following the 'endmcr' command"};

/* ___Typedef___ */
typedef struct mcrNode
{
    char mcrName[MAX_LABEL_LENGTH + 1];
    char (*lines)[MAX_LINE_LENGTH + 1];
    int lineCount;
    struct mcrNode *next;
} mcrNode;

/* ___Prototypes___*/
int is_long_line(ERR_DETAILS_SIG, char *lineToCheck, int buffer, FILE *ip);
int valid_mcr(ERR_DETAILS_SIG, char *toCheck, char *resNames);
mcrNode *new_mcr(const char *stage, char *name, mcrNode *next);
int print_if_mcr(mcrNode *head, char *toCheck, FILE *op);
void free_linked_list(mcrNode *head, int saveMcrNames, char *resNames);

#endif
