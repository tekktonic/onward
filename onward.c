/* Copyright (c) 2017 Daniel Wilkins <tekk@parlementum.net> */
/* Permission to use, copy, modify, and distribute this software for any */
/* purpose with or without fee is hereby granted, provided that the above */
/* copyright notice and this permission notice appear in all copies. */
/* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES */
/* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF */
/* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR */
/* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES */
/* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN */
/* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF */
/* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>


/* Macros */
#define checkmem(value) do {if(!value){ onlog(FATAL, "Failed to allocate memory in ", __LINE__);}}while(0)



/* Types */
enum loglevel {
    FATAL,
    STANDARD,
    VERBOSE,
    DEBUG
};

enum type {
    NUM,
    STRING,
    BUILTIN,
    WORD,
    
};

struct value;

struct stack {
    size_t capacity;
    size_t used;
    struct value *values;
};

struct value {
    enum type t;
    union {
        double n;
        char *s;
        struct stack w;
    };
};

enum tok {
    NUMTOK,
    QUOTETOK,
    IDENTTOK,
    PLUSTOK,
    MINUSTOK,
    STARTOK,
    SLASHTOK,
    DOTTOK,
    WORDTOK,
    WORDENDTOK,
    SWAPTOK,
    LOGTOK,
    POPTOK,
    PRINTTOK,
    REPTOK,
    DUPTOK,
    EXITTOK
};

struct token {
    enum tok type;
    struct value v;
};

// A word is simply a stack, it's evaluated by pushing its contents onto *our* stack. We do need to be careful to 
typedef struct stack word;

struct wordentry {
    char *name;
    word word;
    struct wordentry *next;
};


/* Globals */
unsigned int logset=0;


/* Function Decls */
void onlog(enum loglevel level, char *string, int line);

void stack_push(struct stack *stack, struct value v);
struct value stack_pop(struct stack *stack);

void value_free(struct value *v);
void value_print(struct value *v);

void word_free(word *w);

int words_hash(char *name, int max);
word *words_get(char *name);
void words_put(char *name, word word);

void builtin_plus(struct stack *stack);
void builtin_minus(struct stack *stack);
void builtin_star(struct stack *stack);
void builtin_slash(struct stack *stack);
void builtin_dump(struct stack *stack);
void builtin_exit(struct stack *stack);

char *handle_number(struct stack *stack, char *string);
char *handle_string(struct stack *stack, char *string);
char *handle_word_builtin(struct stack *stack, char *string);

char *chomp(char *string);
void eval(struct stack *stack, char *string);

/* Function Impls */

void stack_push(struct stack *stack, struct value v) {
    if (stack->capacity == stack->used) {
        onlog(DEBUG, "Stack resize", 0);
        stack->values = realloc(stack->values, stack->capacity *= 2);
        checkmem(stack->values);
    }

    stack->values[stack->used++] = v;
}

// Never shrink the stack for now.
struct value stack_pop(struct stack *stack) {
    fprintf(stderr, "Popping a stack with %d elms\n", stack->used);
    if (stack->used == 0) {
        onlog(FATAL, "Attempting to pop empty stack", 0);
    }
    return stack->values[--stack->used];

}

void value_free(struct value *v) {
    switch(v->t) {
    case WORD:
        word_free(&(v->w));
        break;
    case STRING:
        free(v->s);
        break;
    case BUILTIN:
        onlog(FATAL, "Attempt to redefine a builtin", 0);
        break;
    default:
        break;
    }
}

void value_print(struct value *v) {
    switch (v->t) {
    case WORD:
        builtin_dump(&(v->w));
        break;
    case NUM:
        printf("%lf", v->n);
        break;
    case STRING:
        printf("\"%s\"", v->s);
        break;
    case BUILTIN:
        printf("<builtin>");
        break;
    default:
        printf("<unknown %p>", v->s);
    }
}

void word_free(word *w) {
    for (size_t i = 0; i < w->used; i++) {
        value_free(&(w->values[i]));
    }

    free(w->values);
    w->values = NULL;
    w->used = 0;
    w->capacity = 0;
}

 #define entrycount 128

struct words {
    struct wordentry *entries[entrycount];
};

struct words words = {};

int words_hash(char *name, int max) {
    int prime = 7757;
    int ret = 0;
    for (int i = 0; name[i]; i++) {
        ret += pow(prime, i) + name[i];
    }

    return ret % max;
}

word *words_get(char *name) {
    int offset = words_hash(name, entrycount);

    struct wordentry *cur = words.entries[offset];
    for (;cur; cur = cur->next) {
        if (!strcmp(cur->name, name)) {
            return &(cur->word);
        }
    }
    return NULL;
}

