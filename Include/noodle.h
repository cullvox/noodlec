#ifndef NOODLE_PARSER_H
#define NOODLE_PARSER_H

#include <stdint.h>

#ifndef NOODLE_NO_STDBOOL_H
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
#endif

#ifndef NOODLE_NO_STDLIB_H
    #include <stdlib.h>

    #ifndef NOODLE_MALLOC
        #define NOODLE_MALLOC(size) malloc(size)
    #endif
    #ifndef NOODLE_FREE
        #define NOODLE_FREE(ptr) free(ptr)
    #endif

    #ifndef NOODLE_MEMCPY 
        #define NOODLE_MEMCPY(dest, src, size) memcpy(dest, src, size)
    #endif
#endif

#ifndef NOODLE_NO_STRING_H
    #include <string.h>
#endif


typedef enum NoodleResult_t
{
    NOODLE_SUCCESS,
    NOODLE_MEMORY_ALLOC_ERROR,
    NOODLE_UNEXPECTED_TOKEN_ERROR,
} NoodleResult_t;

typedef enum NoodleType_t
{
    NOODLE_TYPE_GROUP, 
    NOODLE_TYPE_ARRAY,
    NOODLE_TYPE_INTEGER, 
    NOODLE_TYPE_FLOAT, 
    NOODLE_TYPE_BOOLEAN, 
    NOODLE_TYPE_STRING,
} NoodleType_t;

typedef struct Noodle_t Noodle_t;
typedef struct NoodleGroup_t NoodleGroup_t;
typedef struct NoodleArray_t NoodleArray_t;
typedef struct NoodleValue_t NoodleValue_t;

NoodleGroup_t*          noodleParse(const char* pContent, char* pErrorBuffer, size_t bufferSize);
NoodleGroup_t*          noodleParseFromFile(const char* pPath, char* pErrorBuffer, size_t bufferSize);
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

#endif // NOODLE_PARSER_H