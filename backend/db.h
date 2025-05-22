#ifndef DB_H_
#define DB_H_

#include "common.h"

typedef struct {
    string_view Id;
    string_view Name;
    string_view Description;
} project_entity;

typedef struct {
    string_view Id;
    string_view FirstName;
    string_view LastName;
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

typedef struct {
    string_view Id;
    string_view Name;
    string_view Description;
    feature_priority Priority;
    string_view ProjectId;
    string_view CreationDate;
    string_view OwnerId;
    feature_state State;
} feature_entity;

void DbInit(void);

b32 DbInsertProject(const project_entity *);
b32 DbGetProjectById(string_view, project_entity *);
b32 DbUpdateProject(project_entity * /* TODO(oleh): Additional arguments. */);
b32 DbDeleteProject(project_entity *);

b32 DbInsertUser(const user_entity *);
b32 DbGetUserById(string_view, user_entity *);
b32 DbUpdateUser(user_entity * /* TODO(oleh): Additional arguments. */);
b32 DbDeleteUser(user_entity *);

b32 DbInsertFeature(const feature_entity *);
b32 DbGetFeatureById(string_view, feature_entity *);
b32 DbUpdateFeature(feature_entity * /* TODO(oleh): Additional arguments. */);
b32 DbDeleteFeature(feature_entity *);

#endif // DB_H_
