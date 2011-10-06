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

/* call-seq: THREADS.each {|item| block } => THREADS
 *
 * Calls +block+ once for each thread in +self+, passing that element as a
 * parameter.
 */
VALUE
notmuch_rb_threads_each (VALUE self)
{
    notmuch_thread_t *thread;
    notmuch_threads_t *threads;

    Data_Get_Notmuch_Reference (self, notmuch_threads_t, threads);

    for (; notmuch_threads_valid (threads); notmuch_threads_move_to_next (threads)) {
	thread = notmuch_threads_get (threads);
	rb_yield (notmuch_rb_reference_wrap (notmuch_rb_cThread, thread));
    }

    return self;
}
