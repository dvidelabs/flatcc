#ifndef HT_TRACE_H
#define HT_TRACE_H

#ifdef HT_TRACE_ON
#ifndef HT_TRACE_OUT
#define HT_TRACE_OUT stderr
#endif

#include <stdio.h>
#define ht_trace(s) fprintf(HT_TRACE_OUT, "trace: %s\n", s)
#define ht_tracei(s, i) fprintf(HT_TRACE_OUT, "trace: %s: %d\n", s, (int)i)
#define ht_tracex(s, x) fprintf(HT_TRACE_OUT, "trace: %s: 0x%lx\n", s, (long)x)
#define ht_traces(s, s2, len) fprintf(HT_TRACE_OUT, "trace: %s: %.*s\n", s, (int)len, s2)

#ifdef HT_SRH
static inline size_t __ht_unfold_hash(size_t fhash, int M);
static void ht_trace_buckets(hash_table_t *ht, char *msg, int first, int count)
{
    int i, j, N, n;
    ht_entry_t *T;
    size_t buckets = 1 << ht->buckets;
    size_t hash;

    n = buckets;
    N = n - 1;

    if (count == 0) {
        count = 32;
    }
    if (count > n) {
        count = n;
    }

    first = first & N;
    fprintf(HT_TRACE_OUT, "bucket trace: %s\n", msg);
    if (n > count) {
        n = count;
    }
    fprintf(HT_TRACE_OUT, "item count: %ld, bucket count %ld, utilization: %0.1f%%\n",
            ht->count, buckets, (double)ht->count / buckets * 100);

    fprintf(HT_TRACE_OUT, "bck item             fhash            (hash)            /key\n");
    T = ht->table;
    for (i = 0; i < n; ++i) {
        j = (first + i) & N;
        hash = __ht_unfold_hash(T[j].hash, ht->buckets);
        if (!(T[j].hash + 1)) {
            fprintf(HT_TRACE_OUT, "%03d: <empty>\n", j);
        } else {
            fprintf(HT_TRACE_OUT, "%03d:%016lx:%016lx:(%016lx)/%.*s...\n", j, (size_t)T[j].item, (size_t)T[j].hash, hash,
#ifndef HT_DISABLE_TRACE_KEYS
                    (int)ht_key_len(T[j].item), ht_key(T[j].item));
#else
            /* Not all keys are printable. */
                    3, "n/a");
#endif
        }
    }
    fprintf(HT_TRACE_OUT, "--\n");
}
#else
static void ht_trace_buckets(hash_table_t *ht, char *msg, int first, int count)
{
    int i, j, N, n;

    n = ht->buckets;
    N = n - 1;

    if (count == 0) {
        count = 32;
    }
    if (count > n) {
        count = n;
    }

    first = first & N;
    fprintf(HT_TRACE_OUT, "bucket trace: %s\n", msg);
    if (n > count) {
        n = count;
    }
    fprintf(HT_TRACE_OUT, "item count: %ld, bucket count %ld, utilization: %0.1f%%\n",
            ht->count, ht->buckets, (double)ht->count / ht->buckets * 100);

    if (ht->offsets) {
        for (i = 0; i < n; ++i) {
            j = (first + i) & N;
            fprintf(HT_TRACE_OUT, "%03d:%08x:[%02d]\n",
                    j, (unsigned int)((void **)ht->table)[j], (unsigned int)ht->offsets[j]);
        }
    } else {
        for (i = 0; i < n; ++i) {
            j = (first + i) & N;
            fprintf(HT_TRACE_OUT, "%03d:%08x\n", j, (unsigned int)((void **)ht->table)[j]);
        }
    }
    fprintf(HT_TRACE_OUT, "--\n");
}
#endif
#else
#define ht_trace(arg1) ((void)0)
#define ht_tracei(arg1, arg2) ((void)0)
#define ht_tracex(arg1, arg2) ((void)0)
#define ht_traces(arg1, arg2, arg3) ((void)0)
#define ht_trace_buckets(arg1, arg2, arg3, arg4) ((void)0)
#endif

#endif /* HT_TRACE_H */
