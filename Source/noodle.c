#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "noodle.h"


////////////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////////////



#define NOODLE_GROUP_BUCKETS_COUNT 16



////////////////////////////////////////////////////////////////////////////////
// STRUCTURE DEFINITIONS
////////////////////////////////////////////////////////////////////////////////



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
    int             count;
    union
    {
        int*        pIntegers;
        float*      pFloats;
        NOODLE_BOOL* pBooleans;
        char**      ppStrings;
    };
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




char*           noodleStringDuplicate(const char* str);
const char*     noodleStringFromTokenKind(NoodleTokenKind_t kind);

NoodleToken_t   noodleToken(NoodleTokenKind_t kind, int start, int end);

char            noodleLexerGet(NoodleLexer_t* pLexer);
char            noodleLexerPeek(const NoodleLexer_t* pLexer);
void            noodleLexerSkipComment(NoodleLexer_t* pLexer);
void            noodleLexerSkipSpaces(NoodleLexer_t* pLexer);
void            noodleLexerAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind, NoodleToken_t* pToken);
void            noodleLexerIdentifierOrBool(NoodleLexer_t* pLexer, NoodleToken_t* pToken);
void            noodleLexerNumber(NoodleLexer_t* pLexer, NoodleToken_t* pToken);
void            noodleLexerString(NoodleLexer_t* pLexer, NoodleToken_t* pToken);

NOODLE_BOOL     noodleLexerIsIdentifier(char);
NOODLE_BOOL     noodleLexerIsNumber(char);
NOODLE_BOOL     noodleLexerIsSpace(char);

NoodleLexer_t   noodleLexer(const char* pContent);
NOODLE_BOOL     noodleLexerNextToken(NoodleLexer_t* pLexer, NoodleToken_t* pToken);

int             noodleParseInt(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken);
float           noodleParseFloat(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken);
NOODLE_BOOL     noodleParseBool(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken);
char*           noodleParseString(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken);

