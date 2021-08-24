// +------------------------------------------------------------------+
// |             ____ _               _        __  __ _  __           |
// |            / ___| |__   ___  ___| | __   |  \/  | |/ /           |
// |           | |   | '_ \ / _ \/ __| |/ /   | |\/| | ' /            |
// |           | |___| | | |  __/ (__|   <    | |  | | . \            |
// |            \____|_| |_|\___|\___|_|\_\___|_|  |_|_|\_\           |
// |                                                                  |
// | Copyright Mathias Kettner 2014             mk@mathias-kettner.de |
// +------------------------------------------------------------------+
//
// This file is part of Check_MK.
// The official homepage is at http://mathias-kettner.de/check_mk.
//
// check_mk is free software;  you can redistribute it and/or modify it
// under the  terms of the  GNU General Public License  as published by
// the Free Software Foundation in version 2.  check_mk is  distributed
// in the hope that it will be useful, but WITHOUT ANY WARRANTY;  with-
// out even the implied warranty of  MERCHANTABILITY  or  FITNESS FOR A
// PARTICULAR PURPOSE. See the  GNU General Public License for more de-
// tails. You should have  received  a copy of the  GNU  General Public
// License along with GNU Make; see the file  COPYING.  If  not,  write
// to the Free Software Foundation, Inc., 51 Franklin St,  Fifth Floor,
// Boston, MA 02110-1301 USA.

#include "strutil.h"
#include <cctype>

// *c points to a string containing white space separated columns. This method
// returns a pointer to the zero-terminated next field. That might be identical
// with *c itself. The pointer c is then moved to the possible beginning of the
// next field.
char *next_field(char **c) {
    // skip leading spaces
    char *begin = *c;
    while (isspace(*begin) != 0) {
        begin++;
    }
    if (*begin == 0) {
        *c = begin;
        return nullptr;  // found end of string -> no more field
    }

    char *end = begin;  // copy pointer, search end of field
    while (*end != 0 && isspace(*end) == 0) {
        end++;  // search for \0 or white space
    }
    if (*end != 0) {  // string continues -> terminate field with '\0'
        *end = '\0';
        *c = end + 1;  // skip to character right *after* '\0'
    } else {
        *c = end;  // no more field, point to '\0'
    }
    return begin;
}

/* similar to next_field() but takes one character as delimiter */
char *next_token(char **c, char delim) {
    char *begin = *c;
    if (*begin == 0) {
        *c = begin;
        return nullptr;
    }

    char *end = begin;
    while (*end != 0 && *end != delim) {
        end++;
    }
    if (*end != 0) {
        *end = 0;
        *c = end + 1;
    } else {
        *c = end;
    }
    return begin;
}

/* same as next_token() but returns "" instead of 0 if
   no tokens has been found */
const char *safe_next_token(char **c, char delim) {
    if (*c == nullptr) {
        return "";
    }
    char *result = next_token(c, delim);
    return result != nullptr ? result : "";
}
