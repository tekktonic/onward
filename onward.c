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
#include <stdbool.h>
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

enum builtins {
    PLUS,
    MINUS,
    STAR,
    SLASH,
    DUMP,
    EXIT,
    REP,
    DUP,
    END,
    BEGIN,
    POP,
    SWAP,
    LOG,

};



struct value {
    enum type t;
    union {
        double n;
        char *s;
        struct stack w;
        enum builtins b;
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
    DUMPTOK,
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

struct value value_copy(struct value *v);
void value_eval(struct stack *stack, struct value *v);
void value_builtin_eval(struct stack *stack, struct value *v);
void value_word_eval(struct stack *stack, struct value *v);
void value_free(struct value *v);
void value_print(struct value *v);

word init_word(void);
void word_free(word *w);
word word_copy(word *w);

int words_hash(char *name, int max);
word *words_get(char *name);
void words_put(char *name, word word);


void builtin_plus(struct stack *stack);
void builtin_minus(struct stack *stack);
void builtin_star(struct stack *stack);
void builtin_slash(struct stack *stack);
void builtin_end(struct stack *stack);
void builtin_dump(struct stack *stack);
void builtin_exit(struct stack *stack);

char *handle_number(struct stack *stack, char *string);
char *handle_string(struct stack *stack, char *string);
char *handle_word_builtin(struct stack *stack, char *string);

char *chomp(char *string);
void eval(struct stack *stack, char *string);
bool match(char *string, char *match);

/* Function Impls */

void stack_push(struct stack *stack, struct value v) {
    printf("Pushing ");
    value_print(&v);
    printf("\n");
    if (stack->capacity == stack->used) {
        onlog(DEBUG, "Stack resize", 0);
        stack->values = realloc(stack->values, stack->capacity *= 2);
        checkmem(stack->values);
    }

    stack->values[stack->used++] = v;
}

// Never shrink the stack for now.
struct value stack_pop(struct stack *stack) {
    if (stack->used == 0) {
        onlog(FATAL, "Attempting to pop empty stack", 0);
    }
    return stack->values[--stack->used];

}

struct value value_copy(struct value *v) {
    struct value ret ={.t = v->t};
    
    switch (v->t) {
    case NUM:
        ret.n = v->n;
        break;
    case STRING:
        ret.s = calloc(strlen(v->s)+1, sizeof(char));
        checkmem(ret.s);
        strcpy(ret.s, v->s);
        break;

    case WORD:
        ret.w = word_copy(&v->w);
        break;
    default:
        break;
    }