NoodleGroup_t*  noodleGroup(char* pName, NoodleGroup_t* pParent);
NoodleArray_t*  noodleArray(char* pName, NoodleType_t type, NoodleGroup_t* pParent);
NoodleValue_t*  noodleValue(char* pName, NoodleType_t type, NoodleGroup_t* pParent);
NoodleValue_t*  noodleInt(char* pName, int value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleFloat(char* pName, float value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleBool(char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleString(char* pName, char* value, NoodleGroup_t* pParent);

size_t          noodleGroupHashFunction(const char* pName);
NOODLE_BOOL     noodleGroupInsert(NoodleGroup_t* pGroup, const char* pName, Noodle_t* pNoodle);




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
    NoodleGroup_t* pRoot = noodleGroup(NULL, NULL);
    if (!pRoot) goto cleanupMemory;
    
    // Create the lexer and begin parsing
    NoodleLexer_t lexer = noodleLexer(pContent);
    NoodleToken_t token = {0};

    noodleLexerNextToken(&lexer, &token);

    const char* pErrorExpected = ""; // On failure use this value is set to hint what the error is
    NoodleGroup_t* pCurrent = pRoot; // Used to parent noodles

    while (token.kind != NOODLE_TOKEN_KIND_END)
    {
        
        if (token.kind != NOODLE_TOKEN_KIND_IDENTIFIER)
        {
            pErrorExpected = "Identifier";
            goto cleanupParse;
        }

        char* pIdentifier = noodleParseString(&lexer, &token);

        // Get the equals token
        noodleLexerNextToken(&lexer, &token);

        if (token.kind != NOODLE_TOKEN_KIND_EQUAL)
        {
            pErrorExpected = "Equals Symbol";
            goto cleanupParse;
        }

        // This next token will determine the type of noodle to create
        noodleLexerNextToken(&lexer, &token);

        Noodle_t* pNewNoodle = NULL;

        switch (token.kind)
        {
            case NOODLE_TOKEN_KIND_LEFTCURLY:
                pNewNoodle = (Noodle_t*)noodleGroup(pIdentifier, pCurrent);
                break;
            case NOODLE_TOKEN_KIND_INTEGER:
                pNewNoodle = (Noodle_t*)noodleInt(pIdentifier, noodleParseInt(&lexer, &token), pCurrent);
                break;
            case NOODLE_TOKEN_KIND_FLOAT:
                pNewNoodle = (Noodle_t*)noodleFloat(pIdentifier, noodleParseFloat(&lexer, &token), pCurrent);
                break;
            case NOODLE_TOKEN_KIND_BOOLEAN:
                pNewNoodle = (Noodle_t*)noodleBool(pIdentifier, noodleParseBool(&lexer, &token), pCurrent);
                break;
            case NOODLE_TOKEN_KIND_STRING:
            {
                char* pString = noodleParseString(&lexer, &token);
                if (!pString) goto cleanupMemory;

                pNewNoodle = (Noodle_t*)noodleString(pIdentifier, pString, pCurrent);
                break;
            }
            case NOODLE_TOKEN_KIND_LEFTBRACKET:
            {
                int arrayStart = token.start;

                // Get the next token to get it's type and ensure it can be in an array 
                noodleLexerNextToken(&lexer, &token);

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
                
                NoodleArray_t* pArray = noodleArray(pIdentifier, NOODLE_TYPE_ARRAY, pCurrent);
                if (!pArray) goto cleanupMemory;

                // Loop through all the tokens of the array to get the count and verification of type
                while (token.kind != NOODLE_TOKEN_KIND_RIGHTBRACKET)
                {
                    // Make sure that the token is the one we expect in the array
                    if (token.kind != expected)
                    {
                        pErrorExpected = noodleStringFromTokenKind(expected);
                        goto cleanupParse;
                    }

                    pArray->count++;

                    // Expect a ',' or a ']'
                    noodleLexerNextToken(&lexer, &token);

                    if (token.kind == NOODLE_TOKEN_KIND_COMMA)
                    {
                        noodleLexerNextToken(&lexer, &token);
                        continue;
                    }
                }

                // Kinda hacky but reset the lexer back to the array start position
                lexer.current = arrayStart;

                // Allocate the array of values
                switch (expected)
                {
                    case NOODLE_TOKEN_KIND_INTEGER:
                        pArray->type = NOODLE_TYPE_INTEGER;
                        pArray->pIntegers = NOODLE_MALLOC(sizeof(int) * pArray->count);
                        break;

                    case NOODLE_TOKEN_KIND_FLOAT:
                        pArray->type = NOODLE_TYPE_FLOAT;
                        pArray->pFloats = NOODLE_MALLOC(sizeof(float) * pArray->count);
                        break;

                    case NOODLE_TOKEN_KIND_BOOLEAN:
                        pArray->type = NOODLE_TYPE_BOOLEAN;
                        pArray->pBooleans = NOODLE_MALLOC(sizeof(NOODLE_BOOL) * pArray->count);
                        break;

                    case NOODLE_TOKEN_KIND_STRING:
                        pArray->type = NOODLE_TYPE_STRING;
                        pArray->ppStrings = NOODLE_MALLOC(sizeof(char*) * pArray->count);
                        break;
                }

                // Check if the array was allocated correctly using ints value of the union 
                if (!pArray->pIntegers) goto cleanupMemory;

                // Iterate through the values once again, to set the array
                noodleLexerNextToken(&lexer, &token);

                int i = 0;
                while (token.kind != NOODLE_TOKEN_KIND_RIGHTBRACKET)
                {
                    switch (expected)
                    {
                        case NOODLE_TOKEN_KIND_INTEGER:
                            pArray->pIntegers[i] = noodleParseInt(&lexer, &token);
                            break;
                        case NOODLE_TOKEN_KIND_FLOAT:
                            pArray->pFloats[i] = noodleParseFloat(&lexer, &token);
                            break;
                        case NOODLE_TOKEN_KIND_BOOLEAN:
                            pArray->pBooleans[i] = noodleParseBool(&lexer, &token); 
                            break;
                        case NOODLE_TOKEN_KIND_STRING:
                        {
                            char* pString = noodleParseString(&lexer, &token);
                            if (!pString) goto cleanupMemory;

                            pArray->ppStrings[i] = pString;
                            break;
                        }
                    }

                    i++;

                    noodleLexerNextToken(&lexer, &token);
                    
                    if (token.kind == NOODLE_TOKEN_KIND_COMMA)
                    {
                        noodleLexerNextToken(&lexer, &token);
                        continue;
                    }
                }

                pNewNoodle = (Noodle_t*)pArray;
                break;
            }
        }

        if (!pNewNoodle || 
            !noodleGroupInsert(pCurrent, pIdentifier, pNewNoodle)) 
            goto cleanupMemory;

        if (pNewNoodle->type == NOODLE_TYPE_GROUP)
        {
            pCurrent = (NoodleGroup_t*)pNewNoodle;
        }

        noodleLexerNextToken(&lexer, &token);
        
        if (token.kind == NOODLE_TOKEN_KIND_COMMA)
        {
            noodleLexerNextToken(&lexer, &token);

            // Spare commas are not recommended, but are allowed after a value 
            // even if it's the last one
            if (token.kind == NOODLE_TOKEN_KIND_RIGHTCURLY)
                goto parseEndGroups;

            continue;
        }

        
    parseEndGroups:

        while (token.kind == NOODLE_TOKEN_KIND_RIGHTCURLY)
        {
            Noodle_t* pTemp = (Noodle_t*)pCurrent;

            if (!pTemp->pParent)
            {
                pErrorExpected = "Identifier";
                goto cleanupParse;
            }

            pCurrent = pTemp->pParent;

            noodleLexerNextToken(&lexer, &token);
        }

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
    snprintf(pErrorBuffer, bufferSize, "(Ln %i, Col %i) Unexpected token found, \"%.*s\", expected token, \"%s\"!", lexer.line, lexer.character, token.end - token.start, pContent, pErrorExpected);
    noodleFree((Noodle_t*)pRoot);
    return NULL;

}

NoodleGroup_t* noodleParseFromFile();

Noodle_t* noodleFrom(const NoodleGroup_t* pGroup, const char* pName)
{
    assert(pGroup);
    assert(pName);

    size_t index = noodleGroupHashFunction(pName) % NOODLE_GROUP_BUCKETS_COUNT;

    NoodleNode_t* pCurrent = pGroup->ppBuckets[index];
    
    while (pCurrent)
    {
        if (strcmp(pCurrent->pNoodle->pName, pName) == 0) break;

        pCurrent = pCurrent->pNext;
    }

    if (!pCurrent) return NULL;

    return pCurrent->pNoodle;
}

NoodleGroup_t* noodleGroupFrom(const NoodleGroup_t* pGroup, const char* pName)
{
    assert(pGroup);
    assert(pName);

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return NULL;

    assert(pNoodle->type == NOODLE_TYPE_GROUP && "Requested noodle was not a group!");
    if (pNoodle->type != NOODLE_TYPE_GROUP) return NULL;

    return (NoodleGroup_t*)pNoodle;
}

int noodleIntFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* pSucceeded)
{
    assert(pGroup);
    assert(pName);

    if (pSucceeded) *pSucceeded = NOODLE_FALSE;

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return 0;

    assert(pNoodle->type == NOODLE_TYPE_INTEGER && "Requested noodle was not am integer!");
    if (pNoodle->type != NOODLE_TYPE_INTEGER) return 0;

    if (pSucceeded) *pSucceeded = true;
    return ((NoodleValue_t*)pNoodle)->i;
}

float noodleFloatFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* pSucceeded)
{
    assert(pGroup);
    assert(pName);

    if (pSucceeded) *pSucceeded = NOODLE_FALSE;

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return 0.0f;

    assert(pNoodle->type == NOODLE_TYPE_FLOAT && "Requested noodle was not a float!");
    if (pNoodle->type != NOODLE_TYPE_FLOAT) return 0.0f;

    if (pSucceeded) *pSucceeded = NOODLE_TRUE;
    return ((NoodleValue_t*)pNoodle)->f;
}

NOODLE_BOOL noodleBoolFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* pSucceeded)
{
    assert(pGroup);
    assert(pName);

    if (pSucceeded) *pSucceeded = NOODLE_FALSE;

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return NOODLE_FALSE;

    assert(pNoodle->type == NOODLE_TYPE_BOOLEAN && "Requested noodle was not a boolean!");
    if (pNoodle->type != NOODLE_TYPE_BOOLEAN) return NOODLE_FALSE;

    if (pSucceeded) *pSucceeded = NOODLE_TRUE;
    return ((NoodleValue_t*)pNoodle)->b;
}

