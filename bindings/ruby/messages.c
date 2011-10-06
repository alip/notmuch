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

/* call-seq: MESSAGES.each {|item| block } => MESSAGES
 *
 * Calls +block+ once for each message in +self+, passing that element as a
 * parameter.
 */
VALUE
notmuch_rb_messages_each (VALUE self)
{
    notmuch_message_t *message;
    notmuch_messages_t *messages;

    Data_Get_Notmuch_Reference (self, notmuch_messages_t, messages);

    for (; notmuch_messages_valid (messages); notmuch_messages_move_to_next (messages)) {
	message = notmuch_messages_get (messages);
	rb_yield (notmuch_rb_reference_wrap (notmuch_rb_cMessage, message));
    }

    return self;
}

/*
 * call-seq: MESSAGES.tags => TAGS
 *
 * Collect tags from the messages
 */
VALUE
notmuch_rb_messages_collect_tags (VALUE self)
{
    notmuch_tags_t *tags;
    notmuch_messages_t *messages;

    Data_Get_Notmuch_Reference (self, notmuch_messages_t, messages);

    tags = notmuch_messages_collect_tags (messages);
    if (!tags)
	rb_raise (notmuch_rb_eMemoryError, "Out of memory");

    return notmuch_rb_reference_wrap (notmuch_rb_cTags, tags);
}
