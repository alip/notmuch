/* The Ruby interface to the notmuch mail library
 *
 * Copyright © 2010, 2011 Ali Polatel
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

#include "defs.h"

/*
 * call-seq: QUERY.sort => fixnum
 *
 * Get sort type of the +QUERY+
 */
VALUE
notmuch_rb_query_get_sort (VALUE self)
{
    notmuch_query_t *query;

    Data_Get_Notmuch_Reference (self, notmuch_query_t, query);

    return FIX2INT (notmuch_query_get_sort (query));
}

/*
 * call-seq: QUERY.sort=(fixnum) => nil
 *
 * Set sort type of the +QUERY+
 */
VALUE
notmuch_rb_query_set_sort (VALUE self, VALUE sortv)
{
    notmuch_query_t *query;

    Data_Get_Notmuch_Reference (self, notmuch_query_t, query);

    if (!FIXNUM_P (sortv))
	rb_raise (rb_eTypeError, "Not a Fixnum");

    notmuch_query_set_sort (query, FIX2UINT (sortv));

    return Qnil;
}

/*
 * call-seq: QUERY.to_s => string
 *
 * Get query string of the +QUERY+
 */
VALUE
notmuch_rb_query_get_string (VALUE self)
{
    notmuch_query_t *query;

    Data_Get_Notmuch_Reference (self, notmuch_query_t, query);

    return rb_str_new2 (notmuch_query_get_query_string (query));
}

/*
 * call-seq: QUERY.search_threads => THREADS
 *
 * Search for threads
 */
VALUE
notmuch_rb_query_search_threads (VALUE self)
{
    notmuch_query_t *query;
    notmuch_threads_t *threads;

    Data_Get_Notmuch_Reference (self, notmuch_query_t, query);

    threads = notmuch_query_search_threads (query);
    if (!threads)
	rb_raise (notmuch_rb_eMemoryError, "Out of memory");

    return notmuch_rb_reference_wrap (notmuch_rb_cThreads, threads);
}

/*
 * call-seq: QUERY.search_messages => MESSAGES
 *
 * Search for messages
 */
VALUE
notmuch_rb_query_search_messages (VALUE self)
{
    notmuch_query_t *query;
    notmuch_messages_t *messages;

    Data_Get_Notmuch_Reference (self, notmuch_query_t, query);

    messages = notmuch_query_search_messages (query);
    if (!messages)
	rb_raise (notmuch_rb_eMemoryError, "Out of memory");

    return notmuch_rb_reference_wrap (notmuch_rb_cMessages, messages);
}