const char* noodleStringFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* pSucceeded)
{
    assert(pGroup);
    assert(pName);

    if (pSucceeded) *pSucceeded = NOODLE_FALSE;

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return NOODLE_FALSE;

    assert(pNoodle->type == NOODLE_TYPE_STRING && "Requested noodle was not a string!");
    if (pNoodle->type != NOODLE_TYPE_STRING) return NOODLE_FALSE;

    if (pSucceeded) *pSucceeded = NOODLE_TRUE;
    return ((NoodleValue_t*)pNoodle)->s; 
}

const NoodleArray_t* noodleArrayFrom(const NoodleGroup_t* pGroup, const char* pName)
{
    assert(pGroup);
    assert(pName);

    Noodle_t* pNoodle = noodleFrom(pGroup, pName);
    if (!pNoodle) return NULL;

    assert(pNoodle->type == NOODLE_TYPE_ARRAY && "Requested noodle was not an array!");
    if (pNoodle->type != NOODLE_TYPE_ARRAY) return NULL;

    return (NoodleArray_t*)pNoodle; 
}

size_t noodleCount(const Noodle_t* pNoodle)
{
    switch (pNoodle->type)
    {
    case NOODLE_TYPE_GROUP: return ((NoodleGroup_t*)pNoodle)->count;
    case NOODLE_TYPE_ARRAY: return ((NoodleArray_t*)pNoodle)->count;
    default:
        assert(false && "Type not a valid array or group!");
        return 0;
    }
}

