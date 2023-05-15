#include <stdio.h>
#include <assert.h>

#include "noodle.h"

////////////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////////////

#define NOODLE_GROUP_BUCKETS_COUNT 16
#define NOODLE_ARRAY_DEFAULT_CAPACITY 8

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
    union       value
    {
        int         i;
        float       f;
        NOODLE_BOOL b;
        char*       s; // Must be freed
    };
} NoodleValue_t;

typedef struct NoodleArray_t
{
    Noodle_t base;
    NoodleType_t type;
    size_t typeSize;
    size_t count;
    size_t capacity;
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

NoodleGroup_t*  noodleMakeGroup(char* pName, NoodleGroup_t* pParent);
NoodleArray_t*  noodleMakeArray(char* pName, NoodleType_t type, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeIntValue(char* pName, int value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeFloatValue(char* pName, float value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeBoolValue(char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent);
NoodleValue_t*  noodleMakeStringValue(char* pName, char* value, NoodleGroup_t* pParent);

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
    NoodleGroup_t* pRoot = noodleMakeGroup("", NULL);
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
        int identifierLength = token.end - token.start;
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

        switch (token.kind)
        {
            case NOODLE_TOKEN_KIND_INTEGER:
            {
                int value = (int)strtol(pContent + token.start, NULL, 10);

                Noodle_t* pValue = (Noodle_t*)noodleMakeIntValue(pIdentifier, value, pCurrent);
                if (!pValue) goto cleanupMemory;
                
                if (!noodleGroupInsert(pCurrent, pIdentifier, pValue)) 
                    goto cleanupMemory;
                
                break;
            }

            case NOODLE_TOKEN_KIND_FLOAT:
            {
                float value = strtof(pContent + token.start, NULL);

                Noodle_t* pValue = (Noodle_t*)noodleMakeFloatValue(pIdentifier, value, pCurrent);
                if (!pValue) goto cleanupMemory;

                if (!noodleGroupInsert(pCurrent, pIdentifier, pValue))
                    goto cleanupMemory;
                
                break;
            }

            case NOODLE_TOKEN_KIND_BOOLEAN:
            {
                // Use the first character in the token to determine the bool value
                bool value = (*(pContent + token.start) == 't') ? NOODLE_TRUE : NOODLE_FALSE;
                
                Noodle_t* pValue = (Noodle_t*)noodleMakeBoolValue(pIdentifier, value, pCurrent);
                if (!pValue) goto cleanupMemory;

                if (!noodleGroupInsert(pCurrent, pIdentifier, pValue))
                    goto cleanupMemory;
                
                break;
            }

            case NOODLE_TOKEN_KIND_STRING:
            {
                // Create the noodle's string
                size_t stringLength = token.end - token.start; 
                char* pString = NOODLE_MALLOC(stringLength + 1);

                if (!pString) goto cleanupMemory;

                // Set the string's contents
                memset(pString, 0, stringLength + 1);
                memcpy(pString, pContent + token.start, stringLength);
                
                // Create the new string noodle 
                NoodleValue_t* pValue = noodleMakeStringValue(pIdentifier, pString, pCurrent);
                if (!pValue)
                {
                    NOODLE_FREE(pString);
                    goto cleanupMemory;
                }

                if (!noodleGroupInsert(pCurrent, pIdentifier, (Noodle_t*)pValue))
                    goto cleanupMemory;

                break;
            }

            case NOODLE_TOKEN_KIND_LEFTBRACKET:
            {
                
                token = noodleLexerNext(&lexer);

                if (token.kind != NOODLE_TOKEN_KIND_INTEGER && 
                    token.kind != NOODLE_TOKEN_KIND_FLOAT &&
                    token.kind != NOODLE_TOKEN_KIND_BOOLEAN &&
                    token.kind != NOODLE_TOKEN_KIND_STRING)
                {
                    pErrorExpected = "Integer, Float, Boolean, or String";
                    goto cleanupParse;
                }


                //NoodleArray_t* pArray = noodleMakeArray(pIdentifier,  pCurrent);
                //if (!pArray) goto cleanupMemory;

                // Allocate the array noodle

                // Get the next few tokens until the end of the array
                NoodleTokenKind_t expected = token.kind;

                do
                {

                    // Make sure that the token is the one we expect in the array
                    if (token.kind != expected)
                    {
                        pErrorExpected = noodleTokenKindToString(expected);
                        goto cleanupParse;
                    }

                    switch (expected)
                    {
                        case NOODLE_TOKEN_KIND_INTEGER:
                            int value = (int)strtol(pContent + token.start, NULL, 10);

                            
                            break;
                    }
                    
                    token = noodleLexerNext(&lexer);

                } while (token.kind != NOODLE_TOKEN_KIND_RIGHTBRACKET);
                
                break;
            }
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

NoodleGroup_t* noodleMakeGroup(char* pName, NoodleGroup_t* pParent)
{
    assert(pName);

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

NoodleArray_t* noodleMakeArray(char* pName, NoodleType_t type, NoodleGroup_t* pParent)
{
    assert(pName);

    size_t typeSize;
    switch (type)
    {
        case NOODLE_TYPE_INTEGER:
            typeSize = sizeof(int);
            break;
        case NOODLE_TYPE_FLOAT:
            typeSize = sizeof(float);
            break;
        case NOODLE_TYPE_BOOLEAN:
            typeSize = sizeof(NOODLE_BOOL);
            break;
        case NOODLE_TYPE_STRING:
            typeSize = sizeof(char*);
            break;
        default:
            return NULL;
    }

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
    pArray->capacity = NOODLE_ARRAY_DEFAULT_CAPACITY;
    pArray->pValues = NOODLE_MALLOC(typeSize);

    if (!pArray->pValues) 
    {
        NOODLE_FREE(pArray);
        return NULL;
    }

    return pArray;
}

NoodleValue_t* noodleMakeIntValue(char* pName, int value, NoodleGroup_t* pParent)
{
    assert(pName);

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

NoodleValue_t* noodleMakeFloatValue(char* pName, float value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_FLOAT;
    pValue->f = value;

    return pValue;
}

NoodleValue_t* noodleMakeBoolValue(char* pName, NOODLE_BOOL value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_BOOLEAN;
    pValue->b = value;

    return pValue;
}

NoodleValue_t* noodleMakeStringValue(char* pName, char* value, NoodleGroup_t* pParent)
{
    assert(pName);

    NoodleValue_t* pValue = NOODLE_MALLOC(sizeof(NoodleValue_t));
    Noodle_t* pNoodle = (Noodle_t*)pValue;
    if (!pValue) return NULL;

    pNoodle->pName = pName;
    pNoodle->pParent = pParent;
    pNoodle->type = NOODLE_TYPE_STRING;
    pValue->s = value;

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

NOODLE_BOOL noodleArrayPushBack(NoodleArray_t* pArray, void* pValue)
{
    assert(pArray && pValue);
    
    if (pArray->capacity <= pArray->count)
    {
        
    }

    char* pValues =  pArray->pValues;

    //pValues[pArray->pCount * ];

    return false;
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
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTBRACKET);
        case '}':
            return noodleLexerMakeAtom(pLexer, NOODLE_TOKEN_KIND_RIGHTBRACKET);
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
    char c = 0;
    do
    {
        c = noodleLexerGet(pLexer);
    } while (c!= '\0' && noodleLexerIsSpace(c));
}

NoodleToken_t noodleLexerMakeAtom(NoodleLexer_t* pLexer, NoodleTokenKind_t kind)
{
    return (NoodleToken_t){ kind, pLexer->current, pLexer->current + 1};
}

NoodleToken_t noodleLexerMakeIdentifier(NoodleLexer_t* pLexer)
{
    int start = pLexer->current;

    while(noodleLexerIsIdentifier(noodleLexerGet(pLexer))) {}

    return (NoodleToken_t){ NOODLE_TOKEN_KIND_IDENTIFIER, start, pLexer->current-1 };
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



