#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "http.h"
#include "db.h"
#include "json.h"

#define HANDLER(Name) static http_response_status Name(http_response_context *Context)

#define DEFAULT_JSON_DESERIALIZER(Json, Object, Type, Field) if (!JsonObjectGet_##Type((Json), SV_LIT(#Field), &(Object)->Field)) return HTTP_STATUS_BAD_REQUEST;

HANDLER(IndexHandler) {
    Context->Content = SV_LIT("HELLO");
    return HTTP_STATUS_OK;
}

HANDLER(InsertProjectHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    json_value JsonPayloadValue;
    if (!JsonParse(Context->Arena, Context->Request.Body, &JsonPayloadValue)) return HTTP_STATUS_BAD_REQUEST;
    if (JsonPayloadValue.Type != JSON_OBJECT) return HTTP_STATUS_BAD_REQUEST;

    json_object JsonPayload = JsonPayloadValue.Object;

    project_entity Project;

#define X(Type, Field) DEFAULT_JSON_DESERIALIZER(&JsonPayload, &Project, Type, Field)
DECLARE_PROJECT_ENTITY
#undef X

    if (!DbInsertProject(&Project)) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    return HTTP_STATUS_OK;
}

HANDLER(UpdateProjectHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    json_value JsonPayloadValue;
    if (!JsonParse(Context->Arena, Context->Request.Body, &JsonPayloadValue)) return HTTP_STATUS_BAD_REQUEST;
    if (JsonPayloadValue.Type != JSON_OBJECT) return HTTP_STATUS_BAD_REQUEST;

    json_object JsonPayload = JsonPayloadValue.Object;

    project_update_entity Update;
#define X(Type, Field) DEFAULT_JSON_DESERIALIZER(&JsonPayload, &Update, Type, Field)
DECLARE_PROJECT_UPDATE_ENTITY
#undef X

    if (!DbUpdateProject(&Update)) return HTTP_STATUS_INTERNAL_SERVER_ERROR;

    return HTTP_STATUS_OK;
}

HANDLER(DeleteProjectHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    string_view ProjectId = Context->Request.Body;
    if (ProjectId.Count == 0) return HTTP_STATUS_BAD_REQUEST;

    // FIXME(oleh): The status returned here depends on the type of error
    // that happened inside the delete subroutine, it might be correct to return
    // 500 instead.
    if (!DbDeleteProjectById(ProjectId)) return HTTP_STATUS_NOT_FOUND;

    return HTTP_STATUS_OK;
}

HANDLER(GetProjectHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    string_view ProjectId = Context->Request.Body;
    if (ProjectId.Count == 0) return HTTP_STATUS_BAD_REQUEST;

    project_entity Project;
    if (!DbGetProjectById(Context->Arena, ProjectId, &Project)) return HTTP_STATUS_NOT_FOUND;

#define X(Type, Field) \
    JsonPutKey(SV_LIT(#Field)); \
    JsonPut_##Type(Project.Field);

    JsonBegin(Context->Arena);

    JsonBeginObject();
    DECLARE_PROJECT_ENTITY
#undef X
    JsonEndObject();

    string_view ProjectJson = JsonEnd();
    Context->Content = ProjectJson;
    return HTTP_STATUS_OK;
}

HANDLER(GetAllProjectsHandler) {
    if (Context->Request.Method != HTTP_GET) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    project_entity *Projects;
    uz ProjectsCount;

    if (!DbGetAllProjects(Context->Arena, &Projects, &ProjectsCount)) return HTTP_STATUS_INTERNAL_SERVER_ERROR;

    JsonBegin(Context->Arena);
    JsonBeginArray();

    for (uz ProjectIndex = 0; ProjectIndex < ProjectsCount; ++ProjectIndex) {
        project_entity Project = Projects[ProjectIndex];

        JsonPrepareArrayElement();

        JsonBeginObject();

#define X(Type, Field)                          \
        JsonPutKey(SV_LIT(#Field));             \
        JsonPut_##Type(Project.Field);

        DECLARE_PROJECT_ENTITY
#undef X

        JsonEndObject();
    }

    JsonEndArray();

    string_view ProjectJson = JsonEnd();
    Context->Content = ProjectJson;
    return HTTP_STATUS_OK;
}

HANDLER(InsertUserHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    json_value JsonPayloadValue;
    if (!JsonParse(Context->Arena, Context->Request.Body, &JsonPayloadValue)) return HTTP_STATUS_BAD_REQUEST;
    if (JsonPayloadValue.Type != JSON_OBJECT) return HTTP_STATUS_BAD_REQUEST;

    json_object JsonPayload = JsonPayloadValue.Object;

    user_entity User;

#define X(Type, Field) DEFAULT_JSON_DESERIALIZER(&JsonPayload, &User, Type, Field)
    DECLARE_USER_ENTITY
#undef X

    if (!DbInsertUser(&User)) return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    return HTTP_STATUS_OK;
}

HANDLER(LoginUserHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    json_value JsonPayloadValue;
    if (!JsonParse(Context->Arena, Context->Request.Body, &JsonPayloadValue)) return HTTP_STATUS_BAD_REQUEST;
    if (JsonPayloadValue.Type != JSON_OBJECT) return HTTP_STATUS_BAD_REQUEST;

    json_object JsonPayload = JsonPayloadValue.Object;

    string_view FirstName, LastName, Password;

    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("FirstName"), &FirstName)) return HTTP_STATUS_BAD_REQUEST;
    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("LastName"), &LastName)) return HTTP_STATUS_BAD_REQUEST;
    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("Password"), &Password)) return HTTP_STATUS_BAD_REQUEST;

    user_entity User;
    if (!DbGetUserByLogin(Context->Arena, FirstName, LastName, &User)) return HTTP_STATUS_NOT_FOUND;

    if (!(StringViewEqual(User.FirstName, FirstName) &&
          StringViewEqual(User.LastName, LastName) &&
          StringViewEqual(User.Password, Password))) return HTTP_STATUS_BAD_REQUEST;

    JsonBegin(Context->Arena);
    JsonBeginObject();

