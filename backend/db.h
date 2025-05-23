#ifndef DB_H_
#define DB_H_

#include "common.h"

#define DECLARE_PROJECT_ENTITY \
    X(string_view, Id) \
    X(string_view, Name) \
    X(string_view, Description)

typedef struct {
#define X(Type, Name) Type Name;
    DECLARE_PROJECT_ENTITY
#undef X
} project_entity;

#define DECLARE_PROJECT_UPDATE_ENTITY \
    X(string_view, Id) \
    X(optional_string_view, Name) \
    X(optional_string_view, Description)

typedef struct {
#define X(Type, Name) Type Name;
    DECLARE_PROJECT_UPDATE_ENTITY
#undef X
} project_update_entity;

#define DECLARE_USER_ENTITY \
    X(string_view, Id) \
    X(string_view, FirstName) \
    X(string_view, LastName)

typedef struct {
#define X(Type, Name) Type Name;
    DECLARE_USER_ENTITY
#undef X
} user_entity;

typedef enum {
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH,
} feature_priority;

typedef enum {
    STATE_TODO,
    STATE_IN_PROGRESS,
    STATE_DONE,
} feature_state;

#define DECLARE_FEATURE_ENTITY \
    X(string_view, Id) \
    X(string_view, Name) \
    X(string_view, Description) \
    X(feature_priority, Priority) \
    X(string_view, ProjectId) \
    X(string_view, CreationDate) \
    X(string_view, OwnerId) \
    X(feature_state, State)

typedef struct {
#define X(Type, Name) Type Name;
    DECLARE_FEATURE_ENTITY
#undef X
} feature_entity;

void DbInit(void);

b32 DbInsertProject(const project_entity *);
b32 DbGetProjectById(arena *Arena, string_view, project_entity *);
b32 DbUpdateProject(const project_update_entity *);
b32 DbDeleteProjectById(string_view);

b32 DbInsertUser(const user_entity *);
b32 DbGetUserById(string_view, user_entity *);
b32 DbUpdateUser(user_entity * /* TODO(oleh): Additional arguments. */);
b32 DbDeleteUser(user_entity *);

b32 DbInsertFeature(const feature_entity *);
b32 DbGetFeatureById(string_view, feature_entity *);
b32 DbUpdateFeature(feature_entity * /* TODO(oleh): Additional arguments. */);
b32 DbDeleteFeature(feature_entity *);

#endif // DB_H_
