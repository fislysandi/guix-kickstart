/* Copyright 2002,2006,2008,2018
     Free Software Foundation, Inc.

   This file is part of Guile.

   Guile is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Guile is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Guile.  If not, see
   <https://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include "libguile/scmconfig.h"


#if SCM_USE_NULL_THREADS
#include "null-threads.h"

static scm_i_pthread_key_t *all_keys = NULL;

static void
destroy_keys (void)
{
  scm_i_pthread_key_t *key;
  int again;

  do {
    again = 0;
    for (key = all_keys; key; key = key->next)
      if (key->value && key->destr_func)
	{
	  void *v = key->value;
	  key->value = NULL;
	  key->destr_func (v);
	  again = 1;
	}
  } while (again);
}

int
scm_i_pthread_key_create (scm_i_pthread_key_t *key,
			  void (*destr_func) (void *))
{
  if (all_keys == NULL)
    atexit (destroy_keys);

  key->next = all_keys;
  all_keys = key;
  key->value = NULL;
  key->destr_func = destr_func;

  return 0;
}

#endif /* SCM_USE_NULL_THREADS */
