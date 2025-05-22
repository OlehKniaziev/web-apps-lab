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

    bson_t *Document = BCON_NEW("id", BsonId,
                                "name", BsonName,
                                "description", BsonDescription);

    b32 Result = mongoc_collection_insert_one(MongoProjectsCollection, Document, NULL, NULL, NULL);
    bson_destroy(Document);
    return Result;
}

b32 DbGetProjectById(string_view Id, project_entity *ProjectEntity) {
    b32 Result;

    bson_t *Query = bson_new();
    bson_t *QueryOptions = BCON_NEW("limit", BCON_INT32(1));

    mongoc_cursor_t *ResultsCursor = mongoc_collection_find_with_opts(ProjectCollection, Query, QueryOptions, NULL);

    const bson_t *ProjectDoc;
    if (!mongoc_cursor_next(ResultsCursor, &ProjectDoc)) {
        Result = 0;
        goto Cleanup;
    }

    ProjectEntity->

Cleanup:
}
