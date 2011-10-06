/* The Ruby interface to the notmuch mail library
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

#include "defs.h"

static void notmuch_rb_refmark (notmuch_rb_reference_t *reference)
{
    if (reference &&
	reference->ref &&
	talloc_reference_count (reference->ref) > 1)
	rb_gc_mark (reference->val);
}

static void notmuch_rb_reffree (notmuch_rb_reference_t *reference)
{
    if (reference) {
	if (reference->ref)
	    talloc_unlink (NULL, reference->ref);
	free (reference);
    }
}

VALUE notmuch_rb_refmake (VALUE klass, void *ref)
{
    notmuch_rb_reference_t *reference;
    VALUE object;

    object = Data_Make_Struct (klass,
			       notmuch_rb_reference_t,
			       notmuch_rb_refmark,
			       notmuch_rb_reffree,
			       reference);

    reference->val = object;
    reference->ref = ref;

    if (reference->ref)
	talloc_increase_ref_count(reference->ref);

    return object;
}

VALUE notmuch_rb_refalloc (VALUE klass)
{
    return notmuch_rb_refmake (klass, NULL);
}

VALUE notmuch_rb_refdestroy (VALUE self)
{
    notmuch_rb_reference_t *reference;

    Data_Get_Struct (self, notmuch_rb_reference_t, reference);

    if (reference && reference->ref) {
	talloc_unlink (NULL, reference->ref);
	reference->ref = NULL;
	return Qnil;
    }

    rb_raise (rb_eRuntimeError, "destroyed notmuch reference");
}

void *notmuch_rb_refget (VALUE self)
{
    notmuch_rb_reference_t *reference;

    Data_Get_Struct (self, notmuch_rb_reference_t, reference);

    if (!reference || !reference->ref)
	rb_raise(rb_eRuntimeError, "notmuch reference lost");

    return reference->ref;
}
