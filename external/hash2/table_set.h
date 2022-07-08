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
 * Stores a pointer to a table_t struct that holds its own allocated but
 * non-mutating memory and uses the table content as key. The table
 * control struct is typically placed in another struct that can be
 * found using container_of, but that is outside the scope of the hash
 * index. Mutating any table would invalidate the hash table index but
 * the hash table can then still be used to iterate over the content or
 * to destroy it.
 *
 * Tables with different element sizes can be mixed and will be considered
 * different even if the content is the same (e.g. all zero).
 */

#ifndef TABLE_SET_H
#define TABLE_SET_H

#include <dvstd/hash/hashtable.h>
#include <dvstd/table.h>

declare_hashtable(table_set, table_t *, table_t *)

#endif /* TABLE_SET_H */
