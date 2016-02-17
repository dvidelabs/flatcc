#include "codegen_c.h"
#include "flatcc/flatcc_types.h"

/* -DFLATCC_PORTABLE may help if inttypes.h is missing. */
#ifndef PRId64
#include <inttypes.h>
#endif

static int gen_json_printer_pretext(output_t *out)
{
    fprintf(out->fp,
        "#ifndef %s_JSON_PRINTER_H\n"
        "#define %s_JSON_PRINTER_H\n",
        out->S->basenameup, out->S->basenameup);

    fprintf(out->fp, "\n/* " FLATCC_GENERATED_BY " */\n\n");
    fprintf(out->fp, "#include \"flatcc/flatcc_json_printer.h\"\n");
    fb_gen_c_includes(out, "_json_printer.h", "_JSON_PRINTER_H");
    gen_pragma_push(out);
    fprintf(out->fp, "\n");
    return 0;
}

static int gen_json_printer_footer(output_t *out)
{
    gen_pragma_pop(out);
    fprintf(out->fp,
        "#endif /* %s_JSON_PRINTER_H */\n",
        out->S->basenameup);
    return 0;
}

static int gen_json_printer_enum(output_t *out, fb_compound_type_t *ct)
{
    fb_symbol_t *sym;
    fb_member_t *member;
    fb_scoped_name_t snt, snref;
    const char *tp, *tn, *ns;
    int bit_flags;
    uint64_t mask;
    char *suffix;
    char *ut;

    fb_clear(snt);
    fb_clear(snref);
    fb_compound_name(ct, &snt);
    tp = scalar_type_prefix(ct->type.st);
    tn = scalar_type_name(ct->type.st);
    ns = scalar_type_ns(ct->type.st, out->nsc);

    bit_flags = !!(ct->metadata_flags & fb_f_bit_flags);
    if (bit_flags) {
        switch (ct->size) {
        case 1:
            mask = 0xff, suffix = "U", ut = "uint8_t";
            break;
        case 2:
            mask = 0xffff, suffix = "U", ut = "uint16_t";
            break;
        case 4:
            mask = 0xffffffffL, suffix = "UL", ut = "uint32_t";
            break;
        default:
            mask = 0xffffffffffffffffULL, suffix = "ULL", ut = "uint64_t";
            break;
        }
        for (sym = ct->members; sym; sym = sym->link) {
            member = (fb_member_t *)sym;
            switch (member->value.type) {
            case vt_uint:
                mask &= ~(uint64_t)member->value.u;
                break;
            case vt_int:
                mask &= ~(uint64_t)member->value.i;
                break;
            case vt_bool:
                mask &= ~(uint64_t)member->value.b;
                break;
            }
        }
    }

    /*
     * We pass a pointer to provide a generic yet efficient interface.
     * td, id would not support vectors or structs of enums. Passing the
     * value would have multiple types and lead to explotion that can
     * easily be avoided by passing a pointer.
     */
    fprintf(out->fp,
            "void __%s_print_json_enum(flatcc_json_printer_t *ctx, %s%s v)\n{\n",
            snt.text, ns, tn);
    if (bit_flags) {
        if (strcmp(ut, tn)) {
            fprintf(out->fp, "    %s x = (%s)v;\n", ut, ut);
        } else {
            fprintf(out->fp, "    %s x = v;\n", ut);
        }
        fprintf(out->fp,
                "    int multiple = 0 != (x & (x - 1));\n"
                "    int i = 0;\n");

        fprintf(out->fp, "\n");
        /*
         * If the value is not entirely within the known bit flags, print as
         * a number.
         */
        if (mask) {
            fprintf(out->fp,
                    "    if ((x & 0x%"PRIx64") || x == 0) {\n"
                    "        flatcc_json_printer_%s(ctx, v);\n"
                    "        return;\n"
                    "    }\n",
                   mask, tp);
        }
        /*
         * Test if multiple bits set. We may have a configuration option
         * that requires multiple flags to be quoted like `color: "Red Green"`
         * but unquoted if just a single value like `color: Green`.
         *
         * The index `i` is used to add space separators much like an
         * index is provided for struct members to handle comma.
         */
        fprintf(out->fp, "    flatcc_json_printer_delimit_enum_flags(ctx, multiple);\n");
        for (sym = ct->members; sym; sym = sym->link) {
            member = (fb_member_t *)sym;
            switch (member->value.type) {
            case vt_uint:
                fprintf(out->fp, "    if (x & 0x%"PRIx64"%s) flatcc_json_printer_enum_flag(ctx, i++, \"%.*s\", %ld);\n",
                        member->value.u, suffix, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            case vt_int:
                fprintf(out->fp, "    if (x & 0x%"PRIx64"%s) flatcc_json_printer_enum_flag(ctx, i++, \"%.*s\", %ld);\n",
                        (uint64_t)member->value.i, suffix, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            case vt_bool:
                fprintf(out->fp, "    if (x & 0x%"PRIx64"%s) flatcc_json_printer_enum_flag(ctx, i++, \"%.*s\", %ld);\n",
                        (uint64_t)member->value.b, suffix, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            default:
                gen_panic(out, "internal error: unexpected value type for enum json_print");
                break;
            }
        }
        fprintf(out->fp, "    flatcc_json_printer_delimit_enum_flags(ctx, multiple);\n");
    } else {
        fprintf(out->fp, "\n    switch (v) {\n");
        for (sym = ct->members; sym; sym = sym->link) {
            member = (fb_member_t *)sym;
            switch (member->value.type) {
            case vt_uint:
                fprintf(out->fp, "    case %"PRIu64": flatcc_json_printer_enum(ctx, \"%.*s\", %ld); break;\n",
                        member->value.u, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            case vt_int:
                fprintf(out->fp, "    case %"PRId64": flatcc_json_printer_enum(ctx, \"%.*s\", %ld); break;\n",
                        member->value.i, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            case vt_bool:
                fprintf(out->fp, "    case %u: flatcc_json_printer_enum(ctx, \"%.*s\", %ld); break;\n",
                        member->value.b, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            default:
                gen_panic(out, "internal error: unexpected value type for enum json_print");
                break;
            }
        }
        fprintf(out->fp,
                "    default: flatcc_json_printer_%s(ctx, v); break;\n"
                "    }\n",
                tp);
    }
    fprintf(out->fp, "}\n\n");
    return 0;
}

static int gen_json_printer_union(output_t *out, fb_compound_type_t *ct)
{
    fb_symbol_t *sym;
    fb_member_t *member;
    fb_scoped_name_t snt, snref;

    fb_clear(snt);
    fb_clear(snref);
    fb_compound_name(ct, &snt);

    fprintf(out->fp,
            "void __%s_print_json_union(flatcc_json_printer_t *ctx, flatcc_json_printer_table_descriptor_t *td, int id, const char *name, int len)\n"
            "{\n    switch (flatcc_json_printer_read_union_type(td, id)) {\n",
            snt.text);
    for (sym = ct->members; sym; sym = sym->link) {
        member = (fb_member_t *)sym;
        if (member->type.type == vt_missing) {
            /* NONE is of type vt_missing and already handled. */
            continue;
        }
        assert(member->type.type == vt_compound_type_ref);
        fb_compound_name(member->type.ct, &snref);
#if FLATCC_JSON_PRINT_MAP_ENUMS
        fprintf(out->fp,
                "    case %u:\n"
                "        flatcc_json_printer_union_type(ctx, td, name, len, %u, \"%.*s\", %ld);\n",
                (unsigned)member->value.u, (unsigned)member->value.u, (int)sym->ident->len, sym->ident->text, sym->ident->len);
#else
        fprintf(out->fp,
                "    case %u:\n"
                "        flatcc_json_printer_union_type(ctx, td, name, len, %u, 0, 0);\n",
                (unsigned)member->value.u, (unsigned)member->value.u);
#endif
        fprintf(out->fp,
                "        flatcc_json_printer_table_field(ctx, td, id, name, len, __%s_print_json_table);\n"
                "        break;\n",
                snref.text);
    }
    fprintf(out->fp,
            "    }\n}\n\n");
    return 0;
}

static int gen_json_printer_struct(output_t *out, fb_compound_type_t *ct)
{
    fb_symbol_t *sym;
    fb_member_t *member;
    fb_scoped_name_t snt, snref;
    int index = 0;
    const char *tp;

    fb_clear(snt);
    fb_clear(snref);
    fb_compound_name(ct, &snt);

    fprintf(out->fp,
            "static void __%s_print_json_struct(flatcc_json_printer_t *ctx, const void *p)\n"
            "{\n",
            snt.text);
    for (sym = ct->members; sym; ++index, sym = sym->link) {
        member = (fb_member_t *)sym;
        if (member->metadata_flags & fb_f_deprecated) {
            continue;
        }
        switch (member->type.type) {
        case vt_scalar_type:
            tp = scalar_type_prefix(member->type.st);
            fprintf(
                    out->fp,
                    "    flatcc_json_printer_%s_struct_field(ctx, %d, p, %zu, \"%.*s\", %ld);\n",
                    tp, index, (size_t)member->offset, (int)sym->ident->len, sym->ident->text, sym->ident->len);
            break;
        case vt_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_enum:
#if FLATCC_JSON_PRINT_MAP_ENUMS
                tp = scalar_type_prefix(member->type.ct->type.st);
                fprintf(out->fp,
                        "    flatcc_json_printer_%s_enum_struct_field(ctx, %d, p, %zu, \"%.*s\", %ld, &__%s_print_json_enum);\n",
                        tp, index, (size_t)member->offset, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                break;
#else
                tp = scalar_type_prefix(member->type.ct->type.st);
                fprintf(
                        out->fp,
                        "    flatcc_json_printer_%s_struct_field(ctx, %d, p, %zu, \"%.*s\", %ld);\n",
                        tp, index, (size_t)member->offset, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
#endif
            case fb_is_struct:
                fprintf(out->fp,
                        "    flatcc_json_printer_embedded_struct_field(ctx, %d, p, %zu, \"%.*s\", %ld, &__%s_print_json_struct);\n",
                        index, (size_t)member->offset, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                break;
            }
        }
    }
    fprintf(out->fp, "}\n\n");
    fprintf(out->fp,
            "static inline int %s_print_json_as_root(flatcc_json_printer_t *ctx, const void *buf, size_t bufsiz, const char *fid)\n"
            "{\n    return flatcc_json_printer_struct_as_root(ctx, buf, bufsiz, fid, &__%s_print_json_struct);\n}\n\n",
            snt.text, snt.text);
    return 0;
}

static int gen_json_printer_table(output_t *out, fb_compound_type_t *ct)
{
    fb_symbol_t *sym;
    fb_member_t *member;
    fb_scoped_name_t snt, snref;
    const char *tp;
    fb_member_t **map;
    uint64_t i;
    int ret = 0;

    fb_clear(snt);
    fb_clear(snref);
    fb_compound_name(ct, &snt);

    /*
     * The compound type can be iterated by the textual order of the
     * parsed schema, or in a sorted order which is either
     * "original_order", i.e. the textual order, or ordered by
     * alignment usinger ct->ordered_members.
     * The textual order makes most sense, but to provide a unique
     * ordering we sort by id which is not provided by the compound
     * type directly, but easy to obtain.
     *
     * NOTE: due to unions, not all entries will be non-zero because the
     * type field is not stored as a member so calloc is important.
     */
    map = calloc(ct->count, sizeof(*map));
    if (!map) {
        gen_panic(out, "internal error: memory allocation failure");
        return -1;
    }
    for (sym = ct->members; sym; sym = sym->link) {
        member = (fb_member_t *)sym;
        map[member->id] = member;
    }
    fprintf(out->fp,
            "static void __%s_print_json_table(flatcc_json_printer_t *ctx, flatcc_json_printer_table_descriptor_t *td)\n"
            "{",
            snt.text);

    for (i = 0; i < ct->count; ++i) {
        member = map[i];
        if (!member) {
            /* A union type. */
            continue;
        }
        sym = &member->symbol;
        if (member->metadata_flags & fb_f_deprecated) {
            continue;
        }
        fprintf(out->fp, "\n    ");
        switch (member->type.type) {
        case vt_scalar_type:
            tp = scalar_type_prefix(member->type.st);

            switch(member->value.type) {
            case vt_bool:
            case vt_uint:
                fprintf( out->fp,
                    "flatcc_json_printer_%s_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRIu64");",
                    tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.u);
                break;
            case vt_int:
                fprintf( out->fp,
                    "flatcc_json_printer_%s_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRId64");",
                    tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.i);
                break;
            case vt_float:
                fprintf( out->fp,
                    "flatcc_json_printer_%s_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %lf);",
                    tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.f);
                break;
            default:
                gen_panic(out, "internal error: unexpected default value type\n");
                return -1;
            }
            break;
        case vt_vector_type:
            if (member->nest) {
                fb_compound_name((fb_compound_type_t *)&member->nest->symbol, &snref);
                if (member->nest->symbol.kind == fb_is_table) {
                    /*
                     * Always set fid to 0 since it is difficult to know what is right.
                     * We do know the type from the field attribute.
                     */
                    fprintf(out->fp,
                        "flatcc_json_printer_table_as_nested_root(ctx, td, %"PRIu64", \"%.*s\", %ld, 0, __%s_print_json_table);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                } else {
                    /*
                     * Always set fid to 0 since it is difficult to know what is right.
                     * We do know the type from the field attribute.
                     */
                    fprintf(out->fp,
                        "flatcc_json_printer_struct_as_nested_root(ctx, td, %"PRIu64", \"%.*s\", %ld, 0, __%s_print_json_struct);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                }
            } else {
                tp = scalar_type_prefix(member->type.st);
                fprintf(out->fp,
                        "flatcc_json_printer_%s_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld);",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len);
            }
            break;
        case vt_string_type:
            fprintf(out->fp,
                    "flatcc_json_printer_string_field(ctx, td, %"PRIu64", \"%.*s\", %ld);",
                    member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len);
            break;
        case vt_vector_string_type:
            fprintf(out->fp,
                    "flatcc_json_printer_string_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld);",
                    member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len);
            break;
        case vt_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_enum:
                tp = scalar_type_prefix(member->type.ct->type.st);
                switch(member->value.type) {
                case vt_bool:
#if FLATCC_JSON_PRINT_MAP_ENUMS
                case vt_uint:
                    fprintf( out->fp,
                        "flatcc_json_printer_%s_enum_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRIu64", &__%s_print_json_enum);",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.u, snref.text);
                    break;
                case vt_int:
                    fprintf( out->fp,
                        "flatcc_json_printer_%s_enum_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRId64", &__%s_print_json_enum);",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.i, snref.text);
                    break;
#else
                case vt_uint:
                    fprintf( out->fp,
                        "flatcc_json_printer_%s_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRIu64");",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.u);
                    break;
                case vt_int:
                    fprintf( out->fp,
                        "flatcc_json_printer_%s_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %"PRId64");",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, member->value.i);
                    break;
#endif
                default:
                    gen_panic(out, "internal error: unexpected default value type for enum\n");
                    return -1;
                }
                break;
            case fb_is_struct:
                fprintf(out->fp,
                        "flatcc_json_printer_struct_field(ctx, td, %"PRIu64", \"%.*s\", %ld, &__%s_print_json_struct);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                break;
            case fb_is_table:
                fprintf(out->fp,
                        "flatcc_json_printer_table_field(ctx, td, %"PRIu64", \"%.*s\", %ld, &__%s_print_json_table);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                break;
            case fb_is_union:
                fprintf(out->fp,
                        "__%s_print_json_union(ctx, td, %"PRIu64", \"%.*s\", %ld);",
                        snref.text, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
            default:
                gen_panic(out, "internal error: unexpected compound type for table json_print");
                goto fail;
            }
            break;
        case vt_vector_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_table:
                fprintf(out->fp,
                        "flatcc_json_printer_table_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld, &__%s_print_json_table);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, snref.text);
                break;
            case fb_is_enum:
#if FLATCC_JSON_PRINT_MAP_ENUMS
                tp = scalar_type_prefix(member->type.st);
                fprintf(out->fp,
                        "flatcc_json_printer_%s_enum_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %zu, &__%s_print_json_enum);",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, (size_t)ct->size, snref.text);
                break;
#else
                tp = scalar_type_prefix(member->type.st);
                fprintf(out->fp,
                        "flatcc_json_printer_%s_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld);",
                        tp, member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len);
                break;
#endif
            case fb_is_struct:
                fprintf(out->fp,
                        "flatcc_json_printer_struct_vector_field(ctx, td, %"PRIu64", \"%.*s\", %ld, %zu, &__%s_print_json_struct);",
                        member->id, (int)sym->ident->len, sym->ident->text, sym->ident->len, (size_t)member->size, snref.text);
                break;
            default:
                gen_panic(out, "internal error: unexpected vector compound type for table json_print");
                goto fail;
            }
            break;
        }
    }
    fprintf(out->fp, "\n}\n\n");
    fprintf(out->fp,
            "static inline int %s_print_json_as_root(flatcc_json_printer_t *ctx, const void *buf, size_t bufsiz, const char *fid)\n"
            "{\n    return flatcc_json_printer_table_as_root(ctx, buf, bufsiz, fid, &__%s_print_json_table);\n}\n\n",
            snt.text, snt.text);
done:
    if (map) {
        free(map);
    }
    return ret;
fail:
    ret = -1;
    goto done;
}

/*
 * Only tables are mutually recursive. Structs are sorted and unions are
 * defined earlier, depending on the table prototypes.
 */
static int gen_json_printer_prototypes(output_t *out)
{
    fb_symbol_t *sym;
    fb_scoped_name_t snt;
    fb_symbol_t *root_type = out->S->root_type.type;

    fb_clear(snt);

    if (root_type)
    switch (root_type->kind) {
    case fb_is_table:
    case fb_is_struct:
        fprintf(out->fp,
                "/*\n"
                " * Prints the default root table or struct from a buffer which must have\n"
                " * the schema declared file identifier, if any. It is also possible to\n"
                " * call the type specific `print_json_as_root` function wich accepts an\n"
                " * optional identifier (or 0) as argument. The printer `ctx` object must\n"
                " * be initialized with the appropriate output type, or it can be 0 which\n"
                " * defaults to stdout. NOTE: `ctx` is not generally allowed to be null, only\n"
                " * here for a simplified interface.\n"
                " */\n");
    fprintf(out->fp,
            "static int %s_print_json(flatcc_json_printer_t *ctx, const char *buf, size_t bufsiz);\n\n",
            out->S->basename);
    default:
        break;
    }

    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_table:
            fb_compound_name((fb_compound_type_t *)sym, &snt);
            fprintf(out->fp,
                    "static void __%s_print_json_table(flatcc_json_printer_t *ctx, flatcc_json_printer_table_descriptor_t *td);\n",
                    snt.text);
        }
    }
    fprintf(out->fp, "\n");
    return 0;
}

static int gen_json_printer_enums(output_t *out)
{
    fb_symbol_t *sym;

    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_enum:
            gen_json_printer_enum(out, (fb_compound_type_t *)sym);
        }
    }
    return 0;
}

static int gen_json_printer_unions(output_t *out)
{
    fb_symbol_t *sym;

    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_union:
            gen_json_printer_union(out, (fb_compound_type_t *)sym);
        }
    }
    return 0;
}

static int gen_json_printer_structs(output_t *out)
{
    fb_symbol_t *sym;

    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_struct:
            gen_json_printer_struct(out, (fb_compound_type_t *)sym);
        }
    }
    return 0;
}

static int gen_json_printer_tables(output_t *out)
{
    fb_symbol_t *sym;

    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_table:
            gen_json_printer_table(out, (fb_compound_type_t *)sym);
        }
    }
    return 0;
}

/* Same for structs and tables. */
static int gen_root_type_printer(output_t *out, fb_compound_type_t *ct)
{
    fb_scoped_name_t snt;

    fb_clear(snt);
    fb_compound_name(ct, &snt);

    fprintf(out->fp,
            "static int %s_print_json(flatcc_json_printer_t *ctx, const char *buf, size_t bufsiz)\n",
            out->S->basename);
    fprintf(out->fp,
            "{\n"
            "    flatcc_json_printer_t printer;\n"
            "\n"
            "    if (ctx == 0) {\n"
            "        ctx = &printer;\n"
            "        flatcc_json_printer_init(ctx, 0);\n"
            "    }\n"
            "    return %s_print_json_as_root(ctx, buf, bufsiz, ",
            snt.text);
    if (out->S->file_identifier.type == vt_string) {
        fprintf(out->fp,
                "\"%.*s\");\n",
                out->S->file_identifier.s.len, out->S->file_identifier.s.s);
    } else {
        fprintf(out->fp,
                "0);");
    }
    fprintf(out->fp,
        "}\n\n");
    return 0;
}

static int gen_json_root_printer(output_t *out)
{
    fb_symbol_t *root_type = out->S->root_type.type;

    if (!root_type) {
        return 0;
    }
    if (root_type) {
        switch (root_type->kind) {
        case fb_is_table:
        case fb_is_struct:
            return gen_root_type_printer(out, (fb_compound_type_t *)root_type);
        default:
            break;
        }
    }
    return 0;
}

int fb_gen_c_json_printer(output_t *out)
{
    gen_json_printer_pretext(out);
    gen_json_printer_prototypes(out);
    gen_json_printer_enums(out);
    gen_json_printer_unions(out);
    gen_json_printer_structs(out);
    gen_json_printer_tables(out);
    gen_json_root_printer(out);
    gen_json_printer_footer(out);
    return 0;
}
