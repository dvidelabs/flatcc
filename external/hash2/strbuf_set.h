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
 * Stores a pointer to a strbuf that contains the start and length of
 * a memory buffer, such a parsed string symbol. Similar to token_set
 * except a strbuf can be used to dynamically allocate memory. Once
 * hashed, a strbuf should be considered a constant. There is no benefit
 * in using a strbuf over a token for hash indexing purposes, but the
 * stored data might prefer the strbuf form, for example because the
 * memory content may have to be modified before being index, for
 * example by unescaping text within a larger block of text.
 *
 * The strbuf typically reside in some other structure that can be
 * obtained via container_of or similar pointer manipulation.
 *
 * The key data type is a token a direct memory reference because
 * a token is easily constructed from other buffer types.
 */

#ifndef STRUBF_SET_H
#define STRUBF_SET_H

#include <dvstd/hash/hashtable.h>
#include <dvstd/token.h>
#include <dvstd/strbuf.h>

declare_hashtable(strbuf_set, struct token, strbuf_t *)

#endif /* STRBUF_SET_H */
