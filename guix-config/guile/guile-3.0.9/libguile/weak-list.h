#ifndef SCM_WEAK_LIST_H
#define SCM_WEAK_LIST_H

/* Copyright 2016,2018
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



#include "libguile/pairs.h"
#include "libguile/weak-vector.h"



static inline SCM
scm_i_weak_cons (SCM car, SCM cdr)
{
  return scm_cons (scm_c_make_weak_vector (1, car), cdr);
}

static inline SCM
scm_i_weak_car (SCM pair)
{
  return scm_c_weak_vector_ref (scm_car (pair), 0);
}

static inline void
scm_i_visit_weak_list (SCM *list_loc, void (*visit) (SCM))
{
  SCM in = *list_loc, out = SCM_EOL;

  while (scm_is_pair (in))
    {
      SCM car = scm_i_weak_car (in);
      SCM cdr = scm_cdr (in);

      if (!scm_is_eq (car, SCM_BOOL_F))
        {
          scm_set_cdr_x (in, out);
          out = in;
          visit (car);
        }

      in = cdr;
    }

  *list_loc = out;
}


#endif  /* SCM_WEAK_LIST_H */