int noodleIntAt(const NoodleArray_t* pArray, size_t index)
{
    assert(pArray);
    assert(pArray->type == NOODLE_TYPE_INTEGER);
    assert(pArray->count > index);

    return pArray->pIntegers[index];
}

float noodleFloatAt(const NoodleArray_t* pArray, size_t index)
{
    assert(pArray);
    assert(pArray->type == NOODLE_TYPE_FLOAT);
    assert(pArray->count > index);

    return pArray->pFloats[index];
}

NOODLE_BOOL noodleBoolAt(const NoodleArray_t* pArray, size_t index)
{
    assert(pArray);
    assert(pArray->type == NOODLE_TYPE_BOOLEAN);
    assert(pArray->count > index);

    return pArray->pBooleans[index];
}

const char* noodleStringAt(const NoodleArray_t* pArray, size_t index)
{
    assert(pArray);
    assert(pArray->type == NOODLE_TYPE_STRING);
    assert(pArray->count > index);

    return pArray->ppStrings[index];
}

void noodleFree(Noodle_t* pNoodle)
{
    switch (pNoodle->type)
    {
        case NOODLE_TYPE_GROUP:
            NoodleGroup_t* pGroup = (NoodleGroup_t*)pNoodle;
            NoodleNode_t* pNode = NULL;
            NoodleNode_t* pToFree = NULL;

            for (int i = 0; i < pGroup->bucketCount; i++)
            {
                pNode = pGroup->ppBuckets[i];    
                
                while (pNode != NULL)
                {
                    noodleFree(pNode->pNoodle);

                    pToFree = pNode;
                    pNode = pNode->pNext;
                    
                    NOODLE_FREE(pToFree);
                    pToFree = NULL;
                }
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTION DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

char* noodleStringDuplicate(const char* pStr)
{
    size_t length = strlen(pStr) + 1;
    char* pNewStr = NOODLE_MALLOC(length);

    if (pNewStr == NULL)
        return NULL;

    return memcpy(pNewStr, pStr, length);
}

const char* noodleStringFromTokenKind(NoodleTokenKind_t kind)
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

NoodleToken_t noodleToken(NoodleTokenKind_t kind, int start, int end)
{
    return (NoodleToken_t){kind, start, end};
}

int noodleParseInt(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken)
{
    assert(pLexer);
    assert(pToken);
    assert(pToken->kind == NOODLE_TOKEN_KIND_INTEGER);

    return (int)strtol(pLexer->pContent + pToken->start, NULL, 10);
}

float noodleParseFloat(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken)
{
    assert(pLexer);
    assert(pToken);
    assert(pToken->kind == NOODLE_TOKEN_KIND_FLOAT);

    return strtof(pLexer->pContent + pToken->start, NULL);
}

NOODLE_BOOL noodleParseBool(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken)
{
    assert(pLexer);
    assert(pToken);
    assert(pToken->kind == NOODLE_TOKEN_KIND_BOOLEAN);
    
    return (*(pLexer->pContent + pToken->start) == 't') ? NOODLE_TRUE : NOODLE_FALSE;   
}

char* noodleParseString(const NoodleLexer_t* pLexer, const NoodleToken_t* pToken)
{
    // Need to allocate a new string
    int stringLength = pToken->end - pToken->start; // Convert indexes into counts
    char* pString = NOODLE_MALLOC(stringLength + 1);
    if (!pString) return NULL;

    // Set the identifier string's contents
    memset(pString, 0, stringLength + 1);
    memcpy(pString, pLexer->pContent + pToken->start, stringLength);
    
    return pString;
}

NoodleGroup_t* noodleGroup(char* pName, NoodleGroup_t* pParent)
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

NoodleArray_t* noodleArray(char* pName, NoodleType_t type, NoodleGroup_t* pParent)
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
    pArray->count = 0;
    pArray->pIntegers = NULL;

    return pArray;
}

NoodleValue_t* noodleValue(char* pName, NoodleType_t type, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    if (!pValue) return NULL;

    Noodle_t* pNoodle = (Noodle_t*)pValue;
    pNoodle->pName = pName;
    pNoodle->type = type;
    pNoodle->pParent = pParent;

    return pValue;
}

NoodleValue_t* noodleInt(char* pName, int value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = noodleValue(pName, NOODLE_TYPE_INTEGER, pParent);
    if (!pValue) return NULL;

    pValue->i = value;

    return pValue;
}

NoodleValue_t* noodleFloat(char* pName, float value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = noodleValue(pName, NOODLE_TYPE_FLOAT, pParent);
    if (!pValue) return NULL;

    pValue->f = value;

    return pValue;
}

NoodleValue_t* noodleBool(char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = noodleValue(pName, NOODLE_TYPE_BOOLEAN, pParent);
    if (!pValue) return NULL;

    pValue->b = value;

    return pValue;
}

NoodleValue_t* noodleString(char* pName, char* pStrValue, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = noodleValue(pName, NOODLE_TYPE_STRING, pParent);
    if (!pValue) return NULL;

    pValue->s = pStrValue;

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

    pGroup->count++;
    pCurrent->pNext = pNode;
    return true;
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

NOODLE_BOOL noodleLexerNextToken(NoodleLexer_t* pLexer, NoodleToken_t* pTokenOut)
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
            noodleLexerIdentifierOrBool(pLexer, pTokenOut);
            break;
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
            noodleLexerNumber(pLexer, pTokenOut);
            break;
        case '=':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_EQUAL, pTokenOut);
            break;
        case '{':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_LEFTCURLY, pTokenOut);
            break;
        case '}':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTCURLY, pTokenOut);
            break;
        case '[':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_LEFTBRACKET, pTokenOut);
            break;
        case ']':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTBRACKET, pTokenOut);
            break;
        case '\"':
            noodleLexerString(pLexer, pTokenOut);
            break;
        case ',':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_COMMA, pTokenOut);
            break;
        case '#':
            noodleLexerSkipComment(pLexer);
            return noodleLexerNextToken(pLexer, pTokenOut);
        case ' ':
        case '\n':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
            noodleLexerSkipSpaces(pLexer);
            return noodleLexerNextToken(pLexer, pTokenOut);
        case '\0':
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_END, pTokenOut);
            break;
        default:
            noodleLexerAtom(pLexer, NOODLE_TOKEN_KIND_UNEXPECTED, pTokenOut);
            return NOODLE_FALSE;
    }

    return true;
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

