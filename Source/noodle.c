#include <stdio.h>

#include "noodle.h"

////////////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////////////

#define NOODLE_GROUP_BUCKETS_COUNT 16

////////////////////////////////////////////////////////////////////////////////
// STRUCTURE DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

typedef struct Noodle_t
{
    NoodleType_t        type;
    struct Noodle_t*    pParent;
    char*               pName; // Must be freed
} Noodle_t;

typedef struct NoodleNode_t
{
    struct NoodleNode_t* pNext;
    Noodle_t* pNoodle;
} NoodleNode_t;

typedef struct NoodleGroup_t
{
    Noodle_t        base;
    size_t          count;
    size_t          bucketCount;
    NoodleNode_t*   ppBuckets[NOODLE_GROUP_BUCKETS_COUNT]; // Each must be freed
} NoodleGroup_t;

typedef struct NoodleValue_t
{
    Noodle_t    base;
    union       value
    {
        int     i;
        float   f;
        bool    b;
        char*   s; // Must be freed
    };
} NoodleValue_t;

typedef struct NoodleArray_t
{
    Noodle_t base;
    NoodleType_t type;
    size_t count;
    void* pValues; // Must be freed
} NoodleArray_t;

typedef enum NoodleTokenKind_t
{
    NOODLE_TOKEN_KIND_UNEXPECTED,
    NOODLE_TOKEN_KIND_IDENTIFIER,
    NOODLE_TOKEN_KIND_INTEGER,
    NOODLE_TOKEN_KIND_FLOAT,
    NOODLE_TOKEN_KIND_BOOLEAN,
    NOODLE_TOKEN_KIND_STRING,
    NOODLE_TOKEN_KIND_LEFTCURLY,
    NOODLE_TOKEN_KIND_RIGHTCURLY,
    NOODLE_TOKEN_KIND_LEFTBRACKET,
    NOODLE_TOKEN_KIND_RIGHTBRACKET,
    NOODLE_TOKEN_KIND_EQUAL,
    NOODLE_TOKEN_KIND_COMMA,
    NOODLE_TOKEN_KIND_END,
    NOODLE_TOKEN_KIND_COMMENT,
    NOODLE_TOKEN_KIND_NEWLINE,
} NoodleTokenKind_t;

typedef struct NoodleToken_t
{
    NoodleTokenKind_t kind;
    int start;
    int end;
} NoodleToken_t;

typedef struct NoodleLexer_t
{
    const char* pContent;
    size_t contentSize;
    int current;
} NoodleLexer_t;

////////////////////////////////////////////////////////////////////////////////
// ERROR STRING DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

static const char* e_memFailure = "Could not allocate memory!";
static const char* e_unexpectedToken = "Unexpected token found, \"%s\", expected token, \"%s\"!";

////////////////////////////////////////////////////////////////////////////////
// HELPER DECLARATIONS
////////////////////////////////////////////////////////////////////////////////

char*           noodleStrdup(const char* str);
void            noodleRemoveSpacesAndComments(char* pContent);
const char*     noodleTokenKindToString(NoodleTokenKind_t kind);

