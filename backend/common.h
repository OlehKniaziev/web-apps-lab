#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define ASSERT(X) do {                                                  \
        if (!(X)) {                                                     \
            fprintf(stderr, "%s:%d: ASSERTION FAILED: %s\n", __FILE__, __LINE__, #X); \
            abort();                                                    \
        }                                                               \
    } while (0)

#define MEMORY_ZERO(Ptr, Size) (memset((Ptr), 0, (Size)))

#define STRUCT_ZERO(Ptr) MEMORY_ZERO((Ptr), sizeof(*(Ptr)))

#define PANIC(Msg) do {                                                 \
        fprintf(stderr, "%s:%d: PROGRAM PANICKED: %s\n", __FILE__, __LINE__, Msg); \
        abort();                                                    \
    } while (0)

#define PANIC_FMT(Fmt, ...) do {                                        \
        fprintf(stderr, "%s:%d: PROGRAM PANICKED: " Fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
        abort();                                                    \
    } while (0)

#define SV_FMT "%.*s"
#define SV_ARG(Sv) (int)(Sv).Count, (const char *)(Sv).Items
#define SV_LIT(Lit) ((string_view){.Items = (u8 *)(Lit), .Count = strlen(Lit)})

#define UNREACHABLE() PANIC("Encountered unreachable code!")

#define TODO_MSG(Msg) do {                                        \
        fprintf(stderr, "%s:%d: TODO: " Msg "\n", __FILE__, __LINE__); \
        abort();                                                  \
    } while (0)

#define TODO() TODO_MSG("NOT IMPLEMENTED")

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u8 b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef size_t uz;

typedef struct {
    u8 *Items;
    uz Count;
} string_view;

static inline b32 StringViewEqualCStr(string_view Sv, const char *CStr) {
    uz CStrLength = strlen(CStr);
    if (Sv.Count != CStrLength) return 0;

    for (uz I = 0; I < Sv.Count; ++I) {
        if (Sv.Items[I] != CStr[I]) return 0;
    }

    return 1;
}

static inline b32 StringViewEqual(string_view Lhs, string_view Rhs) {
    if (Lhs.Count != Rhs.Count) return 0;

    for (uz I = 0; I < Lhs.Count; ++I) {
        if (Lhs.Items[I] != Rhs.Items[I]) return 0;
    }

    return 1;
}

typedef struct {
    u8 *Items;
    uz Capacity;
    uz Offset;
} arena;

static inline uz AlignForward(uz Size, uz Alignment) {
    return Size + ((Alignment - (Size & (Alignment - 1))) & (Alignment - 1));
}

static inline void *ArenaPush(arena *Arena, uz Size) {
    Size = AlignForward(Size, sizeof(uz));
    uz AvailableBytes = Arena->Capacity - Arena->Offset;
    if (AvailableBytes < Size) PANIC_FMT("Arena out of memory for requested size %zu!", Size);

    void *Ptr = Arena->Items + Arena->Offset;
    Arena->Offset += Size;
    return Ptr;
}

static inline void ArenaInit(arena *Arena, uz Capacity) {
    Arena->Capacity = Capacity;
    Arena->Offset = 0;
    Arena->Items = (u8 *)calloc(Capacity, sizeof(u8));
}

static inline string_view ArenaFormat(arena *Arena, const char *Fmt, ...) {
    va_list Args;
    va_start(Args, Fmt);
    uz BytesNeeded = vsnprintf(NULL, 0, Fmt, Args);
    ++BytesNeeded; // NOTE(oleh): Null terminator.
    va_end(Args);

    u8 *Buffer = ArenaPush(Arena, BytesNeeded);
    va_start(Args, Fmt);
    vsprintf((char *)Buffer, Fmt, Args);
    va_end(Args);

    return (string_view) {.Items = Buffer, .Count = BytesNeeded - 1};
}

void ArenaPop(arena *, uz);

static inline void ArenaReset(arena *Arena) {
    Arena->Offset = 0;
}

#define ARENA_NEW(Arena, Type) ((Type *)MEMORY_ZERO(ArenaPush((Arena), sizeof(Type)), sizeof(Type)))

#endif // COMMON_H_
