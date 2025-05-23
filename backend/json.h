#ifndef JSON_H_
#define JSON_H_

#include "common.h"

typedef enum {
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
} json_value_type;

struct json_value;
typedef struct json_value json_value;

typedef struct {
    json_value *Items;
    uz Count;
    uz Capacity;
} json_array;

typedef struct {
    string_view *Keys;
    json_value *Values;
    uz Count;
    uz Capacity;
} json_object;

struct json_value {
    json_value_type Type;
    union {
        double Number;
        string_view String;
        json_array Array;
        json_object Object;
    };
};

b32 JsonParse(arena *Arena, string_view Input, json_value *OutValue);

b32 JsonObjectGet(const json_object *Object, string_view Key, json_value *OutValue);

#define ENUM_JSON_GETTERS \
    X(string_view) \
        X(f64) \
        X(u64) \
        X(u32)

#define X(Type) b32 JsonObjectGet_##Type(const json_object *Object, string_view Key, Type *OutValue);
ENUM_JSON_GETTERS
#undef X

#define OPTIONAL_GETTER(Type) \
    static inline b32 JsonObjectGet_optional_##Type(const json_object *Object, string_view Key, optional_##Type *OutValue) { \
        OutValue->HasValue = JsonObjectGet_##Type(Object, Key, &OutValue->Value); \
        return 1;                                                       \
    }

#define X OPTIONAL_GETTER
ENUM_JSON_GETTERS
#undef X

void JsonBegin(arena *);

void JsonBeginObject(void);
void JsonEndObject(void);

void JsonBeginArray(void);
void JsonEndArray(void);

void JsonPutNumber(f64);
void JsonPutString(string_view);

void JsonPutTrue(void);
void JsonPutFalse(void);
void JsonPutNull(void);

void JsonPutKey(string_view);

string_view JsonEnd(void);

// Aliases for entity-type serializers.

#define JsonPut_string_view JsonPutString

#endif // JSON_H_