#define X(Type, Name) \
    JsonPutKey(SV_LIT(#Name)); \
    JsonPut_##Type(User.Name);

    DECLARE_USER_ENTITY
#undef X

    JsonEndObject();

    string_view UserJson = JsonEnd();
    Context->Content = UserJson;
    return HTTP_STATUS_OK;
}

HANDLER(RegisterUserHandler) {
    if (Context->Request.Method != HTTP_POST) return HTTP_STATUS_METHOD_NOT_ALLOWED;

    json_value JsonPayloadValue;
    if (!JsonParse(Context->Arena, Context->Request.Body, &JsonPayloadValue)) return HTTP_STATUS_BAD_REQUEST;
    if (JsonPayloadValue.Type != JSON_OBJECT) return HTTP_STATUS_BAD_REQUEST;

    json_object JsonPayload = JsonPayloadValue.Object;

    string_view FirstName, LastName, Password, Role;

    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("FirstName"), &FirstName)) return HTTP_STATUS_BAD_REQUEST;
    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("LastName"), &LastName)) return HTTP_STATUS_BAD_REQUEST;
    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("Password"), &Password)) return HTTP_STATUS_BAD_REQUEST;
    if (!JsonObjectGet_string_view(&JsonPayload, SV_LIT("Role"), &Role)) return HTTP_STATUS_BAD_REQUEST;

    user_entity DuplicateUser;
    if (DbGetUserByLogin(Context->Arena, FirstName, LastName, &DuplicateUser)) return HTTP_STATUS_BAD_REQUEST;

    user_entity User = CreateUserWithRandomId(Context->Arena, FirstName, LastName, Password, Role);
    if (!DbInsertUser(&User)) return HTTP_STATUS_NOT_FOUND;

    JsonBegin(Context->Arena);
    JsonBeginObject();

#define X(Type, Name) \
    JsonPutKey(SV_LIT(#Name)); \
    JsonPut_##Type(User.Name);

DECLARE_USER_ENTITY
#undef X

JsonEndObject();

    string_view UserJson = JsonEnd();
    Context->Content = UserJson;
    return HTTP_STATUS_OK;
}

int main() {
    srand(time(NULL));

    arena *TempArena = GetTempArena();

    string_view EnvFileContents;
    ASSERT(ReadFullFile(TempArena, ".env", &EnvFileContents));

    uz VarStart = 0;

    for (uz Index = VarStart; Index < EnvFileContents.Count; ++Index) {
        u8 Char = EnvFileContents.Items[Index];
        if (Char == '=') {
            uz VarEnd = Index;

            for (Index += 1; Index < EnvFileContents.Count; ++Index) {
                if (EnvFileContents.Items[Index] == '\n') break;
            }

            string_view VarNameSv = {.Items = EnvFileContents.Items + VarStart, .Count = VarEnd - VarStart};
            string_view VarValueSv = {.Items = EnvFileContents.Items + VarEnd + 1, .Count = Index - VarEnd - 1};

            const char *VarName = StringViewCloneCStr(TempArena, VarNameSv);
            const char *VarValue = StringViewCloneCStr(TempArena, VarValueSv);

            int Res = setenv(VarName, VarValue, 1);
            if (Res == -1) {
                const char *ErrorMessage = strerror(errno);
                PANIC_FMT("Coult not set the env variable '%s' to value '%s': %s", VarName, VarValue, ErrorMessage);
            }

            VarStart = Index + 1;
        }
    }

    DbInit();

    http_server Server;
    HttpServerInit(&Server);

    u16 ServerPort = 5959;

    HttpServerAttachHandler(&Server, "/", IndexHandler);

    HttpServerAttachHandler(&Server, "/insert-project", InsertProjectHandler);
    HttpServerAttachHandler(&Server, "/update-project", UpdateProjectHandler);
    HttpServerAttachHandler(&Server, "/delete-project", DeleteProjectHandler);
    HttpServerAttachHandler(&Server, "/get-project", GetProjectHandler);
    HttpServerAttachHandler(&Server, "/get-all-projects", GetAllProjectsHandler);

    HttpServerAttachHandler(&Server, "/insert-user", InsertUserHandler);
    HttpServerAttachHandler(&Server, "/login-user", LoginUserHandler);
    HttpServerAttachHandler(&Server, "/register-user", RegisterUserHandler);

    printf("Starting the server on port %u\n", ServerPort);
    HttpServerStart(&Server, ServerPort);
}
