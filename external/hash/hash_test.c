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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "str_set.h"
#include "token_map.h"

#include "ht_trace.h"

str_set_t S;
token_map_t TM;

char *keys[] = {
    "foo",
    "bar",
    "baz",
    "gimli",
    "bofur"
};

struct token tokens[5];

void free_key(void *context, char *key) {
    free(key);
}

void test_str_set()
{
    int i;
    char *s, *s0, *s1;
    unsigned int n = sizeof(keys)/sizeof(keys[0]);

    /* We rely on zero initialization here. */
    assert(str_set_count(&S) == 0);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        /* We don't have to use strdup, but we test the
         * allocation management and item replacement. */
        s = str_set_insert(&S, s, strlen(s), strdup(s), ht_keep);
        assert(str_set_count(&S) == i + 1);
        assert(s == 0);
    }
    assert(n == 5);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s = str_set_find(&S, s, strlen(s));
        assert(strcmp(s, keys[i]) == 0);
    }
    s = str_set_remove(&S, "gimlibofur", 5);
    assert(strcmp(s, "gimli") == 0);
    free(s);
    assert(str_set_count(&S) == n - 1);
    s = str_set_remove(&S, "gimlibofur", 5);
    assert(s == 0);
    assert(str_set_count(&S) == n - 1);
    s = str_set_insert(&S, "foobarbaz", 6,
            (s0 = strndup("foobarbaz", 6)), ht_keep);
    assert(s == 0);
    assert(str_set_count(&S) == n);
    s = str_set_insert(&S, "foobarbaz", 6,
            (s1 = strndup("foobarbaz", 6)), ht_keep);
    assert(s == s0);
    free(s1);
    assert(str_set_count(&S) == n);
    s = str_set_find(&S, "foobar", 6);
    assert(s == s0);
    s = str_set_insert(&S, "foobarbaz", 6,
            (s1 = strndup("foobarbaz", 6)), ht_replace);
    assert(s == s0);
    free(s);
    s = str_set_find(&S, "foobar", 6);
    assert(s == s1);
    s = str_set_find(&S, "foobarbaz", 9);
    assert(s == 0);
    str_set_destroy(&S, free_key, 0);
    s = str_set_find(&S, "foobar", 6);
    assert(s == 0);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s = str_set_find(&S, s, strlen(s));
        assert(s == 0);
    }
}

void test_str_set2()
{
    int i;
    char *s, *s1;
    unsigned int n = sizeof(keys)/sizeof(keys[0]);

    for (i = 0; i < n; ++i) {
        s = keys[i];
        str_set_insert(&S, s, strlen(s), s, ht_unique);
    }
    assert(str_set_count(&S) == n);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        /*
         * Unique and multi are the same logically, but different
         * intentionally.
         */
        str_set_insert(&S, s, strlen(s), s, ht_multi);
    }
    assert(str_set_count(&S) == 2 * n);
    ht_trace_buckets(&S, "after double insert", 0, 8);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s1 = str_set_find(&S, s, strlen(s));
        assert(strcmp(s, s1) == 0);
    }
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s1 = str_set_remove(&S, s, strlen(s));
        assert(strcmp(s, s1) == 0);
        assert(str_set_count(&S) == 2 * n - i - 1);
        ht_trace_buckets(&S, "after single", 8, 8);
    }
    ht_trace_buckets(&S, "after first remove", 0, 8);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s1 = str_set_remove(&S, s, strlen(s));
        assert(strcmp(s, s1) == 0);
        assert(str_set_count(&S) == n - i - 1);
    }
    ht_trace_buckets(&S, "efter second remove", 0, 8);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s1 = str_set_remove(&S, s, strlen(s));
        assert(s1 == 0);
        assert(str_set_count(&S) == 0);
    }
    str_set_clear(&S);
}

void test_str_set3()
{
    int i;
    char *s, *s1;
    unsigned int n = sizeof(keys)/sizeof(keys[0]);

    for (i = 0; i < n; ++i) {
        s = keys[i];
        str_set_insert_item(&S, s, ht_unique);
    }
    assert(str_set_count(&S) == n);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        str_set_insert_item(&S, s, ht_keep);
    }
    assert(str_set_count(&S) == n);
    for (i = 0; i < n; ++i) {
        s = keys[i];
        s1 = str_set_find_item(&S, s);
        assert(strcmp(s, s1) == 0);
    }
    s = keys[1];
    s1 = str_set_remove_item(&S, s);
    /*
     * This doesn't always hold, but here we
     * are sure because of how we inserted data.
     */
    assert(s == s1);
    s1 = str_set_find_item(&S, s);
    assert(s1 == 0);
    str_set_clear(&S);
}

void test_str_set4()
{
    char *s, *s1;

    s = "dumble";
    str_set_insert_item(&S, "dumble", ht_keep);
    s1 = str_set_find_item(&S, s);
    /* TMnsert without replace. */
    str_set_insert_item(&S, "2dumble" + 1, ht_keep);
    assert(s == s1);
    s1 = str_set_find_item(&S, s);
    assert(s == s1);
    /* TMnsert with replace. */
    s1 = str_set_insert_item(&S, "2dumble" + 1, ht_replace);
    /* Old value still returned. */
    assert(s == s1);
    s1 = str_set_find_item(&S, s);
    assert(s != s1);
    /* New item returned. */
    assert(strcmp(s1 - 1, "2dumble") == 0);
    str_set_clear(&S);
}

void visit_item_set(void *context, token_map_item_t item)
{
    int *count = context;
    ++*count;
}

void test_token_map()
{
    int i, count;
    token_map_item_t item;
    unsigned int n = sizeof(keys)/sizeof(keys[0]);

    assert(sizeof(tokens)/sizeof(item[0]) == n);

    for (i = 0; i < n; ++i) {
        tokens[i].token = keys[i];
        tokens[i].len = strlen(keys[i]);
    }
    for (i = 0; i < n; ++i) {
        item = &tokens[i];
        token_map_insert(&TM, item->token, item->len, item, ht_unique);
    }
    count = 0;
    token_map_visit(&TM, visit_item_set, &count);
    assert(count == n);

    for (i = 0; i < n; ++i) {
        item = token_map_find(&TM, keys[i], strlen(keys[i]));
        assert(item->type == 0);
        item->type = 1;
    }
    for (i = 0; i < n; ++i) {
        item = token_map_find_item(&TM, &tokens[i]);
        assert(item->type == 1);
        item->type = 2;
    }
}

int main(int argc, char *argv[])
{
    test_str_set();
    test_str_set2();
    test_str_set3();
    test_str_set4();
    test_token_map();

    printf("all tests passed\n");

    return 0;
}