char noodleLexerPeek(const NoodleLexer_t* pLexer)
{
    return pLexer->pContent[pLexer->current];
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
    char c = noodleLexerGet(pLexer);
    while ((c = noodleLexerPeek(pLexer)) && noodleLexerIsSpace(c))
    {
        c = noodleLexerGet(pLexer);
    }
}

void noodleLexerAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind, NoodleToken_t* pTokenOut)
{
    assert(pLexer);
    assert(pTokenOut);

    noodleLexerGet(pLexer);

    *pTokenOut = noodleToken(kind, pLexer->current, pLexer->current + 1);
}

void noodleLexerIdentifierOrBool(NoodleLexer_t* pLexer, NoodleToken_t* pTokenOut)
{
    assert(pLexer);
    assert(pTokenOut);

    int start = pLexer->current;

    while(noodleLexerIsIdentifier(noodleLexerPeek(pLexer))) 
    {
        noodleLexerGet(pLexer);
    }

    // It's easier to catch booleans here so we do
    if (((sizeof("true") - 1 == pLexer->current - start) || (sizeof("false") - 1 == pLexer->current - start)) 
        && ((strncmp(pLexer->pContent + start, "true", pLexer->current - start) == 0) || (strncmp(pLexer->pContent + start, "false", pLexer->current - start) == 0)))
    {
        *pTokenOut = noodleToken(NOODLE_TOKEN_KIND_BOOLEAN, start, pLexer->current);
        return;
    }

    *pTokenOut = noodleToken(NOODLE_TOKEN_KIND_IDENTIFIER, start, pLexer->current);
}

