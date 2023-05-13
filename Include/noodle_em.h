#ifndef NOODLE_SINGLE_
#define NOODLE_SINGLE_H

// This file is completely self contained. It will allocate no memory and only
// uses what you provide. If you are using noodle as a


typedef enum NoodleType_t
{
    NOODLE_TYPE_GROUP,
    NOODLE_TYPE_ARRAY,
    NOODLE_TYPE_INT,
    NOODLE_TYPE_FLOAT,
    NOODLE_TYPE_BOOLEAN,
    NOODLE_TYPE_STRING,
    NOODLE_TYPE_INVALID,
} NoodleType_t;

typedef struct NoodleTokenEm_t
{
    NoodleTypeEm_t type;
    int nameStart;
    int nameEnd;
    int valueStart;
    int valueEnd;
    int start;
    int end;
    int size;
};

#define NOODLE_SINGLE_IMPLEMENTATION
#ifdef NOODLE_SINGLE_IMPLEMENTATION

static inline bool noodleIsIdentifier(char c)
{

}

NoodleTypeEm_t noodleParseNext();

#endif // NOODLE_SINGLE_IMPLEMENTATION

#endif // NOODLE_SINGLE_H