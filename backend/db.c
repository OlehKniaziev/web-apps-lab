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

#define BCON_SV(Arena, Sv) (BCON_UTF8(StringViewCloneCStr((Arena), (Sv))))

b32 DbInsertProject(const project_entity *ProjectEntity) {
    arena *TempArena = GetTempArena();

    // Id, Name, Description
    const char *BsonId = BCON_SV(TempArena, ProjectEntity->Id);
    const char *BsonName = BCON_SV(TempArena, ProjectEntity->Name);
    const char *BsonDescription = BCON_SV(TempArena, ProjectEntity->Description);

    // !!!
    FIXME();
    bson_t *Document = BCON_NEW("id", BsonId,
                                "name", BsonName,
                                "description", BsonDescription);

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

    const char *IdBson = BCON_SV(TempArena, Id);\
    bson_t *Query = BCON_NEW("id", IdBson);

    mongoc_cursor_t *ResultsCursor = mongoc_collection_find_with_opts(MongoProjectsCollection, Query, NULL, NULL);

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
    /* bson_destroy(QueryOptions); */
    mongoc_cursor_destroy(ResultsCursor);

    return Result;
}
