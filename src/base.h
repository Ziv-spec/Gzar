/* date = March 20th 2022 5:13 pm */

#ifndef BASE_H
#define BASE_H

#define internal static

#if DEBUG
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while(0)
#else
#define Assert(cond)
#endif

// This is a bit of a macro hack to get the compiler to count
// the arguments passed to a function. This is so I wouldn't need
// to do this manually. NOTE(ziv): This works only on the msvc compiler
#define COUNT_ARGS(...) INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(a) a
#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, count, ...) count

#include <stdint.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef unsigned char bool;

#define true 1
#define false 0

#define is_digit(ch) ('0'<=(ch) && (ch)<='9')
#define is_alpha(ch) (('A'<=(ch) && (ch)<='Z') || ('a'<=(ch) && (ch)<='z'))
#define is_alphanumeric(ch) (is_digit(ch) || is_alpha(ch) || ch == '_')

#define ABS(a) (((a) < 0) ? -(a) : (a))
#define MIN(a, b) (((a)<(b)) ? (a) : (b))
#define MAX(a, b) (((a)>(b)) ? (a) : (b))

#define ArrayLength(arr) (sizeof(arr)/sizeof(arr[0]))

////////////////////////////////
/// Memory

#define DEFAULT_POOL_SIZE 4*1024 // a page size on windows

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2*sizeof(void *))
#endif

internal bool is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

internal uintptr_t align_forward(uintptr_t ptr, size_t align) {
    uintptr_t p, a, modulo;
	
    Assert(is_power_of_two(align));
	
    p = ptr;
    a = (uintptr_t)align;
    modulo = p & (a-1); // same as (p % a) buf faster because 'a' is a power of two
	
    if (modulo != 0) {
        p += a - modulo;
    }
	
    return p;
}

typedef struct M_Arena M_Arena;
struct M_Arena {
    unsigned char *buff;
    size_t len;
    size_t prev_offset;
    size_t curr_offset;
	
    M_Arena *next;
};

internal void *arena_alloc_align(M_Arena *a, size_t size, size_t align) {
    // Align 'curr_offset' forward to the specified alignment
	uintptr_t curr_ptr = (uintptr_t)a->buff + (uintptr_t)a->curr_offset;
	uintptr_t offset = align_forward(curr_ptr, align);
	offset -= (uintptr_t)a->buff; // Change to relative offset
	
	// Check to see if the backing memory has space left
	if (offset+size <= a->len) {
		void *ptr = &a->buff[offset];
		a->prev_offset = offset;
		a->curr_offset = offset+size;
		
		// Zero new memory by default
		memset(ptr, 0, size);
		return ptr;
	}
	// Return NULL if the arena is out of memory (or handle differently)
	return NULL;
}

