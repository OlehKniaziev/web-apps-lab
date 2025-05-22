#include "db.h"

#include <mongoc/mongoc.h>

#define MONGO_ENV_STRING "MONGO_CONNECTION_STRING"
#define MONGO_DATABASE "databaz"

#define MONGO_PROJECTS_COLLECTION "projects"
#define MONGO_USERS_COLLECTION "users"
#define MONGO_FEATURES_COLLECTION "features"

static mongoc_client_t *MongoClient;
static mongoc_database_t *MongoDatabase;

static mongoc_collection_t *MongoProjectsCollection;
static mongoc_collection_t *MongoUsersCollection;
static mongoc_collection_t *MongoFeaturesCollection;

static arena TempArena;

static inline arena GetTempArena(void) {
    ArenaReset(&TempArena);
    return TempArena;
}

void DbInit(void) {
    mongoc_init();

    const char *MongoConnectionString = getenv(MONGO_ENV_STRING);
    if (MongoConnectionString == NULL) {
        PANIC("Expected the MongoDB connection string to be set in the environment");
    }

    MongoClient = mongoc_client_new(MongoConnectionString);

    MongoDatabase = mongoc_client_get_database(MongoClient, MONGO_DATABASE);

    MongoProjectsCollection = mongoc_database_get_collection(MongoDatabase, MONGO_PROJECTS_COLLECTION);
    MongoUsersCollection = mongoc_database_get_collection(MongoDatabase, MONGO_USERS_COLLECTION);
    MongoFeaturesCollection = mongoc_database_get_collection(MongoDatabase, MONGO_FEATURES_COLLECTION);
}

#define BCON_SV(Arena, Sv) (BCON_UTF8(StringViewCloneCStr((Arena), (Sv))))

b32 DbInsertProject(const project_entity *ProjectEntity) {
    arena TempArena = GetTempArena();

    // Id, Name, Description
    const char *BsonId = BCON_SV(&TempArena, ProjectEntity->Id);
    const char *BsonName = BCON_SV(&TempArena, ProjectEntity->Name);
    const char *BsonDescription = BCON_SV(&TempArena, ProjectEntity->Description);

    bson_t *Document = BCON_NEW("id", BsonId,
                                "name", BsonName,
                                "description", BsonDescription);

    b32 Result = mongoc_collection_insert_one(MongoProjectsCollection, Document, NULL, NULL, NULL);
    bson_destroy(Document);
    return Result;
}
