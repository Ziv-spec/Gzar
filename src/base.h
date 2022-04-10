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


// Some language definitions: 
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

#define MIN(a, b) (((a)<(b)) ? (a) : (b))
#define MAX(a, b) (((a)>(b)) ? (a) : (b))

#define ArrayLength(arr) (sizeof(arr)/sizeof(arr[0]))

////////////////////////////////
/// Strings

typedef struct String8 String8; 
struct String8 {
    char *str;
    s64 size;
}; 

internal char *str8_to_cstring(String8 str) {
    char *result = (char *)malloc(str.size+1); 
    memcpy(result, str.str, str.size); 
    result[str.size] = '\0';
    return result;
}

internal int str8_compare(String8 s1, String8 s2) {
    
    if (s1.size > s2.size) {
        return 1; 
    }
    else if (s1.size< s2.size) {
        return -1; 
    }
    
    return strncmp(s1.str, s2.str, s1.size); 
}

internal String8 str8_lit(const char *s) {
    String8 lit; 
    lit.str = (char *)s; 
    lit.size = strlen(s); 
    
    return lit; 
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
/// Vector type 

// this is going to be somewhat of a generic way of doing dynamic arrays in C 
// NOTE(ziv): This is by no means a good way of doing this but it will work :)
typedef struct Vector Vector; 
struct Vector {
    int index;
    int capacity; 
    void **data;
}; 

#define DEFAULT_VEC_SIZE 16

internal Vector *init_vec() { 
    Vector *vec = (Vector *)malloc(sizeof(Vector)); 
    vec->capacity = DEFAULT_VEC_SIZE; vec->index = 0; 
    vec->data = malloc(sizeof(void *) * DEFAULT_VEC_SIZE);
    return vec;
}

internal Vector *vec_push(Vector *vec, void *elem) {
    Assert(vec); 
    
    if (vec->capacity <= vec->index) {
        vec->capacity = vec->capacity*2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity); 
    }
    vec->data[vec->index++] = elem;
    return vec;
}

internal void *vec_pop(Vector *vec) {
    Assert(vec && vec->index > 0); 
    return vec->data[vec->index--];
}

internal bool vec_equal(Vector *src1, Vector *src2) {
    
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


////////////////////////////////
/// Hashtable type 

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
    for (s64 i = 0; i < str.size; i++ ) {
        hash ^= (uint64_t)(unsigned char)(str.str[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}


#define DEFAULT_MAP_SIZE 16
internal Map *init_map(int elem_size) {
    
    Map *map = malloc(sizeof(Map)); 
    if (!map) {
        return NULL; 
    }
    
    map->buckets = calloc(sizeof(Bucket), DEFAULT_MAP_SIZE);
    Assert(map->buckets); 
    
    map->capacity  = DEFAULT_MAP_SIZE; 
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

// NOTE(ziv): this function does not remove the element from the map 
// it only returns it's value
internal void *map_peek(Map *map, String8 key) {
    
    u64 hash = str8_hash(key); 
    size_t index = hash & (map->capacity-1);
    while (map->buckets[index].key.str != NULL) {
        
        if (str8_compare(map->buckets[index].key, key) == 0) {
            return map->buckets[index].value;
        }
        
        // linear probing
        index++; 
        if (index >= map->capacity) { 
            index = 0; 
        }
        
    }
    
    return NULL;
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
        Bucket *new_buckets = calloc(map->capacity, sizeof(void *));
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
        
        free(map->buckets); // free old bucket array
        
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

typedef long long Register; 

#endif //BASE_H