// Because C doesn't have default parameters
internal void *arena_alloc(M_Arena *a, size_t size) {
	return arena_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

internal void arena_init(M_Arena *a, void *backing_bufffer, size_t backing_bufffer_length) {
	a->buff = (unsigned char *)backing_bufffer;
	a->len = backing_bufffer_length;
	a->curr_offset = 0;
	a->prev_offset = 0;
    a->next = NULL;
}

void arena_free(M_Arena *a, void *ptr) {
    a, ptr;
	// Do nothing
}

void *arena_resize_align(M_Arena *a, void *old_memory, size_t old_size, size_t new_size, size_t align) {
	unsigned char *old_mem = (unsigned char *)old_memory;
	
	Assert(is_power_of_two(align));
	
	if (old_mem == NULL || old_size == 0) {
		return arena_alloc_align(a, new_size, align);
	}
    else if (a->buff <= old_mem && old_mem < a->buff+a->len) {
		if (a->buff+a->prev_offset == old_mem) {
			a->curr_offset = a->prev_offset + new_size;
			if (new_size > old_size) {
				// Zero the new memory by default
				memset(&a->buff[a->curr_offset], 0, new_size-old_size);
			}
			return old_memory;
		}
        else {
			void *new_memory = arena_alloc_align(a, new_size, align);
			size_t copy_size = old_size < new_size ? old_size : new_size;
			// Copy across old memory to the new memory
			memmove(new_memory, old_memory, copy_size);
			return new_memory;
		}
		
	}
    else {
		Assert(!"Memory is out of bounds of the bufffer in this arena");
		return NULL;
	}
}

// Because C doesn't have default parameters
void *arena_resize(M_Arena *a, void *old_memory, size_t old_size, size_t new_size) {
	return arena_resize_align(a, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

// TODO(ziv) Check whether this is needed
void arena_free_all(M_Arena *a) {
	a->curr_offset = 0;
	a->prev_offset = 0;
}

typedef struct M_Pool M_Pool;
struct M_Pool {
    M_Arena *first;
    M_Arena *last;
	
	M_Arena *scratch;
	
    size_t pool_size;
};

internal void pools_init(M_Pool *p) {
    Assert(p != NULL && p->pool_size > 0);
    Assert(p->last == NULL || p->first == NULL); // Expecting a empty m_pool to initialize
	
    M_Arena *a = (M_Arena *)realloc(NULL, p->pool_size);
    arena_init(a, (unsigned char *)a + sizeof(M_Arena), p->pool_size-sizeof(M_Arena));
	
    p->last  = a;
    p->first = a;
	
	p->scratch = NULL;
	
}

internal void *pool_alloc_align(M_Pool *m, size_t size, size_t align) {
    Assert(m != NULL && size > 0);
	
    void *result = arena_alloc_align(m->last, size, align);
    if (result) {
        return result;
    }
	
    M_Arena *a = (M_Arena *)realloc(NULL, m->pool_size);
    Assert(a || !"Memory is out of bounds of the bufffer in this arena");
    arena_init(a, (unsigned char *)a + sizeof(M_Arena), m->pool_size-sizeof(M_Arena));
	
    m->last->next = a;
    m->last = a;
	
    return arena_alloc_align(a, size, align);
}

internal void *pool_alloc(M_Pool *m, size_t size) {
    return pool_alloc_align(m, size, DEFAULT_ALIGNMENT);
}

internal void *pool_resize_align(M_Pool *m, void *old_memory, size_t old_size, size_t new_size, size_t align) {
    Assert(m != NULL);
	
    void *new_memory = arena_resize_align(m->last, old_memory, old_size, new_size, align);
    if (new_memory) {
        return new_memory;
    }
	
    M_Arena *a = (M_Arena *)realloc(NULL, m->pool_size);
    Assert(a || !"Memory is out of bounds of the bufffer in this arena");
    arena_init(a, (unsigned char *)a + sizeof(M_Arena), m->pool_size-sizeof(M_Arena));
	
    m->last->next = a;
    m->last = a;
	
    new_memory = arena_alloc_align(a, new_size, align);
    memcpy(new_memory, old_memory, old_size);
	
    return new_memory;
	
}

internal void *pool_resize(M_Pool *m, void *old_memory, size_t old_size, size_t new_size) {
    return pool_resize_align(m, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

internal M_Arena *pool_get_scratch(M_Pool *m) {
	if (m->scratch) {
		return m->scratch;
	}
	
	M_Arena *a = (M_Arena *)realloc(NULL, 0x2000 + sizeof(M_Arena));
    Assert(a || !"Memory is out of bounds of the bufffer in this arena");
    arena_init(a, (unsigned char *)a + sizeof(M_Arena), m->pool_size-sizeof(M_Arena));
	
	m->scratch = a; 
	return m->scratch;
}

internal void pool_free(M_Pool *m) {
	
    M_Arena *a = m->first;
    M_Arena *temp;
	
    while (a) {
        temp = a->next;
        free(a);
        a = temp;
    }
	
}

////////////////////////////////
/// Strings

internal int _atoi(char *value) {
	int exp_table[] = { 
		1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 
	};
	int size = 0, final_size = 0;
	for (; '0' <= value[size] && value[size] <= '9'; size++);
	for (int i = size-1; i >= 0; i--) {
		final_size += exp_table[size-1-i] * (value[i] - '0');
	}
	return final_size;
}

typedef struct String8 String8;
struct String8 {
    char *str;
     size_t size;
};

internal char *str8_to_cstring(String8 str) {
    char *result = (char *)malloc(str.size+1);
    memcpy(result, str.str, str.size);
    result[str.size] = '\0';
    return result;
}

internal int str8_compare(String8 s1, String8 s2) {
    if (s1.size > s2.size) return 1;
    if (s1.size < s2.size) return -1;
    return strncmp(s1.str, s2.str, s1.size);
}

internal String8 str8_lit(const char *s) {
    String8 lit = { 
		.str = (char *)s,
		.size = strlen(s)
	};
    return lit;
}

internal String8 str8_duplicate(M_Arena *m, String8 s) {
	char *dup = arena_alloc(m, s.size); 
	memcpy(dup, s.str, s.size); 
	return (String8){ dup, s.size };
}

internal int get_line_number(char *start, String8 str) {

    int line = 1;
    char *end = str.str;
    for (; start < end; start++) {
        if (*start == '\n') line++;
    }

    return line;
}

internal int get_character_number(char *start, String8 str) {

    char *end = str.str;
    char *line_beginning = NULL;
    for (; start < end; start++) {
        if (*start == '\n')
            line_beginning = start;
    }

    int ch = 1;
    for (; line_beginning < end; line_beginning++, ch++);

    return ch;
}

////////////////////////////////
/// Vectors

// this is going to be somewhat of a generic way of doing dynamic arrays in C
// NOTE(ziv): This is by no means a good way of doing this but it will work :)
typedef struct Vector Vector;
struct Vector {
    int index;
    int capacity;
    void **data;
};

#define DEFAULT_VEC_SIZE 16

internal Vector *vec_init() {
    Vector *vec = (Vector *)malloc(sizeof(Vector) + sizeof(void *)*DEFAULT_VEC_SIZE);
    vec->capacity = DEFAULT_VEC_SIZE; vec->index = 0;
    vec->data = (void **)(((char *)vec) + sizeof(Vector));
    return vec;
}

internal Vector *vec_push(M_Pool *m, Vector *vec, void *elem) {
    Assert(vec);

    if (vec->capacity <= vec->index) {
        int old_capacity = vec->capacity;
        int new_capacity = vec->capacity*2;
        vec->data = pool_resize(m, vec->data, sizeof(void *) * old_capacity, sizeof(void *) * new_capacity);

        vec->capacity = new_capacity;
    }
    vec->data[vec->index++] = elem;
    return vec;
}

internal void *vec_pop(Vector *vec) {
    Assert(vec && vec->index > 0);
    return vec->data[vec->index--];
}

internal bool vec_equal(Vector *src1, Vector *src2) {
    Assert(src1 && src2);
    if (src1->index != src2->index)
        return false;

    // NOTE(ziv): maybe I should think of using memcpy
    int len = src1->index;
    for (int i = 0; i < len; i++) {
        if (src1->data[i] != src2->data[i]) {
            return false;
        }
    }

    return true;
}

internal void vec_free(Vector *v) {
	free(v);
}

////////////////////////////////
/// Hashtables

typedef struct Bucket Bucket;
struct Bucket {
    String8 key;
    void *value;
};

typedef struct Map Map;
struct Map {
    Bucket *buckets;
    int capacity;
    int count;
    int elem_size;
};


#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static u64 str8_hash(String8 str) {
    u64 hash = FNV_OFFSET;
    for (s64 i = 0; i < (s64)str.size; i++ ) {
        hash ^= (uint64_t)(unsigned char)(str.str[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}


#define DEFAULT_MAP_SIZE 16

internal Map *init_map(int elem_size) {

    Map *map = malloc(sizeof(Map));
    if (!map) { return NULL; }

    map->buckets = calloc(DEFAULT_MAP_SIZE, sizeof(Bucket));
    Assert(map->buckets);

    map->capacity = DEFAULT_MAP_SIZE;
    map->count = 0;
    map->elem_size = elem_size;

    return map;
}

internal void map_destroy(Map *map) {
    free(map->buckets);
    free(map);
}

internal void *map_get(Map *map, String8 key) {

    u64 hash = str8_hash(key);
    size_t index = hash & (map->capacity-1);
    while (map->buckets[index].key.str != NULL) {

        if (str8_compare(map->buckets[index].key, key) == 0) {
            void *v = map->buckets[index].value;
            map->buckets[index].value = NULL; // mark as empty space
            return v;
        }

        // linear probing
        index++;
        if (index >= map->capacity) {
            index = 0;
        }

    }

    map->count--;
    return NULL;
}

internal Bucket *map_peek_bucket(Map *map, String8 key) {
	
    u64 hash = str8_hash(key);
    size_t index = hash & (map->capacity-1);
    while (map->buckets[index].key.str != NULL) {
		
        if (str8_compare(map->buckets[index].key, key) == 0) {
			return &map->buckets[index];
        }
		
        // linear probing
        index++;
        if (index >= map->capacity) {
            index = 0;
        }
		
    }
	
    return NULL;
}

// NOTE(ziv): this function does not remove the element from the map
// it only returns it's value
internal void *map_peek(Map *map, String8 key) {
	Bucket *b = map_peek_bucket(map, key); 
	if (b) return b->value;
	return b;
}

internal bool map_set_bucket(Map *map, String8 key, void *value) {

    size_t hash = str8_hash(key);
    size_t index = hash & (map->capacity-1);

    // loop until we find an empty bucket
    while (map->buckets[index].value != NULL) {

        if (str8_compare(key, map->buckets[index].key) == 0) {
            // update value in bucket
            map->buckets[index].value = value;
            return true;
        }

        // linear probing
        index++;
        if (index >= map->capacity) {
            index = 0;
        }
    }

    if (!key.str) {
        return false;
    }

    map->count++;
    map->buckets[index].value = value;
    map->buckets[index].key = key;
    return true;
}

internal bool map_set(Map *map, String8 key, void *value) {
    Assert(value != NULL);

    if (value == NULL) {
        return false;
    }

    // if more than half of the map is used, expand it
    if (map->count>= map->capacity/2) {
        int new_capacity = map->capacity * 2;
        Assert(map->capacity < new_capacity); // overflow could happen
        Bucket *new_buckets = (Bucket *)calloc(new_capacity, sizeof(Bucket));
        if (!map->buckets) {
            return false; // failed to allocated memory
        }

        // new map to resize to
        Map nm = { new_buckets, new_capacity, 0, map->elem_size };
        // copy info from old map to new map
        for (int i = 0; i < map->capacity; i++) {
            Bucket bucket = map->buckets[i];
            if (bucket.key.str)
                map_set_bucket(&nm, bucket.key, bucket.value);
        }

        free(map->buckets);

        // copy info to the new array to given map
        map->buckets = new_buckets;
        map->capacity = new_capacity;
    }

    return map_set_bucket(map, key, value);
}

typedef struct Map_Iterator Map_Iterator;
struct Map_Iterator {
    Map *map;
    s64 index;

    String8 key;
    void *value;
};

internal Map_Iterator map_iterator(Map *m) {
    Map_Iterator it;
    it.map = m;
    it.index = 0;
    return it;
}

internal bool map_next(Map_Iterator *it) {

    // Loop till we've hit end of entries array.
    Map *map = it->map;
    while (it->index < map->capacity) {
        size_t i = it->index;
        it->index++;
        if (map->buckets[i].value != NULL) {

            // Found next non-empty item, update iterator key and value.
            Bucket bucket = map->buckets[i];
            it->key = bucket.key;
            it->value = bucket.value;
            return true;
        }
    }
    return false;
}

////////////////////////////////
/// Perf counters and more

// NOTE(ziv): THIS WILL WORK ONLY IF YOU COMPILE WITH MSVC

#if TIMINGS

enum {
    C_LEXER,
#if 1
    C_LEXER_IDENTIFIER,
    C_LEXER_DOUBLE_ASCII,
    C_LEXER_TK_TO_STREAM,
    C_LEXER_ASCII,
    C_LEXER_NUMBER,
    C_LEXER_STRING,
    C_LEXER_TRASH,
#endif
    C_PARSER,
    C_SEMA,
    C_CODEGEN,
    C_FINAL,
    C_COUNT
};


char *clock_names[] = {
    [C_LEXER] = "lexer",
#if TIMINGS
    [C_LEXER_IDENTIFIER]   = "lexer identifier",
    [C_LEXER_TK_TO_STREAM] = "lexer tk assign",
    [C_LEXER_DOUBLE_ASCII] = "lexer double ascii",
    [C_LEXER_STRING]       = "lexer string",
    [C_LEXER_TRASH]        = "lexer trash",
    [C_LEXER_NUMBER]       = "lexer number",
    [C_LEXER_ASCII]        = "lexer ascii",
#endif
    [C_PARSER]  = "parser",
    [C_SEMA]    = "sema",
    [C_CODEGEN] = "codegen",
    [C_FINAL]   = "final"
};

static f32 clock_counters[C_COUNT];

#define CLOCK_START(name) LARGE_INTEGER ST_##name, ET_##name; QueryPerformanceCounter(&ST_##name)
#define CLOCK_END(name) QueryPerformanceCounter(&ET_##name); clock_counters[C_##name] += (float)(ET_##name.QuadPart - ST_##name.QuadPart)

#else
#define CLOCK_START(name)
#define CLOCK_END(name)
#endif // TIMINGS


#endif //BASE_H
