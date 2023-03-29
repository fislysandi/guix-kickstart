/* Copyright 1995-1998,2000-2001,2003-2004,2009-2014,2018
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
#  include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>  /* for getenv, system, exit, free */
#include <unistd.h>  /* for _exit */

#include "boolean.h"
#include "gsubr.h"
#include "numbers.h"
#include "strings.h"

#include "simpos.h"



SCM_DEFINE (scm_system, "system", 0, 1, 0,
           (SCM cmd),
	    "Execute @var{cmd} using the operating system's \"command\n"
	    "processor\".  Under Unix this is usually the default shell\n"
	    "@code{sh}.  The value returned is @var{cmd}'s exit status as\n"
	    "returned by @code{waitpid}, which can be interpreted using\n"
	    "@code{status:exit-val} and friends.\n"
	    "\n"
	    "If @code{system} is called without arguments, return a boolean\n"
	    "indicating whether the command processor is available.")
#define FUNC_NAME s_scm_system
{
  int rv, eno;
  char *c_cmd;
  
  if (SCM_UNBNDP (cmd))
    {
      rv = system (NULL);
      return scm_from_bool (rv);
    }  
  SCM_VALIDATE_STRING (1, cmd);
  errno = 0;
  c_cmd = scm_to_locale_string (cmd);
  rv = system (c_cmd);
  eno = errno; free (c_cmd); errno = eno;
  if (rv == -1 || (rv == 127 && errno != 0))
    SCM_SYSERROR;
  return scm_from_int (rv);
}
#undef FUNC_NAME


SCM_DEFINE (scm_getenv, "getenv", 1, 0, 0, 
            (SCM nam),
	    "Looks up the string @var{nam} in the current environment.  The return\n"
	    "value is @code{#f} unless a string of the form @code{NAME=VALUE} is\n"
	    "found, in which case the string @code{VALUE} is returned.")
#define FUNC_NAME s_scm_getenv
{
  char *val;
  char *var = scm_to_locale_string (nam);
  val = getenv (var);
  free (var);
  return val ? scm_from_locale_string (val) : SCM_BOOL_F;
}
#undef FUNC_NAME

/* Get an integer from an environment variable.  */
int
scm_getenv_int (const char *var, int def)
{
  char *end = 0;
  char *val = getenv (var);
  long res = def;
  if (!val)
    return def;
  res = strtol (val, &end, 10);
  if (end == val)
    return def;
  return res;
}

/* simple exit, without unwinding the scheme stack or flushing ports.  */
SCM_DEFINE (scm_primitive_exit, "primitive-exit", 0, 1, 0, 
            (SCM status),
	    "Terminate the current process without unwinding the Scheme\n"
	    "stack.  The exit status is @var{status} if supplied, otherwise\n"
	    "zero.")
#define FUNC_NAME s_scm_primitive_exit
{
  int cstatus = 0;
  if (!SCM_UNBNDP (status))
    cstatus = scm_to_int (status);
  exit (cstatus);
}
#undef FUNC_NAME

SCM_DEFINE (scm_primitive__exit, "primitive-_exit", 0, 1, 0,
            (SCM status),
	    "Terminate the current process using the _exit() system call and\n"
	    "without unwinding the Scheme stack.  The exit status is\n"
	    "@var{status} if supplied, otherwise zero.\n"
	    "\n"
	    "This function is typically useful after a fork, to ensure no\n"
	    "Scheme cleanups or @code{atexit} handlers are run (those\n"
	    "usually belonging in the parent rather than the child).")
#define FUNC_NAME s_scm_primitive__exit
{
  int cstatus = 0;
  if (!SCM_UNBNDP (status))
    cstatus = scm_to_int (status);
  _exit (cstatus);
}
#undef FUNC_NAME



void
scm_init_simpos ()
{
#include "simpos.x"
}

