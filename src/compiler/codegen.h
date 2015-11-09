#ifndef CODEGEN_H
#define CODEGEN_H

#include "symbols.h"
#include "parser.h"

int __flatcc_fb_codegen_common_c(fb_options_t *opts);
#define fb_codegen_common_c __flatcc_fb_codegen_common_c

int __flatcc_fb_codegen_c(fb_options_t *opts, fb_schema_t *S);
#define  fb_codegen_c __flatcc_fb_codegen_c

void *__flatcc_fb_codegen_bfbs_to_buffer(fb_options_t *opts, fb_schema_t *S, void *buffer, size_t *size);
#define fb_codegen_bfbs_to_buffer __flatcc_fb_codegen_bfbs_to_buffer

void *__flatcc_fb_codegen_bfbs_alloc_buffer(fb_options_t *opts, fb_schema_t *S, size_t *size);
#define fb_codegen_bfbs_alloc_buffer __flatcc_fb_codegen_bfbs_alloc_buffer

int __flatcc_fb_codegen_bfbs_to_file(fb_options_t *opts, fb_schema_t *S);
#define fb_codegen_bfbs_to_file __flatcc_fb_codegen_bfbs_to_file

#endif /* CODEGEN_H */