void noodleLexerNumber(NoodleLexer_t* pLexer, NoodleToken_t* pOutToken)
{
    assert(pLexer);
    assert(pOutToken);

    int start = pLexer->current;

    while (noodleLexerIsNumber(noodleLexerPeek(pLexer)))
    {
        noodleLexerGet(pLexer);
    }

    // Determine if this string is a long or a float, or a failed case
    char* pFloatEnd = NULL;
    char* pLongEnd = NULL;

    strtof(pLexer->pContent + start, &pFloatEnd);
    strtol(pLexer->pContent + start, &pLongEnd, 10);

    NoodleTokenKind_t kind = NOODLE_TOKEN_KIND_UNEXPECTED;

    if (pLongEnd == pLexer->pContent + pLexer->current)
        kind = NOODLE_TOKEN_KIND_INTEGER;
    else if (pFloatEnd == pLexer->pContent + pLexer->current)
        kind = NOODLE_TOKEN_KIND_FLOAT;

    *pOutToken = noodleToken(kind, start, pLexer->current);
}

void noodleLexerString(NoodleLexer_t* pLexer, NoodleToken_t* pTokenOut)
{
    assert(pLexer);
    assert(pTokenOut);

    noodleLexerGet(pLexer); // Get the starting quote

    int start = pLexer->current; 

    // Iterate until another end quote is found
    char c = '\0';
    while((c = noodleLexerPeek(pLexer)) && c != '\"')
    {
        noodleLexerGet(pLexer);
    }

    noodleLexerGet(pLexer); // Push past the second quote 

    *pTokenOut = noodleToken(NOODLE_TOKEN_KIND_STRING, start, pLexer->current - 1);
}
