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
    char*               pName;
} Noodle_t;

typedef struct NoodleGroup_t
{
    Noodle_t    base;
    size_t      count;
    size_t      bucketCount;
    Noodle_t*   ppBuckets[16];
};

typedef struct NoodleValue_t
{
    Noodle_t    base;
    union       value
    {
        int     i;
        float   f;
        bool    b;
        char*   s;
    };
} NoodleValue_t;

typedef struct NoodleArray_t
{
    Noodle_t base;
    NoodleType_t type;
    size_t count;
    void* pValues;
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
    NoodleTokenKind_t current;
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

////////////////////////////////////////////////////////////////////////////////
// HELPER DECLARATIONS
////////////////////////////////////////////////////////////////////////////////

char*           noodleStrdup(const char* str);

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
    char* pContent = strdup(pUserContent);
    if (pContent == NULL)
    {
        strncpy(pErrorBuffer, e_memFailure, bufferSize);
        return NULL;
    }

    noodleRemoveCommentsAndSpaces(pContent);

    // Create the root group to contain the other noodles
    NoodleGroup_t* pRoot = NOODLE_CALLOC(1, NoodleGroup_t);
    if (pRoot == NULL)
    {
        strncpy(pErrorBuffer, e_memFailure, bufferSize);
        return NULL;
    }

    NoodleGroup_t* pCurrent = pRoot;

    char c = pContent[0];
    
    
    while (true)
    {

        // Remove any comments from the code


        while (noodleLexerIsIdentifier(c))

    }
    
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
void                    noodleFree(Noodle_t* noodle);

////////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTION DEFINITIONS
////////////////////////////////////////////////////////////////////////////////

char* noodleStrdup(const char* pStr)
{
    char* pUserStr = NOODLE_CALLOC(strlen(str), char);
    
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

static NOODLE_BOOL noodleLexerIsNumber(char c)
{
    switch(c)
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

static NoodleToken_t noodleLexerNext(NoodleLexer_t* pLexer)
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
    char* pStart = pLexer->current;
    char* pEnd = pLexer->current;

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



