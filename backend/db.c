#include "db.h"

#include <mongoc/mongoc.h>

#define MONGO_CONNECTION_STRING_VAR "MONGO_CONNECTION_STRING"
#define MONGO_DATABASE "databaz"

#define MONGO_PROJECTS_COLLECTION "projects"
#define MONGO_USERS_COLLECTION "users"
#define MONGO_FEATURES_COLLECTION "features"

static mongoc_client_t *MongoClient;
static mongoc_database_t *MongoDatabase;

static mongoc_collection_t *MongoProjectsCollection;
static mongoc_collection_t *MongoUsersCollection;
static mongoc_collection_t *MongoFeaturesCollection;

void DbInit(void) {
    mongoc_init();

    const char *MongoConnectionString = getenv(MONGO_CONNECTION_STRING_VAR);
    if (MongoConnectionString == NULL) {
        PANIC_FMT("Expected the MongoDB connection string (var '%s') to be set in the environment", MONGO_CONNECTION_STRING_VAR);
    }

    MongoClient = mongoc_client_new(MongoConnectionString);

    bson_t *PingCommand = BCON_NEW("ping", BCON_INT32(1));
    bson_t PingReply = BSON_INITIALIZER;
    bson_error_t PingError;

    b32 PingOk = mongoc_client_command_simple(MongoClient, "admin", PingCommand, NULL, &PingReply, &PingError);
    if (!PingOk) {
        PANIC_FMT("Could not send a ping command to the database: %s", PingError.message);
    }

    bson_destroy(&PingReply);
    bson_destroy(PingCommand);

    MongoDatabase = mongoc_client_get_database(MongoClient, MONGO_DATABASE);

    MongoProjectsCollection = mongoc_database_get_collection(MongoDatabase, MONGO_PROJECTS_COLLECTION);
    MongoUsersCollection = mongoc_database_get_collection(MongoDatabase, MONGO_USERS_COLLECTION);
    MongoFeaturesCollection = mongoc_database_get_collection(MongoDatabase, MONGO_FEATURES_COLLECTION);
}

static const char *BsonEncode_string_view(arena *Arena, string_view Sv) {
    const char *CStr = StringViewCloneCStr(Arena, Sv);
    return (BCON_UTF8(CStr));
}

b32 DbInsertProject(const project_entity *ProjectEntity) {
    arena *TempArena = GetTempArena();

    // Id, Name, Description
#define X(Type, Name) #Name, BsonEncode_##Type(TempArena, ProjectEntity->Name),
bson_t *Document = bcon_new(NULL,
                            DECLARE_PROJECT_ENTITY
                            NULL);
#undef X

    b32 Result = mongoc_collection_insert_one(MongoProjectsCollection, Document, NULL, NULL, NULL);
    bson_destroy(Document);
    return Result;
}

static inline b32 BsonIterGet_string_view(bson_iter_t *Iterator, arena *Arena, string_view *Out) {
    u32 Length = 0;
    const char *CStr = bson_iter_utf8(Iterator, &Length);
    if (CStr == NULL) return 0;

    Out->Items = ArenaPush(Arena, Length);
    Out->Count = Length;
    memcpy(Out->Items, CStr, Length);
    return 1;
}

