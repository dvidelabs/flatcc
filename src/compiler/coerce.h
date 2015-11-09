#ifndef COERCE_H
#define COERCE_H

#include <assert.h>

#include "symbols.h"
#include "parser.h"

int __flatcc_fb_coerce_scalar_type(fb_parser_t *P,
        fb_symbol_t *sym, fb_scalar_type_t st, fb_value_t *value);
#define fb_coerce_scalar_type __flatcc_fb_coerce_scalar_type

static inline size_t sizeof_scalar_type(fb_scalar_type_t st)
{
    static const int scalar_type_size[] = {
        0, 8, 4, 2, 1, 1, 8, 4, 2, 1, 8, 4
    };

    return scalar_type_size[st];
}

#endif /* COERCE_H */
