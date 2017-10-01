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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

unsigned int logset=0;

enum loglevel {
    FATAL,
    STANDARD,
    VERBOSE,
    DEBUG
};

#define checkmem(value) do {if(!value){ onlog(FATAL, "Failed to allocate memory in ", __LINE__);}}while(0)

void onlog(enum loglevel level, char *string, int line) {
    if (level > logset) {
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
enum type {
    NUM,
    STRING,
    BUILTIN,
    WORD,
    
};

struct value {
    enum type t;
    union {
        double n;
        char *s;
        char *word;
    };
};

    

struct stack {
    size_t capacity;
    size_t used;
    struct value *values;
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
    REPTOK
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
}

void words_put(char *name, word word) {
    int offset = words_hash(name, entrycount);

    struct wordentry *cur = words.entries[offset];
    struct wordentry *trailer;

    // Make sure that if word is already defined, overwrite it
    for (;cur; cur = cur->next) {
        if (!strcmp(cur->name, name)) {
            word_free(cur->word);
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
    
}
#undef entrycount