b32 DbGetProjectById(arena *Arena, string_view Id, project_entity *ProjectEntity) {
    arena *TempArena = GetTempArena();

    b32 Result;

    const char *IdBson = BsonEncode_string_view(TempArena, Id);
    bson_t *Query = BCON_NEW("Id", IdBson);
    bson_t *QueryOptions = BCON_NEW("limit", BCON_INT32(1));

    mongoc_cursor_t *ResultsCursor = mongoc_collection_find_with_opts(MongoProjectsCollection, Query, QueryOptions, NULL);

    const bson_t *ProjectDoc;
    if (!mongoc_cursor_next(ResultsCursor, &ProjectDoc)) {
        Result = 0;
        goto Cleanup;
    }

    bson_iter_t DocIterator;
    if (!bson_iter_init(&DocIterator, ProjectDoc)) {
        Result = 0;
        goto Cleanup;
    }

#define X(Type, Field)                                                  \
    if (!bson_iter_find(&DocIterator, #Field)) {                        \
        Result = 0;                                                     \
        goto Cleanup;                                                   \
    }                                                                   \
    if (!BsonIterGet_##Type(&DocIterator, Arena, &ProjectEntity->Field)) { \
        Result = 0;                                                     \
        goto Cleanup;                                                   \
    }

    DECLARE_PROJECT_ENTITY
#undef X

    Result = 1;

Cleanup:
    bson_destroy(Query);
    bson_destroy(QueryOptions);
    mongoc_cursor_destroy(ResultsCursor);

    return Result;
}

b32 DbUpdateProject(const project_update_entity *ProjectUpdate) {
    ASSERT(ProjectUpdate->Name.HasValue || ProjectUpdate->Description.HasValue);

    b32 Result;

    arena *TempArena = GetTempArena();

    const char *UpdateId = BsonEncode_string_view(TempArena, ProjectUpdate->Id);
    bson_t *Query = BCON_NEW("Id", UpdateId);
    bson_t *Update;

    // FIXME(oleh): This is so ugly because for whatever reason i get an assertion
    // error inside the Mongo driver when calling `bcon_append` :/.
    if (ProjectUpdate->Name.HasValue) {
        const char *UpdateName = BsonEncode_string_view(TempArena, ProjectUpdate->Name.Value);

        if (ProjectUpdate->Description.HasValue) {
            const char *UpdateDescription = BsonEncode_string_view(TempArena, ProjectUpdate->Description.Value);
            Update = BCON_NEW("$set", "{", "Name", UpdateName, "Description", UpdateDescription, "}");
        } else {
            Update = BCON_NEW("$set", "{", "Name", UpdateName, "}");
        }
    } else {
        const char *UpdateDescription = BsonEncode_string_view(TempArena, ProjectUpdate->Description.Value);
        Update = BCON_NEW("$set", "{", "Description", UpdateDescription, "}");
    }

    if (!mongoc_collection_update_one(MongoProjectsCollection, Query, Update, NULL, NULL, NULL)) {
        Result = 0;
    } else {
        Result = 1;
    }

    bson_destroy(Query);
    bson_destroy(Update);

    return Result;
}

b32 DbDeleteProjectById(string_view ProjectId) {
    b32 Result;

    arena *TempArena = GetTempArena();

    const char *DeleteId = BsonEncode_string_view(TempArena, ProjectId);
    bson_t *Query = BCON_NEW("Id", DeleteId);

    if (!mongoc_collection_delete_one(MongoProjectsCollection, Query, NULL, NULL, NULL)) {
        Result = 0;
    } else {
        Result = 1;
    }

    bson_destroy(Query);

    return Result;
}

b32 DbGetAllProjects(arena *Arena, project_entity **Projects, uz *ProjectsCount) {
    uz StartOffset = Arena->Offset;

    b32 Result = 1;

    bson_t Query;
    bson_init(&Query);

    mongoc_cursor_t *ResultsCursor = mongoc_collection_find_with_opts(MongoProjectsCollection, &Query, NULL, NULL);

    struct {
        project_entity *Items;
        uz Count;
        uz Capacity;
    } ProjectsArray;
    ARRAY_INIT(Arena, &ProjectsArray);

    const bson_t *ProjectDoc;
    while (mongoc_cursor_next(ResultsCursor, &ProjectDoc)) {
        bson_iter_t DocIterator;
        if (!bson_iter_init(&DocIterator, ProjectDoc)) {
            Result = 0;
            goto Cleanup;
        }

        project_entity ProjectEntity;

#define X(Type, Field)                                                  \
        if (!bson_iter_find(&DocIterator, #Field)) {                    \
            Result = 0;                                                 \
            goto Cleanup;                                               \
        }                                                               \
        if (!BsonIterGet_##Type(&DocIterator, Arena, &ProjectEntity.Field)) { \
            Result = 0;                                                 \
            goto Cleanup;                                               \
        }

        DECLARE_PROJECT_ENTITY
#undef X

       ARRAY_PUSH(Arena, &ProjectsArray, ProjectEntity);
    }

Cleanup:
    bson_destroy(&Query);
    mongoc_cursor_destroy(ResultsCursor);

    if (Result == 0) {
        Arena->Offset = StartOffset;
    } else {
        *ProjectsCount = ProjectsArray.Count;
        *Projects = ProjectsArray.Items;
    }

    return Result;
}