    return ret;
}

void value_free(struct value *v) {
    switch(v->t) {
    case WORD:
        word_free(&(v->w));
        break;
    case STRING:
    case BUILTIN:
        free(v->s);
        break;
    default:
        break;
    }
}

void value_print(struct value *v) {
    char *builtins[] = {"PLUS", "MINUS", "STAR", "SLASH", "DUMP", "EXIT", "REP", "DUP",
                        "END", "BEGIN", "POP", "SWAP", "LOG"};
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
        printf("<builtin %s> ", builtins[v->b]);
        break;
    default:
        printf("<unknown %p>", v->s);
    }
}

void value_eval(struct stack *stack, struct value *v) {

    switch (v->t) {
    case BUILTIN:
        value_builtin_eval(stack, v);
        break;
    case WORD:
        value_word_eval(stack, v);
        break;
        // All others are self-describing
    default:
        break;
    }
}

/* void value_builtin_eval(struct stack *stack, struct value *builtin) { */

/*     char *string = builtin->s; */
    
/*     if (match(string, "+")) { */
/*         builtin_plus(stack); */
/*     } */
/*     else if (match(string, "-")) { */
/*         builtin_minus(stack); */
/*     } */
/*     else if (match(string, "*")) { */
/*         builtin_star(stack); */
/*     } */
/*     else if (match(string, "/")) { */
/*         builtin_slash(stack); */
/*     } */
/*     else if (match(string, "dump")) { */
/*         builtin_dump(stack); */
/*     } */
/*     else if (match(string, "exit")) { */
/*         builtin_exit(stack); */
/*     } */
/*     else if (match(string, "pop")) { */
/*         stack_pop(stack); */
/*     } */
/*     else if (match(string, "dup")) { */
/*         struct value v = stack_pop(stack); */
/*         stack_push(stack, v); */
/*         stack_push(stack, value_copy(&v)); */
/*     } */
/*     else if (match(string, "rep")) { */
/*         struct value n = stack_pop(stack); */
/*         struct value repeat = stack_pop(stack); */

/*         if (n.t != NUM || floor(n.n) != n.n || n.n <= 0) { */
/*             onlog(FATAL, "Can't rep a non-positive integer amount of times", 0); */
/*         } */

/*         stack_push(stack, repeat); */
    
/*         for (int i = 0; i < (int)n.n-1; i++){ */
/*             stack_push(stack, value_copy(&repeat)); */
/*         } */
    
/*     } */
/*     else { */
/*         printf("%s: ", string); */
/*         onlog(FATAL, "Invalid builtin", 0); */
/*     } */

/*     value_free(builtin); */
/* } */

void value_word_eval(struct stack *stack, struct value *v) {
    word *w = &v->w;
    for (int i = w->used-1; i >= 0; i--) {
        // Push stuff backwards so that it looks right
        stack_push(stack, value_copy(&(w->values[i])));
        /*
         * words are created starting when we see "end": we create a word and push
         * everything onto the word stack until "begin" is seen. This means that the top
         * of the stack is the first thing after begin.
         */
    }

    // We don't free after a word eval, because the word is still valid.
}
word word_copy(word *w) {
    word ret = {.capacity = w->capacity, .used = w->used, .values = calloc(w->capacity, sizeof(struct value))};
    checkmem(ret.values);

    memcpy(ret.values, w->values, w->capacity * sizeof(struct value));
    return ret;
}

word init_word() {
    word ret = {.capacity=64, .used=0, .values=calloc(sizeof(struct value), 64)};
    return ret;
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

    if (v1.t != NUM || v2.t != NUM) {
        printf("%d %d", v1.t, v2.t);
        onlog(FATAL, "Attempting to add non-numbers, type is ", 0);
    }
    double result = v1.n + v2.n;

    stack_push(stack, (struct value){.t=NUM,.n=result});
}

void builtin_minus(struct stack *stack) {
    struct value v1, v2;

    v1 = stack_pop(stack);
    v2 = stack_pop(stack);

    v1 = value_expand(&v1);
    v2 = value_expand(&v2);
    if (v1.t != NUM || v2.t != NUM)
        onlog(FATAL, "Attempting to add non-numbers", 0);

    struct value ret =(struct value){.t=NUM,.n=(v1.n-v2.n)};
    value_print(&ret);
    stack_push(stack, ret);
    builtin_dump(stack);
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


void builtin_end(struct stack *stack) {
    //    word newword = init_word();

    onlog(FATAL, "Not yet implemented", 0);
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

void builtin_pop(struct stack *stack) {
    struct value v = stack_pop(stack);
    value_free(&v);
}
bool match(char *string, char *match) {
    while (*string == *match) {
        // Only advance our match if we matched.
        string++;
        match++;
    }
    return (*match == '\0') && (isspace(*string) || *string == '\0');
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

    if (*string == '.') {
        string++;
        double multiplier = 0.1;
        while (isdigit(*(string))) {
            value += ((*(string++) - 48)) * multiplier;
            multiplier /= 10;
        }
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

bool executable(struct value *v) {
    return (v->t == BUILTIN || v->t == WORD);
}

typedef void (*builtinfunc)(struct stack*);


void value_builtin_eval(struct stack *stack, struct value *builtin) {
    builtinfunc b[] = {builtin_plus, builtin_minus, builtin_star, builtin_slash,
                       builtin_dump, builtin_exit};

    b[builtin->b](stack);
}

void collapse(struct stack *stack) {
    struct value v = stack_pop(stack);
    while (executable(&v)) {
        printf("Evaluating an executable with id %d\n", v.b);
        value_eval(stack, &v);
        v = stack_pop(stack);
    }

    if (!executable(&v))
        stack_push(stack, v);
}

char *handle_word_builtin(struct stack *stack, char *string) {
    int jump = 0;
    struct value v = {.t = BUILTIN};
    
    if (match(string, "+")) {
        v.b = PLUS;
        jump = 1;
    }
    else if (match(string, "-")) {
        v.b = MINUS;
        jump = 1;
    }
    else if (match(string, "*")) {
        v.b = STAR;
        jump = 1;
    }
    else if (match(string, "/")) {
        v.b = SLASH;
        jump = 1;
    }
    else if (match(string, "dump")) {
        v.b = DUMP;
        jump = 4;
    }
    else if (match(string, "exit")) {
        v.b = EXIT;
        jump = 4;
    }
    else if (match(string, "pop")) {
        v.b = POP;
        jump = 3;
    }
    else if (match(string, "dup")) {
        v.b = DUP;
        jump = 3;
    }
    else if (match(string, "rep")) {
        v.b = REP;
        jump = 3;
    }
    else if (match(string, "end")) {
        v.b = END;
        jump = 3;
    }
    else if (match(string, "begin")) {
        v.b = BEGIN;
        jump = 5;
    }
    else if (match(string, ".")) {
        collapse(stack);
        jump = 1;
        string += jump;
        return string;
    }
     
    else {
        printf("%s: ", string);
        onlog(FATAL, "Invalid builtin", 0);
    }

    string += jump;

    stack_push(stack, v);
    return string;
}


int main(void) {
    char buf[512] = {};
    struct stack stack = {.capacity = 512, .used=0, .values = malloc(512*sizeof(struct value))};
    while (1) {
        eval(&stack, fgets(buf, 511, stdin));
    }
}
