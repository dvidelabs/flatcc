#ifndef HEXDUMP_H
#define HEXDUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Based on: http://stackoverflow.com/a/7776146 */
static void hexdump(char *desc, void *addr, size_t len, FILE *fp) {
    unsigned int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        fprintf(fp, "%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < (unsigned int)len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                fprintf(fp, "  %s\n", buff);

            // Output the offset.
            fprintf(fp, "  %04x ", i);
        } else if ((i % 8) == 0) {
            fprintf(fp, " ");
        }

        // Now the hex code for the specific character.
        fprintf(fp, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        fprintf(fp, "   ");
        i++;
    }

    // And print the final ASCII bit.
    fprintf(fp, "  %s\n", buff);
}

#ifdef __cplusplus
}
#endif

#endif /* HEXDUMP_H */
