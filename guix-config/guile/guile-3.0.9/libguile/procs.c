/* Copyright 1995-1997,1999-2001,2006,2008-2013,2017-2018,2020
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

#include "deprecation.h"
#include "goops.h"
#include "gsubr.h"
#include "loader.h"
#include "procprop.h"
#include "programs.h"
#include "smob.h"
#include "strings.h"
#include "struct.h"
#include "symbols.h"
#include "vectors.h"

#include "procs.h"




/* {Procedures}
 */


SCM_DEFINE (scm_procedure_p, "procedure?", 1, 0, 0, 
	    (SCM obj),
	    "Return @code{#t} if @var{obj} is a procedure.")
#define FUNC_NAME s_scm_procedure_p
{
  return scm_from_bool (SCM_PROGRAM_P (obj)
                        || (SCM_STRUCTP (obj) && SCM_STRUCT_APPLICABLE_P (obj))
                        || (SCM_HAS_TYP7 (obj, scm_tc7_smob)
                            && SCM_SMOB_APPLICABLE_P (obj)));
}
#undef FUNC_NAME

SCM_DEFINE (scm_thunk_p, "thunk?", 1, 0, 0, 
	    (SCM obj),
	    "Return @code{#t} if @var{obj} is a procedure that can be "
            "called with zero arguments.")
#define FUNC_NAME s_scm_thunk_p
{
  int req, opt, rest;
  return scm_from_bool (scm_i_procedure_arity (obj, &req, &opt, &rest)
                        && req == 0);
}
#undef FUNC_NAME


/* Procedure-with-setter
 */

static SCM pws_vtable;


SCM_DEFINE (scm_procedure_with_setter_p, "procedure-with-setter?", 1, 0, 0, 
            (SCM obj),
	    "Return @code{#t} if @var{obj} is a procedure with an\n"
	    "associated setter procedure.")
#define FUNC_NAME s_scm_procedure_with_setter_p
{
  return scm_from_bool (SCM_STRUCTP (obj) && SCM_STRUCT_SETTER_P (obj));
}
#undef FUNC_NAME

SCM_DEFINE (scm_make_procedure_with_setter, "make-procedure-with-setter", 2, 0, 0, 
            (SCM procedure, SCM setter),
	    "Create a new procedure which behaves like @var{procedure}, but\n"
	    "with the associated setter @var{setter}.")
#define FUNC_NAME s_scm_make_procedure_with_setter
{
  SCM_VALIDATE_PROC (1, procedure);
  SCM_VALIDATE_PROC (2, setter);
  return scm_make_struct_no_tail (pws_vtable, scm_list_2 (procedure, setter));
}
#undef FUNC_NAME

SCM_DEFINE (scm_procedure, "procedure", 1, 0, 0, 
            (SCM proc),
	    "Return the procedure of @var{proc}, which must be an\n"
	    "applicable struct.")
#define FUNC_NAME s_scm_procedure
{
  SCM_ASSERT (SCM_STRUCTP (proc) && SCM_STRUCT_APPLICABLE_P (proc),
              proc, SCM_ARG1, FUNC_NAME);
  return SCM_STRUCT_PROCEDURE (proc);
}
#undef FUNC_NAME

SCM_PRIMITIVE_GENERIC (scm_setter, "setter", 1, 0, 0,
                       (SCM proc),
                       "Return the setter of @var{proc}, which must be an\n"
                       "applicable struct with a setter.")
#define FUNC_NAME s_scm_setter
{
  if (SCM_UNLIKELY (!SCM_STRUCTP (proc)))
    return scm_wta_dispatch_1 (g_scm_setter, proc, SCM_ARG1, FUNC_NAME);
  if (SCM_STRUCT_SETTER_P (proc))
    return SCM_STRUCT_SETTER (proc);
  return scm_wta_dispatch_1 (g_scm_setter, proc, SCM_ARG1, FUNC_NAME);
}
#undef FUNC_NAME


void
scm_init_procs ()
{
  pws_vtable =
    scm_c_make_struct (scm_applicable_struct_with_setter_vtable_vtable,
                       0,
                       1,
                       SCM_UNPACK (scm_from_latin1_symbol ("pwpw")));

#include "procs.x"
}