NoodleGroup_t*  noodleMakeGroup(char* pName, NoodleGroup_t* pParent);
NoodleArray_t*  noodleMakeArray(char* pName, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeIntValue(char* pName, int value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeFloatValue(char* pName, float value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeBoolValue(char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeStringValue(char* pName, char* value, NoodleGroup_t* pParent);

size_t          noodleGroupHashFunction(const char* pName);
NOODLE_BOOL     noodleGroupInsert(NoodleGroup_t* pGroup, const char* pName, Noodle_t* pNoodle);

NOODLE_BOOL     noodleLexerIsIdentifier(char c);
NOODLE_BOOL     noodleLexerIsNumber(char c);
NOODLE_BOOL     noodleLexerIsSpace(char c);
NoodleLexer_t   noodleLexer(const char* pContent, size_t contentSize);
char            noodleLexerGet(NoodleLexer_t* pLexer);
char            noodleLexerPeek(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerNext(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerMakeAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind);
NoodleToken_t   noodleLexerMakeIdentifier(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerMakeNumber(NoodleLexer_t* pLexer);


////////////////////////////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

NoodleGroup_t* noodleParse(const char* pUserContent, char* pErrorBuffer, size_t bufferSize)
{
    if (pUserContent == NULL)
    {
        snprintf(pErrorBuffer, bufferSize, e_memFailure);
        return NULL;
    }

    if (pErrorBuffer != NULL && bufferSize > 0 )
    {
        memset(pErrorBuffer, '\0', bufferSize);
        
        // Allow for at least one null-terminator always
        bufferSize--;
    }
    else
    {
        // Make sure that the strncpy's do nothing
        pErrorBuffer = NULL;
        bufferSize = 0;
    }

    // Create a copy of the code to remove comments
    char* pContent = noodleStrdup(pUserContent);
    if (pContent == NULL)
    {
        snprintf(pErrorBuffer, bufferSize, e_memFailure);
        return NULL;
    }

    noodleRemoveSpacesAndComments(pContent);
    size_t contentSize = strlen(pContent);

    printf("%s", pContent);

    // Create the root group to contain the other noodles
    NoodleGroup_t* pRoot = noodleMakeGroup("", NULL);
    
    // Create the lexer and begin parsing
    NoodleLexer_t lexer = noodleLexer(pContent, contentSize);
    NoodleToken_t token = noodleLexerNext(&lexer);
    
    NoodleGroup_t* pCurrent = pRoot;

    while (token.kind != NOODLE_TOKEN_KIND_END)
    {
        
        // Get the identifier
        if (token.kind != NOODLE_TOKEN_KIND_IDENTIFIER)
        {
            snprintf(
                pErrorBuffer, bufferSize, 
                e_unexpectedToken, noodleTokenKindToString(token.kind), "identifier");
            noodleFree((Noodle_t*)pRoot);
            return NULL;
        }

        // Create the identifier string
        int identifierLength = token.end - token.start;
        char* pIdentifier = NOODLE_MALLOC(identifierLength + 1);
        if (!pIdentifier)
        {
            snprintf(pErrorBuffer, bufferSize, e_memFailure);
            noodleFree((Noodle_t*)pRoot);
            return NULL;
        }

        memcpy(pIdentifier, pContent + token.start, identifierLength);

        // Get the equals token
        token = noodleLexerNext(&lexer);

        if (token.kind != NOODLE_TOKEN_KIND_EQUAL)
        {
            snprintf(
                pErrorBuffer, bufferSize, 
                e_unexpectedToken, noodleTokenKindToString(token.kind), "equal symbol");
            noodleFree((Noodle_t*)pRoot);
            return NULL;
        }

        // Next token determines type
        token = noodleLexerNext(&lexer);

        switch (token.kind)
        {
            case NOODLE_TOKEN_KIND_INTEGER:
            {
                int value = (int)strtol(pContent + token.start, NULL, 10);
                noodleGroupInsert(pCurrent, pIdentifier, noodleMakeIntValue(pIdentifier, value, pCurrent));
                break;
            }
            case NOODLE_TOKEN_KIND_FLOAT:
            {
                float value = strtof(pContent + token.start, NULL);
                noodleGroupInsert(pCurrent, pIdentifier, noodleMakeFloatValue(pIdentifier, value, pCurrent));
                break;
            }
            case NOODLE_TOKEN_KIND_BOOLEAN:
            {
                
            }
        }

    }

    return pRoot;
}

NoodleGroup_t*          noodleGroupFrom(const NoodleGroup_t* group, const char* name);
int                     noodleIntFrom(const NoodleGroup_t* group, const char* name);
float                   noodleFloatFrom(const NoodleGroup_t* group, const char* name);
NOODLE_BOOL             noodleBoolFrom(const NoodleGroup_t* group, const char* name);
const char*             noodleStringFrom(const NoodleGroup_t* group, const char* name);
const NoodleArray_t*    noodleArrayFrom(const NoodleGroup_t* group, const char* name);
size_t                  noodleCount(const Noodle_t* noodle);
int                     noodleIntAt(const NoodleArray_t* array, size_t index);
float                   noodleFloatAt(const NoodleArray_t* array, size_t index);
NOODLE_BOOL             noodleBoolAt(const NoodleArray_t* array, size_t index);
const char*             noodleStringAt(const NoodleArray_t* array, size_t index);

void noodleFree(Noodle_t* pNoodle)
{
    switch (pNoodle->type)
    {
        case NOODLE_TYPE_GROUP:
            NoodleGroup_t* pGroup = (NoodleGroup_t*)pNoodle;

            for (int i = 0; i < pGroup->bucketCount; i++)
            {

            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTION DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

char* noodleStrdup(const char* pStr)
{
    size_t length = strlen(pStr) + 1;
    char* pNewStr = NOODLE_MALLOC(length);

    if (pNewStr == NULL)
        return NULL;

    return memcpy(pNewStr, pStr, length);
}

void noodleRemoveSpacesAndComments(char* pContent)
{
    char* pD = pContent;
    NOODLE_BOOL isString = false;

    do
    {
        // Only remove if they aren't in a string
        if (!isString)
        {
            // Remove comments until a \n
            if (*pD == '#')
            {
                while (*pD != '\0' || *pD != '\n')
                {
                    ++pD;
                }
            }

            // Remove all spaces
            while(noodleLexerIsSpace(*pD))
            {
                ++pD;
            }
        }

        // Do not remove spaces in quotes
        if (*pD == '\"')
        {
            isString = !isString;
        }

    } while (*pContent++ = *pD++);
    
    *pD = 0;
}

NoodleGroup_t* noodleMakeGroup(char* pName, Noodle_t* pParent)
{
    // Allocate the group and cast to the noodle base composition
    NoodleGroup_t* pGroup = NOODLE_MALLOC(sizeof(NoodleGroup_t));
    Noodle_t* pNoodle = (Noodle_t*)pGroup;
    if (!pGroup)
    {
        return NULL;
    }

    memset(pGroup, 0, sizeof(NoodleGroup_t));

    // Set the values
    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_GROUP;
    pGroup->bucketCount = NOODLE_GROUP_BUCKETS_COUNT;

    return pGroup;
}

NoodleArray_t* noodleMakeArray(char* pName, Noodle_t* pParent)
{
    NoodleArray_t* pArray = NOODLE_MALLOC(sizeof(NoodleArray_t));
    Noodle_t* pNoodle = (Noodle_t*)pArray;
    if (!pArray)
    {
        return NULL;
    }

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_ARRAY;

}

NoodleValue_t* noodleMakeIntValue(char* pName, int value, NoodleGroup_t* pParent)
{
    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue)
    {
        return NULL;
    }

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_INTEGER;
    pValue->i = value;

}

NoodleValue_t*  noodleMakeFloatValue(const char* pName, float value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeBoolValue(const char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeStringValue(const char* pName, char* value, NoodleGroup_t* pParent);

NOODLE_BOOL noodleGroupInsert(NoodleGroup_t* pGroup, const char* pName, Noodle_t* pNoodle)
{
    // Allocate a noodle group node
    NoodleNode_t* pNode = NOODLE_MALLOC(sizeof(NoodleNode_t));
    if (!pNode)
    {
        return false;
    }

    pNode->pNext = NULL;
    pNode->pNoodle = pNoodle;

    // Hash the name to get an index
    size_t index = noodleGroupHashFunction(pName) % pGroup->bucketCount;

    // Add this node to the last value in the bucket
    NoodleNode_t* pCurrent = pGroup->ppBuckets[index];
    
    if (!pCurrent)
    {
        pGroup->ppBuckets[index] = pNode;
        return true;
    }

    while (pCurrent->pNext)
    {
        pCurrent = pCurrent->pNext; 
    }

    pCurrent->pNext = pNode;
    return true;
}

const char* noodleTokenKindToString(NoodleTokenKind_t kind)
{
    switch (kind)
    {
        case NOODLE_TOKEN_KIND_IDENTIFIER: return "NOODLE_TOKEN_KIND_IDENTIFIER"; 
        case NOODLE_TOKEN_KIND_INTEGER: return "NOODLE_TOKEN_KIND_INTEGER"; 
        case NOODLE_TOKEN_KIND_FLOAT: return "NOODLE_TOKEN_KIND_FLOAT"; 
        case NOODLE_TOKEN_KIND_BOOLEAN: return "NOODLE_TOKEN_KIND_BOOLEAN"; 
        case NOODLE_TOKEN_KIND_STRING: return "NOODLE_TOKEN_KIND_STRING"; 
        case NOODLE_TOKEN_KIND_LEFTCURLY: return "NOODLE_TOKEN_KIND_LEFTCURLY"; 
        case NOODLE_TOKEN_KIND_RIGHTCURLY: return "NOODLE_TOKEN_KIND_RIGHTCURLY"; 
        case NOODLE_TOKEN_KIND_LEFTBRACKET: return "NOODLE_TOKEN_KIND_LEFTBRACKET"; 
        case NOODLE_TOKEN_KIND_RIGHTBRACKET: return "NOODLE_TOKEN_KIND_RIGHTBRACKET"; 
        case NOODLE_TOKEN_KIND_EQUAL: return "NOODLE_TOKEN_KIND_EQUAL"; 
        case NOODLE_TOKEN_KIND_COMMA: return "NOODLE_TOKEN_KIND_COMMA"; 
        case NOODLE_TOKEN_KIND_END: return "NOODLE_TOKEN_KIND_END"; 
        case NOODLE_TOKEN_KIND_COMMENT: return "NOODLE_TOKEN_KIND_COMMENT"; 
        case NOODLE_TOKEN_KIND_NEWLINE: return "NOODLE_TOKEN_KIND_NEWLINE"; 
        
        default:
        case NOODLE_TOKEN_KIND_UNEXPECTED: return "NOODLE_TOKEN_KIND_UNEXPECTED"; 
    }
}

NoodleLexer_t noodleLexer(const char* pContent, size_t contentSize)
{
    return (NoodleLexer_t){pContent, contentSize, 0};
}

static NOODLE_BOOL noodleLexerIsIdentifier(char c)
{
    switch (c)
    {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
            return NOODLE_TRUE;
        default:
            return NOODLE_FALSE;
    }
}

NOODLE_BOOL noodleLexerIsNumber(char c)
{
    switch (c)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            return NOODLE_TRUE;
        default:
            return NOODLE_FALSE;
    }
}

NOODLE_BOOL noodleLexerIsSpace(char c)
{
    switch (c)
    {
        case ' ':
        case '\n':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
            return NOODLE_TRUE;
        default:
            return NOODLE_FALSE;
    }
}

NoodleToken_t noodleLexerNext(NoodleLexer_t* pLexer)
{
    switch(noodleLexerPeek(pLexer))
    {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
            return noodleLexerMakeIdentifier(pLexer);
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return noodleLexerMakeNumber(pLexer);
        case '=':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_EQUAL);
        case '{':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTBRACKET);
        case '}':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTBRACKET);
        
        
        case '\0':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_END);
        default:
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_UNEXPECTED);
    }
}

char noodleLexerGet(NoodleLexer_t* pLexer)
{
    return pLexer->pContent[pLexer->current++];
}

char noodleLexerPeek(NoodleLexer_t* pLexer)
{
    return pLexer->pContent[pLexer->current + 1];
}

NoodleToken_t noodleLexerMakeAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind)
{
    return (NoodleToken_t){ kind, pLexer->current, pLexer->current + 1};
}

NoodleToken_t noodleLexerMakeIdentifier(NoodleLexer_t* pLexer)
{
    int start = pLexer->current;

    while(noodleLexerIsIdentifier(noodleLexerGet(pLexer))) {}

    return (NoodleToken_t){ NOODLE_TOKEN_KIND_IDENTIFIER, start, pLexer->current };
}

NoodleToken_t noodleLexerMakeNumber(NoodleLexer_t* pLexer)
{
    int start = pLexer->current;
    const char* pStart = pLexer->pContent;
    const char* pEnd = pLexer->pContent;

    while(noodleLexerIsNumber(noodleLexerGet(pLexer)))
    {
        pEnd++;
    }

    // Determine if this string is a long or a float, or a failed case
    char* pFloatEnd = NULL;
    char* pLongEnd = NULL;

    strtof(pStart, &pFloatEnd);
    strtol(pStart, &pLongEnd, 10);

    NoodleTokenKind_t kind = NOODLE_TOKEN_KIND_UNEXPECTED;

    if (pEnd == pLongEnd)
        kind = NOODLE_TOKEN_KIND_INTEGER;
    else if (pEnd == pFloatEnd)
        kind = NOODLE_TOKEN_KIND_FLOAT;

    return (NoodleToken_t){ kind, start, pLexer->current };
}



