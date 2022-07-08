/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Mikkel F. Jørgensen, dvide.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Example of how to hash non-trivial data.
 *
 * It has a practical use with a parser the fills token structs as the
 * lexer digests them. The token string pointer can point directly into
 * the source buffer. If the token is a symbol, the parser can then
 * insert a pointer to token struct into the token map and later add type
 * and related information.
 *
 * Note that the hash table itself only stores a pointer so it is
 * reasonable to have a low load factor. The hashed string is never
 * allocated.
 */

#ifndef TOKEN_MAP_H
#define TOKEN_MAP_H

#include <dvstd/hash/hashtable.h>
#include <dvstd/token.h>

struct token_data {
    struct token token;
    int type;
    void *data;
};

/*
 * The hash table only finds and deletes by key type so it is useful to
 * have some conversion functions from ordinary strings to the key type.
 *
 * The function `struct token token_map_key(struct token *)` will be
 * defined by the interface, but here additional conversions are
 * provided that accept strings with and without length.
 *
 * If the key type had been `const char *` it would not be possible to
 * search for unterminated strings of a given length and such searches
 * are useful in a parsed text stream to avoid copying strings.
 */

/* Application helper not defined in hash table interface. */
static inline struct token token_map_key_from_strn(const char *s, size_t len)
{
    struct token tk;

    token_set_strn(&tk, s, len);
    return tk;
}

/* Application helper not defined in hash table interface. */
static inline struct token token_map_key_from_str(const char *s)
{
    struct token tk;

    token_set_str(&tk, s);
    return tk;
}

declare_hashtable(token_map, struct token, struct token_data *)

#endif /* TOKEN_MAP_H */
