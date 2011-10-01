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

#include "maildir.h"

static const char *const maildir_subdirs_array[] = {"new", "cur", "tmp"};

/* FIXME: The two functions below, dirent_sort_inode and get_dtype duplicate
 * code from notmuch-new.c
 */
static int
dirent_sort_inode (const struct dirent **a, const struct dirent **b)
{
    return ((*a)->d_ino < (*b)->d_ino) ? -1 : 1;
}

static unsigned char
get_dtype(const char *fullpath, struct dirent *entry)
{
    struct stat buf;

    if (entry->d_type != DT_UNKNOWN)
	return entry->d_type;

    if (lstat(fullpath, &buf) == -1) {
	fprintf (stderr, "Warning: stat failed on `%s': %s\n",
		 fullpath, strerror(errno));
	return DT_UNKNOWN;
    }

    if (S_ISREG (buf.st_mode)) {
	return DT_REG;
    } else if (S_ISDIR (buf.st_mode)) {
	return DT_DIR;
    } else if (S_ISLNK (buf.st_mode)) {
	return DT_LNK;
    }

    return DT_UNKNOWN;
}

static notmuch_bool_t
maildir_access (const char *path)
{
    struct stat buf;

    if (access (path, R_OK | W_OK | X_OK) == -1) {
	fprintf (stderr, "Failed to access path `%s': %s\n",
		 path, strerror(errno));
	return FALSE;
    }

    if (lstat(path, &buf) == -1) {
	fprintf (stderr, "Failed to access path `%s': %s\n",
		 path, strerror(errno));
	return FALSE;
    }

    if (!S_ISDIR(buf.st_mode)) {
	fprintf (stderr, "Path `%s' is not a directory\n", path);
	return FALSE;
    }

    return TRUE;
}

/* Determine whether the source message is in 'new' or in 'cur';
 * we ignore messages in 'tmp' for obvious reasons
 */
static notmuch_bool_t
maildir_subdir (const char *src, notmuch_bool_t *in_cur)
{
    char *srcpath;

    srcpath = g_path_get_dirname (src);

    if (g_str_has_suffix (srcpath, "new"))
	*in_cur = FALSE;
    else if (g_str_has_suffix (srcpath, "cur"))
	*in_cur = TRUE;
    else {
	g_free (srcpath);
	errno = EINVAL;
	return FALSE;
    }

    g_free(srcpath);
    return TRUE;
}

static char *
maildir_transform_path (const char *src, const char *targetpath)
{
    char *targetfullpath, *srcfile;
    notmuch_bool_t in_cur;

    if (!maildir_subdir (src, &in_cur)) {
	fprintf (stderr, "Invalid maildir subdirectory `%s': %s\n",
		 src, strerror(errno));
	return NULL;
    }

    srcfile = g_path_get_basename (src);
    targetfullpath = g_strdup_printf ("%s%c%s%c%s",
				      targetpath,
				      G_DIR_SEPARATOR,
				      in_cur ? "cur" : "new",
				      G_DIR_SEPARATOR,
				      srcfile);
    g_free (srcfile);

    return targetfullpath;
}

notmuch_bool_t
maildir_check (const char *path, notmuch_bool_t makedir, mode_t mode)
{
    int i;
    char *fullpath = NULL;

    for (i = 0; i != G_N_ELEMENTS (maildir_subdirs_array); i++) {
	    fullpath = g_build_filename (path, maildir_subdirs_array[i], NULL);
	    if (!makedir && !maildir_access(fullpath))
		goto FAIL;
	    if (makedir && g_mkdir_with_parents (fullpath, (int)mode) != 0) {
		fprintf (stderr, "Error creating %s: %s\n",
			 fullpath, strerror(errno));
		goto FAIL;
	    }
	    g_free (fullpath);
    }

    return TRUE;

  FAIL:
    g_free (fullpath);
    return FALSE;
}

notmuch_bool_t
maildir_rename (const char *src, const char *targetpath, rename_method_t methr)
{
    int ret;
    char *targetfullpath;

    targetfullpath = maildir_transform_path (src, targetpath);
    if (!targetfullpath)
	return FALSE;

    switch (methr) {
    case RENAME_SYMLINK:
	ret = symlink (src, targetfullpath);
	break;
    case RENAME_HARDLINK:
	ret = link (src, targetfullpath);
	break;
    default:
	return FALSE;
    }

    if (ret == -1) {
	if (errno != EEXIST) {
	    fprintf (stderr, "Failed to link %s to %s: %s\n",
		     src, targetfullpath, strerror(errno));
	}
	g_free (targetfullpath);
	return FALSE;
    }

    g_free (targetfullpath);
    return TRUE;
}

int
maildir_clean_recursive (const char *path, clean_method_t methc)
{
    int i, count, ret, num_fs_entries;
    notmuch_bool_t delete;
    char *fullpath = NULL;
    struct dirent *entry = NULL;
    struct dirent **fs_entries = NULL;
    struct stat buf;

    if (methc == CLEAN_NONE)
	return 0;

    num_fs_entries = scandir(path, &fs_entries, NULL, dirent_sort_inode);
    if (num_fs_entries == -1) {
	fprintf (stderr, "Error opening directory %s: %s\n",
		 path, strerror(errno));
	return -1;
    }

    count = 0;
    for (i = 0; i < num_fs_entries; i++) {
	entry = fs_entries[i];

	if (!entry->d_name ||
	    strcmp (entry->d_name, ".") == 0 ||
	    strcmp (entry->d_name, "..") == 0 ||
	    strcmp (entry->d_name, "tmp") == 0)
	{
	    continue;
	}

	delete = FALSE;
	fullpath = g_build_filename (path, entry->d_name, NULL);
	switch (get_dtype(fullpath, entry)) {
	case DT_REG:
	    if (methc == CLEAN_ALL)
		delete = TRUE;
	    break;
	case DT_LNK:
	    if (methc == CLEAN_ALL ||
		methc == CLEAN_SYMLINK ||
		(methc == CLEAN_DANGLING &&
		 (stat(fullpath, &buf) == -1 &&
		  (errno == ENOENT || errno == ELOOP))))
		delete = TRUE;
	    break;
	case DT_DIR:
	    ret = maildir_clean_recursive (fullpath, methc);
	    if (ret != -1)
		count += ret;
	    break;
	default:
	    break; /* skip the rest */
	}

	if (delete) {
	    if (unlink (fullpath) == -1) {
		fprintf (stderr, "Warning: error unlinking `%s': %s",
			 fullpath, strerror(errno));
	    } else {
		count++;
	    }
	}

	g_free (fullpath);
    }

    return count;
}
