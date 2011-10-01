/* Maildir utilities for the notmuch mail library
 *
 * Copyright © 2011 Ali Polatel
 * Based in part upon mu which is:
 *   Copyright © 2008-2011 Dirk-Jan C. Binnema <djcb@djcbsoftware.nl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/ .
 *
 * Author: Ali Polatel <polatel@gmail.com>
 */

#ifndef NOTMUCH_MAILDIR_H
#define NOTMUCH_MAILDIR_H

#include "notmuch-client.h"

typedef enum {
    RENAME_SYMLINK,
    RENAME_HARDLINK,
    /* TODO:
     * RENAME_COPY,
     * RENAME_MOVE,
     */
} rename_method_t;

typedef enum {
    CLEAN_DANGLING,
    CLEAN_SYMLINK,
    CLEAN_ALL,
    CLEAN_NONE
} clean_method_t;

notmuch_bool_t
maildir_check (const char *path, notmuch_bool_t makedir, mode_t mode);

notmuch_bool_t
maildir_rename (const char *src, const char *targetpath, rename_method_t methr);

int
maildir_clean_recursive(const char *path, clean_method_t methc);

#endif