void words_put(char *name, word word) {
    int offset = words_hash(name, entrycount);

    struct wordentry *cur = words.entries[offset];
    struct wordentry *trailer;

    // Make sure that if word is already defined, overwrite it
    for (;cur; cur = cur->next) {
        if (!strcmp(cur->name, name)) {
            word_free(&cur->word);
            cur->word = word;

        }

        trailer = cur;
    }

    // If we hit here, then cur is null and trailer is the last element
    trailer->next = malloc(sizeof(struct wordentry));
    checkmem(trailer->next);
    cur = trailer->next;
    size_t namelen = strlen(name) +1;
    checkmem(namelen);
    
    // Ensure that worst case we have a null terminator preserved.
    strcpy(cur->name, name);

    cur->word = word;

    onlog(DEBUG, "Created a new word", 0);
}

#undef entrycount

void onlog(enum loglevel level, char *string, int line) {
    if (level >= logset) {
        if (line) {
            fprintf(stderr, "Internal error: %s at %d\n", string, line);
        }
        else {
            fprintf(stderr, "%s\n", string);
        }
    }

    if (level == FATAL) {

        exit(1);
    }
}



/* Builtins */
void builtin_plus(struct stack *stack) {
    struct value v1, v2;

    v1 = stack_pop(stack);
    v2 = stack_pop(stack);

    if (v1.t != NUM || v2.t != NUM)
        onlog(FATAL, "Attempting to add non-numbers", 0);
    double result = v1.n + v2.n;

    printf("%lf + %lf = %lf\n", v1.n, v2.n, result);
    stack_push(stack, (struct value){.t=NUM,.n=result});
}

void builtin_minus(struct stack *stack) {
    struct value v1, v2;

    v1 = stack_pop(stack);
    v2 = stack_pop(stack);

    if (v1.t != NUM || v2.t != NUM)
        onlog(FATAL, "Attempting to add non-numbers", 0);

    stack_push(stack, (struct value){.t=NUM,.n=(v1.n-v2.n)});
}

void builtin_star(struct stack *stack) {
    struct value v1, v2;

    v1 = stack_pop(stack);
    v2 = stack_pop(stack);

    if (v1.t != NUM || v2.t != NUM)
        onlog(FATAL, "Attempting to add non-numbers", 0);

    stack_push(stack, (struct value){.t=NUM,.n=(v1.n*v2.n)});
}

void builtin_slash(struct stack *stack) {
    struct value v1, v2;

    v1 = stack_pop(stack);
    v2 = stack_pop(stack);

    if (v1.t != NUM || v2.t != NUM)
        onlog(FATAL, "Attempting to add non-numbers", 0);

    stack_push(stack, (struct value){.t=NUM,.n=(v2.n/v1.n)});
}


void builtin_dump(struct stack *stack) {
    printf("[");
    for (size_t i = 0; i < stack->used; i++) {
        value_print(&(stack->values[i]));
        printf(", ");
    }

    printf("]\n");
}


void builtin_exit(struct stack *stack) {
    struct value v1 = stack_pop(stack);

    if (v1.t != NUM) {
        onlog(FATAL, "Attempting to exit without a status code", 0);
    }

    exit((int)v1.n);
}

char *chomp(char *string) {
    while(isspace(*string)) {
        string++;
    }
    return string;
}

void eval(struct stack *stack, char *string) {
    char *curr = string;
    while (*(curr = chomp(curr))) {
        if (isdigit(*curr)) {
            curr = handle_number(stack, curr);
        }
        else if (*curr == '"') {
            curr = handle_string(stack, curr);
        }

        else {
            curr = handle_word_builtin(stack, curr);
        }
        
    }
}

char *handle_number(struct stack *stack, char *string) {
    double value = 0;

    while (isdigit(*(string))) {
        value *= 10;
        value += (*(string++) - 48);
    }

    stack_push(stack, (struct value){.t=NUM,.n=value});
    
    return string;
}

char *handle_string(struct stack *stack, char *string) {
    char *start = ++string;
    char *end;
    while (*(string) != '"') {
        string++;
    }
    end = string;

    fprintf(stderr, "Found string of length %d\n", (int)(end-start));

    char *value = calloc(end-start+1, sizeof(char));

    strncpy(value, start, end-start);
    
    stack_push(stack, (struct value){.t=STRING,.s=value});

    // We end on the last " I believe.
    return string+1;
}

char *handle_word_builtin(struct stack *stack, char *string) {
    switch (*string) {
    case '+':
        builtin_plus(stack);
        break;
    case '-':
        builtin_minus(stack);
        break;
    case '*':
        builtin_star(stack);
        break;
    case '/':
        builtin_slash(stack);
        break;
    case 'd':
        builtin_dump(stack);
        break;

    default:
        onlog(FATAL, "Invalid builtin", 0);
        break;
    }

    return string+1;
}

int main(void) {
    char buf[512] = {};
    struct stack stack = {.capacity = 512, .used=0, .values = malloc(512*sizeof(struct value))};
    while (1) {
        eval(&stack, fgets(buf, 511, stdin));
    }
}
