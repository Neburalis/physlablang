#ifndef BASE_H
#define BASE_H

#include <stdlib.h>
#include <stdio.h>

#define MEOW() printf("MEOW\n")

#define $d(x) printf("%s = %d", #x, (x))
#define $f(x) printf("%s = %f", #x, (x))
#define $lf(x) printf("%s = %lf", #x, (x))

#define global   static
#define local    static
#define function static

#ifndef STR_
#define STR_(s) #s
#endif
#ifndef STR
#define STR(s)  STR_(s)
#endif
#ifndef GLUE_
#define GLUE_(a, b) a##b
#endif
#ifndef GLUE
#define GLUE(a, b) GLUE_(a, b)
#endif

static inline int randint(int min, int max) {
    return (int) (rand() % (max - min)) + min;
}

#define ARRAY_COUNT(a) (sizeof(a) / (sizeof(a[0])))

#ifdef _STDLIB_H

    #define TYPED_CALLOC(NMEMB, TYPE) \
        (TYPE *) calloc((NMEMB), sizeof(TYPE));

    #define TYPED_MALLOC(TYPE) \
        (TYPE *) calloc(1, sizeof(TYPE));

    #define TYPED_REALLOC(PTR, NMEM, TYPE) \
        (TYPE *) realloc((PTR), NMEM * sizeof(TYPE));

    #define FREE(ptr)     \
        free((ptr));      \
        (ptr) = nullptr;

#endif

#define DIE()  \
    abort();

#define VERIFY(cond, code) \
    if (!(cond)) {code};

#define POSASSERT(cond) \
    VERIFY(cond, ERROR_MSG("possasert killed program, the cond (" #cond ") was false"); DIE();)

#endif // BASE_H