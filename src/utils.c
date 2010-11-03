/*
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Paulo Zanoni <pzanoni@mandriva.com>
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

extern int verbose;

void print_log(char *string, ...)
{
    va_list args;

    if (!verbose)
        return;

    va_start(args, string);
    vprintf(string, args);
    va_end(args);
}

/* Write the substring of "origin" indicated by "offset_begin" and "offset_end"
 * into the "dest" string, also adding a '\0'. There's no size checking. */
void substrcpy(char *dest, char *origin, int offset_begin, int offset_end)
{
    strncpy(dest, &origin[offset_begin], offset_end - offset_begin);
    dest[offset_end - offset_begin] = '\0';
}
