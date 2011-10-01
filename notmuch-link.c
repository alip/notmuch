/* notmuch - Not much of an email program, (just index and search)
 *
 * Copyright © 2011 Ali Polatel
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
 * Author: Ali Polatel <alip@exherbo.org>
 */

#include "notmuch-client.h"

typedef struct {
    const char *maildir;

    notmuch_bool_t cleandir;
    notmuch_bool_t makedir;
    notmuch_bool_t entire_thread;
    mode_t mode;

    clean_method_t clean_method;
    rename_method_t rename_method;
} link_options_t;

static inline void
init_link_options(link_options_t *options)
{
    options->maildir = NULL;

    options->makedir = FALSE;
    options->entire_thread = FALSE;
    options->mode = 0700;

    options->clean_method = CLEAN_NONE;
    options->rename_method = RENAME_SYMLINK;
}

static inline const char *
rename_method_abbr(rename_method_t method)
{
    switch (method) {
    case RENAME_SYMLINK:
	return "sym";
    case RENAME_HARDLINK:
	return "hard";
    default:
	return "love";
    }
}

static inline const char *
rename_method_verb(rename_method_t method)
{
    switch (method) {
    case RENAME_SYMLINK:
	return "symlinked";
    case RENAME_HARDLINK:
	return "hardlinked";
    default:
	return "made love all day long!";
    }
}

static notmuch_bool_t
prepare_maildir (const char *maildir,
		 notmuch_bool_t makedir,
		 mode_t mode,
		 clean_method_t methc)
{
    int ret;

    if (!maildir_check(maildir, makedir, mode))
	return FALSE;

    ret = maildir_clean_recursive(maildir, methc);
    if (ret == -1)
	return FALSE;
    else if (ret > 0)
	printf("Unlinked %d entries under %s\n",
	       ret, maildir);

    return TRUE;
}

static int
link_messages (notmuch_messages_t *messages,
	       rename_method_t rename_method,
	       const char *maildir)
{
    int ret = 0;
    const char *path;
    notmuch_message_t *message;
    notmuch_filenames_t *filenames;

    for (;
	 notmuch_messages_valid (messages);
	 notmuch_messages_move_to_next (messages))
    {
	message = notmuch_messages_get (messages);

	filenames = notmuch_message_get_filenames (message);
	for (;
	     notmuch_filenames_valid (filenames);
	     notmuch_filenames_move_to_next (filenames))
	{
	    path = notmuch_filenames_get (filenames);
	    if (maildir_rename (path, maildir, rename_method)) {
		ret++;
	    }
	}
	notmuch_filenames_destroy( filenames );

	notmuch_message_destroy (message);
    }

    return ret;
}

static int
do_link_threads (notmuch_query_t *query,
		 const link_options_t *lopts)
{
    int message_count, thread_count;
    notmuch_threads_t *threads;
    notmuch_thread_t *thread;
    notmuch_messages_t *messages;

    threads = notmuch_query_search_threads (query);
    if (threads == NULL)
	return 1;

    if (!prepare_maildir(lopts->maildir,
			 lopts->makedir, lopts->mode,
			 lopts->clean_method))
	return 1;

    message_count = thread_count = 0;
    for (;
	 notmuch_threads_valid (threads);
	 notmuch_threads_move_to_next (threads))
    {
	thread = notmuch_threads_get (threads);

	messages = notmuch_thread_get_toplevel_messages (thread);

	if (messages == NULL)
	    INTERNAL_ERROR ("Thread %s has no toplevel messages.\n",
			    notmuch_thread_get_thread_id (thread));
	else
	    thread_count++;

	message_count += link_messages (messages,
					lopts->rename_method,
					lopts->maildir);

	notmuch_messages_destroy (messages);
	notmuch_thread_destroy (thread);
    }

    if (message_count > 0) {
	printf("%d messages in %d threads %s under %s\n",
	       message_count, thread_count,
	       rename_method_verb(lopts->rename_method),
	       lopts->maildir);
    }

    notmuch_threads_destroy (threads);

    return 0;
}

static int
do_link_messages (notmuch_query_t *query,
		  const link_options_t *lopts)
{
    int message_count;
    notmuch_messages_t *messages;

    messages = notmuch_query_search_messages (query);
    if (messages == NULL)
	return 1;

    if (!prepare_maildir(lopts->maildir,
			 lopts->makedir,
			 lopts->mode,
			 lopts->clean_method))
	return 1;

    message_count = link_messages (messages,
				   lopts->rename_method,
				   lopts->maildir);
    if (message_count > 0) {
	printf("%d messages %s under %s\n",
	       message_count,
	       rename_method_verb(lopts->rename_method),
	       lopts->maildir);
    }

    notmuch_messages_destroy (messages);

    return 0;
}

