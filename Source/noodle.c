#include <stdio.h>
#include <assert.h>

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
    NoodleGroup_t*    pParent;
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
    union
    {
        int         i;
        float       f;
        NOODLE_BOOL b;
        char*       s; // Must be freed
    };
} NoodleValue_t;

typedef struct NoodleElement_t
{
    union
    {
        int         i;
        float       f;
        NOODLE_BOOL b;
        char*       s; // Must be freed
    };
    struct NoodleElement_t* pNext;
} NoodleElement_t;

typedef struct NoodleArray_t
{
    Noodle_t        base;
    NoodleType_t    type;
    NoodleElement_t* pFirst;
    NoodleElement_t* pLast;
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
    int current;
    int line;
    int character;
} NoodleLexer_t;

////////////////////////////////////////////////////////////////////////////////
// HELPER DECLARATIONS
////////////////////////////////////////////////////////////////////////////////

char*           noodleStrdup(const char* str);
void            noodleRemoveSpacesAndComments(char* pContent);
const char*     noodleTokenKindToString(NoodleTokenKind_t kind);

NoodleGroup_t*  noodleParserHandleGroup(char* pName, NoodleGroup_t* pParent);
NoodleArray_t*  noodleMakeArray(char* pName, NoodleType_t type, NoodleGroup_t* pParent);
NoodleValue_t*  noodleParserHandleInt(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent);
NoodleValue_t*  noodleParserHandleFloat(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeBoolValue(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeStringValue(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent);

size_t          noodleGroupHashFunction(const char* pName);
NOODLE_BOOL     noodleGroupInsert(NoodleGroup_t* pGroup, const char* pName, Noodle_t* pNoodle);

NOODLE_BOOL     noodleArrayPushBack(NoodleArray_t* pArray, void* pValue);

NOODLE_BOOL     noodleLexerIsIdentifier(char c);
NOODLE_BOOL     noodleLexerIsNumber(char c);
NOODLE_BOOL     noodleLexerIsSpace(char c);
NoodleLexer_t   noodleLexer(const char* pContent);
char            noodleLexerGet(NoodleLexer_t* pLexer);
char            noodleLexerPeek(NoodleLexer_t* pLexer);
void            noodleLexerSkipComment(NoodleLexer_t* pLexer);
void            noodleLexerSkipSpaces(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerNext(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerMakeAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind);
NoodleToken_t   noodleLexerMakeIdentifier(NoodleLexer_t* pLexer);
NoodleToken_t   noodleLexerMakeNumber(NoodleLexer_t* pLexer);


////////////////////////////////////////////////////////////////////////////////
// FUNCTION DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

NoodleGroup_t* noodleParse(const char* pContent, char* pErrorBuffer, size_t bufferSize)
{
    if (!pContent) goto cleanupArgument;

    if (pErrorBuffer && bufferSize > 1 )
    {
        memset(pErrorBuffer, '\0', bufferSize);
        bufferSize--; // Allow for at least one null-terminator
    }
    else
    {
        // If the user doesn't want to receive errors, do nothing
        pErrorBuffer = NULL;
        bufferSize = 0;
    }

    printf("%s", pContent); // DEBUG

    // Create the root group to contain the other noodles
    NoodleGroup_t* pRoot = noodleParserHandleGroup(NULL, NULL);
    if (!pRoot) goto cleanupMemory;
    
    // Create the lexer and begin parsing
    NoodleLexer_t lexer = noodleLexer(pContent);
    NoodleToken_t token = noodleLexerNext(&lexer);

    const char* pErrorExpected = ""; // On failure use this value is set to hint what the error is
    NoodleGroup_t* pCurrent = pRoot; // Used to parent noodles

    while (token.kind != NOODLE_TOKEN_KIND_END)
    {
        
        if (token.kind != NOODLE_TOKEN_KIND_IDENTIFIER)
        {
            pErrorExpected = "Identifier";
            goto cleanupParse;
        }

        // Create the noodle's identifier string
        int identifierLength = token.end - token.start + 1; // Convert indexes into counts
        char* pIdentifier = NOODLE_MALLOC(identifierLength + 1);
        if (!pIdentifier) goto cleanupMemory;

        // Set the identifier string's contents
        memset(pIdentifier, 0, identifierLength + 1);
        memcpy(pIdentifier, pContent + token.start, identifierLength);

        // Get the equals token
        token = noodleLexerNext(&lexer);

        if (token.kind != NOODLE_TOKEN_KIND_EQUAL)
        {
            pErrorExpected = "Equals Symbol";
            goto cleanupParse;
        }

        // This next token will determine the type of noodle to create
        token = noodleLexerNext(&lexer);

        Noodle_t* pNewNoodle = NULL;

        switch (token.kind)
        {
            case NOODLE_TOKEN_KIND_LEFTCURLY:
            {
                NoodleGroup_t* pGroup = noodleParserHandleGroup(pIdentifier, pRoot);
                pNewNoodle = (Noodle_t*)pGroup;
                pCurrent = pGroup;
                break;
            }

            case NOODLE_TOKEN_KIND_INTEGER:
            {   
                pNewNoodle = (Noodle_t*)noodleParserHandleInt(pIdentifier, &lexer, &token, pCurrent);
                break;
            }

            case NOODLE_TOKEN_KIND_FLOAT:
            {   
                pNewNoodle = (Noodle_t*)noodleParserHandleFloat(pIdentifier, &lexer, &token, pCurrent);
                break;
            }

            case NOODLE_TOKEN_KIND_BOOLEAN:
            {
                pNewNoodle = (Noodle_t*)noodleMakeBoolValue(pIdentifier, &lexer, &token, pCurrent);
                break;
            }

            case NOODLE_TOKEN_KIND_STRING:
            {
                pNewNoodle = noodleMakeStringValue(pIdentifier, &lexer, &token, pCurrent);
                break;
            }

            case NOODLE_TOKEN_KIND_LEFTBRACKET:
            {

                NoodleArray_t* pArray = NULL; // TODO
                
                // Get the next token to get it's type and ensure it can be in an array 
                token = noodleLexerNext(pLexer);

                if (token.kind != NOODLE_TOKEN_KIND_INTEGER && 
                    token.kind != NOODLE_TOKEN_KIND_FLOAT &&
                    token.kind != NOODLE_TOKEN_KIND_BOOLEAN &&
                    token.kind != NOODLE_TOKEN_KIND_STRING)
                {
                    pErrorExpected = "Integer, Float, Boolean, or String";
                    goto cleanupParse;
                }

                // Get the next few tokens until the end of the array
                NoodleTokenKind_t expected = token.kind;
                int arrayStart = token.start;
                int arrayCount = 0;

                // Loop through all the tokens of the array to get the count and verification of type
                while (token.kind != NOODLE_TOKEN_KIND_RIGHTBRACKET)
                {
                    // Make sure that the token is the one we expect in the array
                    if (token.kind != expected)
                    {
                        pErrorExpected = noodleTokenKindToString(expected);
                        goto cleanupParse;
                    }

                    arrayCount++;

                    // Expect a ',' or a ']'
                    token = noodleLexerNext(&lexer);

                    if (token.kind == NOODLE_TOKEN_KIND_COMMA)
                    {
                        token = noodleLexerNext(&lexer);
                        continue;
                    }
                }

                // Kinda hacky but reset the lexer back to the array start position
                lexer.current = arrayStart;

                // Allocate the array of values
                union
                {
                    int* pI;
                    float* pF;
                    NOODLE_BOOL* pB;
                    char** ppS;
                } array;

                switch (expected)
                {
                    case NOODLE_TOKEN_KIND_INTEGER: 
                        pArray->type = NOODLE_TYPE_INTEGER;
                        p
                }

                // Iterate through the values once again, verified and with a count
                while (token.kind != NOODLE_TOKEN_KIND_RIGHTBRACKET)
                {
                    
                }
                
            }
        }

        if (!pNewNoodle)
            goto cleanupMemory;

        if (!noodleGroupInsert(pCurrent, pIdentifier, pNewNoodle)) 
            goto cleanupMemory;
        


    }

    return pRoot;

cleanupArgument:
    snprintf(pErrorBuffer, bufferSize, "Invalid argument!");
    return NULL;

cleanupMemory:
    snprintf(pErrorBuffer, bufferSize, "Could not allocate memory!");
    noodleFree((Noodle_t*)pRoot);
    return NULL;

cleanupParse:
    snprintf(pErrorBuffer, bufferSize, "Unexpected token found, \"%.*s\", expected token, \"%s\"!", token.end - token.start, pContent, pErrorExpected);
    noodleFree((Noodle_t*)pRoot);
    return NULL;

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

NoodleGroup_t* noodleParserHandleGroup(char* pName, NoodleGroup_t* pParent)
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

NoodleArray_t* noodleMakeArray(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleArray_t* pArray = NOODLE_MALLOC(sizeof(NoodleArray_t));
    Noodle_t* pNoodle = (Noodle_t*)pArray;
    if (!pArray)
    {
        return NULL;
    }

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_ARRAY;
    pArray->type = type;
    pArray->pFirst = NULL;
    pArray->pLast = NULL;

    
    return pArray;
}

NoodleValue_t* noodleParserHandleInt(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent)
{
    assert(pName);
    assert(pToken);
    assert(pToken->kind == NOODLE_TOKEN_KIND_INTEGER);

    int value = (int)strtol(pLexer->pContent + pToken->start, NULL, 10);

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

    return pValue;
}

NoodleValue_t* noodleParserHandleFloat(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent)
{
    assert(pName);

    float value = strtof(pLexer->pContent + pToken->start, NULL);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_FLOAT;
    pValue->f = value;

    return pValue;
}

NoodleValue_t* noodleMakeBoolValue(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent)
{
    assert(pName);
    assert(pLexer);
    assert(pToken);
    assert(pToken->kind == NOODLE_TOKEN_KIND_BOOLEAN);

    bool value = (*(pLexer->pContent + pToken->start) == 't') ? NOODLE_TRUE : NOODLE_FALSE;

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_BOOLEAN;
    pValue->b = value;

    return pValue;
}

NoodleValue_t* noodleMakeStringValue(char* pName, NoodleLexer_t* pLexer, NoodleToken_t* pToken, NoodleGroup_t* pParent)
{
    assert(pName);

    // Create the noodle's string
    size_t stringLength = pToken->end - pToken->start; 
    char* pString = NOODLE_MALLOC(stringLength + 1);

    if (!pString) return NULL;

    // Set the string's contents
    memset(pString, 0, stringLength + 1);
    memcpy(pString, pLexer->pContent + pToken->start, stringLength);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_STRING;
    pValue->s = pString;

    return pValue;
}

size_t noodleGroupHashFunction(const char* pName)
{
    // Implementation of sdbm
    size_t hash = 0;
    int c;

    while (c = *pName++)
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

NOODLE_BOOL noodleGroupInsert(NoodleGroup_t* pGroup, const char* pName, Noodle_t* pNoodle)
{
    assert(pGroup && pName && pNoodle);

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

NOODLE_BOOL noodleArrayAdd(NoodleArray_t* pArray, void* pValue)
{
    assert(pArray && pValue);

    // Allocate the noodle element
    NoodleElement_t* pElement = NOODLE_MALLOC(sizeof(NoodleElement_t));
    if (!pElement) return NOODLE_FALSE;

    switch (pArray->type)
    {
        case NOODLE_TYPE_INTEGER:
            pElement->i = *((int*)pValue);
            break;
        case NOODLE_TYPE_FLOAT:
            pElement->f = *((float*)pValue);
            break;
        case NOODLE_TYPE_BOOLEAN:
            pElement->b = *((NOODLE_BOOL*)pValue);
            break;
        case NOODLE_TYPE_STRING:
            pElement->s = (char*)pValue;
            break;
    }

    // Add the element to the end of the linked list
    if (pArray->pLast)
    {
        pArray->pLast->pNext = pElement;
    }
    else
    {
        pArray->pFirst = pElement;
        pArray->pLast = pElement;
    }

    return NOODLE_TRUE;
}

const char* noodleTokenKindToString(NoodleTokenKind_t kind)
{
    switch (kind)
    {
        case NOODLE_TOKEN_KIND_IDENTIFIER: return "Identifier"; 
        case NOODLE_TOKEN_KIND_INTEGER: return "Integer"; 
        case NOODLE_TOKEN_KIND_FLOAT: return "Float"; 
        case NOODLE_TOKEN_KIND_BOOLEAN: return "Boolean"; 
        case NOODLE_TOKEN_KIND_STRING: return "String"; 
        case NOODLE_TOKEN_KIND_LEFTCURLY: return "Left Curly"; 
        case NOODLE_TOKEN_KIND_RIGHTCURLY: return "Right Curly"; 
        case NOODLE_TOKEN_KIND_LEFTBRACKET: return "Left Bracket"; 
        case NOODLE_TOKEN_KIND_RIGHTBRACKET: return "Right Bracket"; 
        case NOODLE_TOKEN_KIND_EQUAL: return "Equal"; 
        case NOODLE_TOKEN_KIND_COMMA: return "Comma"; 
        case NOODLE_TOKEN_KIND_END: return "End"; 
        case NOODLE_TOKEN_KIND_COMMENT: return "Comment"; 
        case NOODLE_TOKEN_KIND_NEWLINE: return "Newline"; 
        
        default:
        case NOODLE_TOKEN_KIND_UNEXPECTED: return "Unexpected"; 
    }
}

NoodleLexer_t noodleLexer(const char* pContent)
{
    return (NoodleLexer_t){pContent, 0, 0, 0};
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
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_LEFTCURLY);
        case '}':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTCURLY);
        case '#':
            noodleLexerSkipComment(pLexer);
            return noodleLexerNext(pLexer);
        case ' ':
        case '\n':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
            noodleLexerSkipSpaces(pLexer);
            return noodleLexerNext(pLexer);
        case '\0':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_END);
        default:
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_UNEXPECTED);
    }
}

char noodleLexerGet(NoodleLexer_t* pLexer)
{
    char c = pLexer->pContent[pLexer->current++];

    // Correctly increase line counts and character numbers
    if (c == '\n')
    {
        pLexer->line++;
        pLexer->character = 0;
    }
    else
    {
        pLexer->character++;
    }
 
    return c;
}

char noodleLexerPeek(NoodleLexer_t* pLexer)
{
    return pLexer->pContent[pLexer->current + 1];
}

void noodleLexerSkipComment(NoodleLexer_t* pLexer)
{
    char c = 0;
    do
    {
        c = noodleLexerGet(pLexer);
    } while (c != '\0' && c != '\n');
}

void noodleLexerSkipSpaces(NoodleLexer_t* pLexer)
{
    char c = noodleLexerPeek(pLexer);
    while (c!= '\0' && noodleLexerIsSpace(c))
    {
        c = noodleLexerGet(pLexer);
    }
}

NoodleToken_t noodleLexerMakeAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind)
{
    const NoodleToken_t token = { kind, pLexer->current, pLexer->current + 1};
    noodleLexerGet(pLexer);
    return token;
}

NoodleToken_t noodleLexerMakeIdentifier(NoodleLexer_t* pLexer)
{
    int start = pLexer->current;

    while(noodleLexerIsIdentifier(noodleLexerPeek(pLexer))) 
    {
        noodleLexerGet(pLexer);
    }

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



