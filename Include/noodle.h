#ifndef NOODLE_PARSER_H
#define NOODLE_PARSER_H

#include <stdint.h>
#include <stdbool.h>


#ifndef NOODLE_BOOL
#define NOODLE_BOOL bool
#endif

#ifndef NOODLE_TRUE
#define NOODLE_TRUE true
#endif

#ifndef NOODLE_FALSE
#define NOODLE_FALSE false
#endif

// Any value prefixed with this can be set to NULL or 0
#ifndef NOODLE_NULLABLE
#define NOODLE_NULLABLE 
#endif

#ifndef NOODLE_MALLOC
#define NOODLE_MALLOC(size) malloc(size)
#endif

#ifndef NOODLE_FREE
#define NOODLE_FREE(ptr) free(ptr)
#endif

typedef enum NoodleType_t
{
    NOODLE_TYPE_GROUP, 
    NOODLE_TYPE_ARRAY,
    NOODLE_TYPE_INTEGER, 
    NOODLE_TYPE_FLOAT, 
    NOODLE_TYPE_BOOLEAN, 
    NOODLE_TYPE_STRING,
} NoodleType_t;


// Every noodle type can be cast to Noodle_t to get it's type and name
typedef struct Noodle_t Noodle_t;
typedef struct NoodleGroup_t NoodleGroup_t;
typedef struct NoodleArray_t NoodleArray_t;
typedef struct NoodleValue_t NoodleValue_t;

typedef NOODLE_BOOL (* NoodleForeachGroupCallback_t)(Noodle_t* pNoodle); // Return false to break

typedef struct Noodle_t
{
    NoodleType_t        type;
    NoodleGroup_t*      pParent;
    char*               pName;
} Noodle_t;


NoodleGroup_t*          noodleParse(const char* pContent, char* NOODLE_NULLABLE pErrorBuffer, size_t NOODLE_NULLABLE bufferSize);
NoodleGroup_t*          noodleParseFromFile(const char* pPath, char* NOODLE_NULLABLE pErrorBuffer, size_t NOODLE_NULLABLE bufferSize);
Noodle_t*               noodleFrom(const NoodleGroup_t* pGroup, const char* pName);
NoodleGroup_t*          noodleGroupFrom(const NoodleGroup_t* pGroup, const char* pName);
int                     noodleIntFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* NOODLE_NULLABLE pSucceeded);
float                   noodleFloatFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* NOODLE_NULLABLE pSucceeded);
NOODLE_BOOL             noodleBoolFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* NOODLE_NULLABLE pSucceeded);
const char*             noodleStringFrom(const NoodleGroup_t* pGroup, const char* pName, NOODLE_BOOL* NOODLE_NULLABLE pSucceeded);
const NoodleArray_t*    noodleArrayFrom(const NoodleGroup_t* pGroup, const char* pName);
size_t                  noodleCount(const Noodle_t* noodle);
int                     noodleIntAt(const NoodleArray_t* pArray, size_t index);
float                   noodleFloatAt(const NoodleArray_t* pArray, size_t index);
NOODLE_BOOL             noodleBoolAt(const NoodleArray_t* pArray, size_t index);
const char*             noodleStringAt(const NoodleArray_t* pArray, size_t index);
void                    noodleCleanup(NoodleGroup_t* pGroup);

NOODLE_BOOL             noodleHas(const NoodleGroup_t* pGroup, const char* pName);
void                    noodleGroupForeach(NoodleGroup_t* pGroup, NoodleForeachGroupCallback_t callback);

#endif // NOODLE_PARSER_H