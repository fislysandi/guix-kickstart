/* Copyright 1995-2001,2003-2004,2006,2008-2009,2011,2013,2015,2018
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

#include <string.h>
#include <stdarg.h>

#include "async.h"
#include "dynwind.h"
#include "gsubr.h"
#include "hashtab.h"
#include "list.h"
#include "pairs.h"
#include "ports.h"
#include "smob.h"
#include "strings.h"
#include "symbols.h"

#include "keywords.h"



static SCM keyword_obarray;

#define SCM_KEYWORDP(x) (SCM_HAS_TYP7 (x, scm_tc7_keyword))
#define SCM_KEYWORD_SYMBOL(x) (SCM_CELL_OBJECT_1 (x))

SCM_DEFINE (scm_keyword_p, "keyword?", 1, 0, 0, 
            (SCM obj),
	    "Return @code{#t} if the argument @var{obj} is a keyword, else\n"
	    "@code{#f}.")
#define FUNC_NAME s_scm_keyword_p
{
  return scm_from_bool (SCM_KEYWORDP (obj));
}
#undef FUNC_NAME

SCM_DEFINE (scm_symbol_to_keyword, "symbol->keyword", 1, 0, 0,
	    (SCM symbol),
	    "Return the keyword with the same name as @var{symbol}.")
#define FUNC_NAME s_scm_symbol_to_keyword
{
  SCM keyword;

  SCM_ASSERT_TYPE (scm_is_symbol (symbol), symbol, 0, NULL, "symbol");

  scm_dynwind_begin (0);
  scm_i_dynwind_pthread_mutex_lock (&scm_i_misc_mutex);
  /* Note: `scm_cell' and `scm_hashq_set_x' can raise an out-of-memory
     error.  */
  keyword = scm_hashq_ref (keyword_obarray, symbol, SCM_BOOL_F);
  if (scm_is_false (keyword))
    {
      keyword = scm_cell (scm_tc7_keyword, SCM_UNPACK (symbol));
      scm_hashq_set_x (keyword_obarray, symbol, keyword);
    }
  scm_dynwind_end ();
  return keyword;
}
#undef FUNC_NAME

SCM_DEFINE (scm_keyword_to_symbol, "keyword->symbol", 1, 0, 0,
	    (SCM keyword),
	    "Return the symbol with the same name as @var{keyword}.")
#define FUNC_NAME s_scm_keyword_to_symbol
{
  SCM_VALIDATE_KEYWORD (1, keyword);
  return SCM_KEYWORD_SYMBOL (keyword);
}
#undef FUNC_NAME

int
scm_is_keyword (SCM val)
{
  return SCM_KEYWORDP (val);
}

SCM
scm_from_locale_keyword (const char *name)
{
  return scm_symbol_to_keyword (scm_from_locale_symbol (name));
}

SCM
scm_from_locale_keywordn (const char *name, size_t len)
{
  return scm_symbol_to_keyword (scm_from_locale_symboln (name, len));
}

SCM
scm_from_latin1_keyword (const char *name)
{
  return scm_symbol_to_keyword (scm_from_latin1_symbol (name));
}

SCM
scm_from_utf8_keyword (const char *name)
{
  return scm_symbol_to_keyword (scm_from_utf8_symbol (name));
}

SCM_SYMBOL (scm_keyword_argument_error, "keyword-argument-error");

void
scm_c_bind_keyword_arguments (const char *subr, SCM rest,
                              scm_t_keyword_arguments_flags flags, ...)
{
  va_list va;

  while (scm_is_pair (rest))
    {
      SCM kw_or_arg = SCM_CAR (rest);
      SCM tail = SCM_CDR (rest);

      if (scm_is_keyword (kw_or_arg))
        {
          SCM kw;
          SCM *arg_p;

          va_start (va, flags);
          for (;;)
            {
              kw = va_arg (va, SCM);
              if (SCM_UNBNDP (kw))
                {
                  /* KW_OR_ARG is not in the list of expected keywords.  */
                  if (!(flags & SCM_ALLOW_OTHER_KEYS))
                    scm_error_scm (scm_keyword_argument_error,
				   scm_from_locale_string (subr),
				   scm_from_latin1_string
				   ("Unrecognized keyword"),
				   SCM_EOL, scm_list_1 (kw_or_arg));

                  /* Advance REST.  Advance past the argument of an
                     unrecognized keyword, but don't error if such a
                     keyword has no argument.  */
                  rest = scm_is_pair (tail) ? SCM_CDR (tail) : tail;
                  break;
                }
              arg_p = va_arg (va, SCM *);
              if (scm_is_eq (kw_or_arg, kw))
                {
                  /* We found the matching keyword.  Store the
                     associated value and break out of the loop.  */
                  if (!scm_is_pair (tail))
                    scm_error_scm (scm_keyword_argument_error,
				   scm_from_locale_string (subr),
				   scm_from_latin1_string
				   ("Keyword argument has no value"),
				   SCM_EOL, scm_list_1 (kw));
                  *arg_p = SCM_CAR (tail);
                  /* Advance REST.  */
                  rest = SCM_CDR (tail);
                  break;
                }
            }
          va_end (va);
        }
      else
        {
          /* The next argument is not a keyword, or is a singleton
             keyword at the end of REST.  */
          if (!(flags & SCM_ALLOW_NON_KEYWORD_ARGUMENTS))
            scm_error_scm (scm_keyword_argument_error,
			   scm_from_locale_string (subr),
			   scm_from_latin1_string ("Invalid keyword"),
			   SCM_EOL, scm_list_1 (kw_or_arg));

           /* Advance REST.  */
           rest = tail;
        }
    }
}

void
scm_init_keywords ()
{
  keyword_obarray = scm_c_make_hash_table (0);
#include "keywords.x"
}