int
notmuch_link_command (void *ctx, int argc, char *argv[])
{
    int i, ret;
    char *query_str;
    char *opt;
    notmuch_config_t *config;
    notmuch_database_t *notmuch;
    notmuch_query_t *query;
    link_options_t options;

    init_link_options(&options);

    for (i = 0; i < argc && argv[i][0] == '-'; i++) {
	if (strcmp (argv[i], "--") == 0) {
	    i++;
	    break;
	}
	if (STRNCMP_LITERAL (argv[i], "--rename=") == 0) {
	    opt = argv[i] + sizeof ("--rename=") - 1;
	    if (strcmp (opt, "symlink") == 0) {
		options.rename_method = RENAME_SYMLINK;
	    } else if (strcmp (opt, "hardlink") == 0) {
		options.rename_method = RENAME_HARDLINK;
#if 0
#error TODO
	    } else if (strcmp (opt, "copy") == 0) {
		options.rename_method = RENAME_COPY;
	    } else if (strcmp (opt, "move") == 0) {
		options.rename_method = RENAME_MOVE;
#endif
	    } else {
		fprintf (stderr, "Invalid value for --rename: %s\n", opt);
		return 1;
	    }
	} else if (STRNCMP_LITERAL (argv[i], "--maildir=") == 0) {
	    opt = argv[i] + sizeof ("--maildir=") - 1;
	    options.maildir = talloc_strdup (ctx, opt);
	} else if (STRNCMP_LITERAL (argv[i], "--clean=") == 0) {
	    opt = argv[i] + sizeof ("--clean=") - 1;
	    if (strcmp (opt, "dangling") == 0) {
		options.clean_method = CLEAN_DANGLING;
	    } else if (strcmp (opt, "symlink") == 0) {
		options.clean_method = CLEAN_SYMLINK;
	    } else if (strcmp (opt, "all") == 0) {
		options.clean_method = CLEAN_ALL;
	    } else if (strcmp (opt, "none") == 0) {
		options.clean_method = CLEAN_NONE;
	    } else {
		fprintf (stderr, "Invalid value for --clean: %s\n", opt);
		return 1;
	    }
	} else if (STRNCMP_LITERAL (argv[i], "--mkdir") == 0) {
	    options.makedir = TRUE;

	    opt = argv[i] + sizeof ("--mkdir") - 1;

	    if (opt[0] == '\0') {
		continue;
	    } else if (opt[0] != '=') {
		fprintf (stderr, "Unrecognized option: %s\n", argv[i]);
		return 1;
	    } else {
		opt++; /* skip '=' */
	    }

	    options.mode = 0;
	    while (*opt >= '0' && *opt <= '7')
		options.mode = options.mode * 8 + (*opt++ - '0');
	    if (*opt) {
		fprintf (stderr, "Invalid value for --mkdir: %s\n", opt);
		return 1;
	    }
	} else if (STRNCMP_LITERAL (argv[i], "--entire-thread") == 0) {
	    options.entire_thread = TRUE;
	} else {
	    fprintf (stderr, "Unrecognized option: %s\n", argv[i]);
	    return 1;
	}
    }

    if (!options.maildir) {
	fprintf (stderr, "Target directory must be specified "
			 "using --maildir option\n");
	return 1;
    }

    argc -= i;
    argv += i;

    config = notmuch_config_open (ctx, NULL, NULL);
    if (config == NULL)
	return 1;

    notmuch = notmuch_database_open (notmuch_config_get_database_path (config),
				     NOTMUCH_DATABASE_MODE_READ_ONLY);
    if (notmuch == NULL)
	return 1;

    query_str = query_string_from_args (notmuch, argc, argv);
    if (query_str == NULL) {
	fprintf (stderr, "Out of memory.\n");
	return 1;
    }
    if (*query_str == '\0') {
	fprintf (stderr, "Error: notmuch link requires at least one search term.\n");
	return 1;
    }

    query = notmuch_query_create (notmuch, query_str);
    if (query == NULL) {
	fprintf (stderr, "Out of memory\n");
	return 1;
    }

    if (options.entire_thread) {
	ret = do_link_threads (query, &options);
    } else {
	ret = do_link_messages (query, &options);
    }

    notmuch_query_destroy (query);
    notmuch_database_close (notmuch);

    return ret;
}
