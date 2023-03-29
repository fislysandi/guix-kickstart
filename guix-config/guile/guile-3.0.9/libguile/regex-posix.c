/* Copyright 1997-2001,2004,2006-2007,2010-2012,2018-2019
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




/* regex-posix.c -- POSIX regular expression support.

   This code was written against Henry Spencer's famous regex package.
   The principal reference for POSIX behavior was the man page for this
   library, not the 1003.2 document itself.  Ergo, other `POSIX'
   libraries which do not agree with the Spencer implementation may
   produce varying behavior.  Sigh. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <wchar.h>

#include "async.h"
#include "feature.h"
#include "gsubr.h"
#include "list.h"
#include "modules.h"
#include "numbers.h"
#include "pairs.h"
#include "ports.h"
#include "smob.h"
#include "strings.h"
#include "strports.h"
#include "symbols.h"
#include "vectors.h"

#include "regex-posix.h"

/* This is defined by some regex libraries and omitted by others. */
#ifndef REG_BASIC
#define REG_BASIC 0
#endif

scm_t_bits scm_tc16_regex;

static size_t
regex_free (SCM obj)
{
  regfree (SCM_RGX (obj));
  scm_gc_free (SCM_RGX (obj), sizeof(regex_t), "regex");
  return 0;
}



SCM_SYMBOL (scm_regexp_error_key, "regular-expression-syntax");

static SCM
scm_regexp_error_msg (int regerrno, regex_t *rx)
{
  char *errmsg;
  int l;

  errmsg = scm_malloc (80);
  l = regerror (regerrno, rx, errmsg, 80);
  if (l > 80)
    {
      free (errmsg);
      errmsg = scm_malloc (l);
      regerror (regerrno, rx, errmsg, l);
    }
  return scm_take_locale_string (errmsg);
}

SCM_DEFINE (scm_regexp_p, "regexp?", 1, 0, 0,
            (SCM obj),
	    "Return @code{#t} if @var{obj} is a compiled regular expression,\n"
	    "or @code{#f} otherwise.")
#define FUNC_NAME s_scm_regexp_p
{
  return scm_from_bool(SCM_RGXP (obj));
}
#undef FUNC_NAME

SCM_DEFINE (scm_make_regexp, "make-regexp", 1, 0, 1,
            (SCM pat, SCM flags),
	    "Compile the regular expression described by @var{pat}, and\n"
	    "return the compiled regexp structure.  If @var{pat} does not\n"
	    "describe a legal regular expression, @code{make-regexp} throws\n"
	    "a @code{regular-expression-syntax} error.\n"
	    "\n"
	    "The @var{flags} arguments change the behavior of the compiled\n"
	    "regular expression.  The following flags may be supplied:\n"
	    "\n"
	    "@table @code\n"
	    "@item regexp/icase\n"
	    "Consider uppercase and lowercase letters to be the same when\n"
	    "matching.\n"
	    "@item regexp/newline\n"
	    "If a newline appears in the target string, then permit the\n"
	    "@samp{^} and @samp{$} operators to match immediately after or\n"
	    "immediately before the newline, respectively.  Also, the\n"
	    "@samp{.} and @samp{[^...]} operators will never match a newline\n"
	    "character.  The intent of this flag is to treat the target\n"
	    "string as a buffer containing many lines of text, and the\n"
	    "regular expression as a pattern that may match a single one of\n"
	    "those lines.\n"
	    "@item regexp/basic\n"
	    "Compile a basic (``obsolete'') regexp instead of the extended\n"
	    "(``modern'') regexps that are the default.  Basic regexps do\n"
	    "not consider @samp{|}, @samp{+} or @samp{?} to be special\n"
	    "characters, and require the @samp{@{...@}} and @samp{(...)}\n"
	    "metacharacters to be backslash-escaped (@pxref{Backslash\n"
	    "Escapes}).  There are several other differences between basic\n"
	    "and extended regular expressions, but these are the most\n"
	    "significant.\n"
	    "@item regexp/extended\n"
	    "Compile an extended regular expression rather than a basic\n"
	    "regexp.  This is the default behavior; this flag will not\n"
	    "usually be needed.  If a call to @code{make-regexp} includes\n"
	    "both @code{regexp/basic} and @code{regexp/extended} flags, the\n"
	    "one which comes last will override the earlier one.\n"
	    "@end table")
#define FUNC_NAME s_scm_make_regexp
{
  SCM flag;
  regex_t *rx;
  int status, cflags;
  char *c_pat;

  SCM_VALIDATE_STRING (1, pat);
  SCM_VALIDATE_REST_ARGUMENT (flags);

  /* Examine list of regexp flags.  If REG_BASIC is supplied, then
     turn off REG_EXTENDED flag (on by default). */
  cflags = REG_EXTENDED;
  flag = flags;
  while (!scm_is_null (flag))
    {
      if (scm_to_int (SCM_CAR (flag)) == REG_BASIC)
	cflags &= ~REG_EXTENDED;
      else
	cflags |= scm_to_int (SCM_CAR (flag));
      flag = SCM_CDR (flag);
    }

  rx = scm_gc_malloc_pointerless (sizeof (regex_t), "regex");
  c_pat = scm_to_locale_string (pat);
  status = regcomp (rx, c_pat,
		    /* Make sure they're not passing REG_NOSUB;
                       regexp-exec assumes we're getting match data.  */
		    cflags & ~REG_NOSUB);
  free (c_pat);
  if (status != 0)
    {
      SCM errmsg = scm_regexp_error_msg (status, rx);
      scm_gc_free (rx, sizeof(regex_t), "regex");
      scm_error_scm (scm_regexp_error_key,
		     scm_from_utf8_string (FUNC_NAME),
		     errmsg,
		     SCM_BOOL_F,
		     scm_list_1 (pat));
      
      /* never returns */
    }
  SCM_RETURN_NEWSMOB (scm_tc16_regex, rx);
}
#undef FUNC_NAME

/*
 * While regexec does respect the current locale, it returns byte
 * offsets instead of character offsets. This routine fixes up the
 * regmatch_t structures to refer to characters instead. See "Converting
 * a Character" in the libc manual, for more details.
 */
static void
fixup_multibyte_match (regmatch_t *matches, int nmatches, char *str)
{
  mbstate_t state;
  int i;
  size_t char_idx, byte_idx;
  size_t nbytes = 1; /* just to kick off the for loop */

  memset (&state, '\0', sizeof (state));

  for (char_idx = byte_idx = 0; nbytes > 0; char_idx++, byte_idx += nbytes)
    {
      for (i = 0; i < nmatches; ++i)
        {
          if (matches[i].rm_so == byte_idx)
            matches[i].rm_so = char_idx;
          if (matches[i].rm_eo == byte_idx)
            matches[i].rm_eo = char_idx;
        }

      nbytes = mbrlen (str + byte_idx, MB_LEN_MAX, &state);
      if (nbytes == (size_t) -2 || nbytes == (size_t) -1)
        /* Something is wrong. Shouldn't be possible, as the regex match
           succeeded.  */
        abort ();
    }

}

SCM_DEFINE (scm_regexp_exec, "regexp-exec", 2, 2, 0,
            (SCM rx, SCM str, SCM start, SCM flags),
	    "Match the compiled regular expression @var{rx} against\n"
	    "@code{str}.  If the optional integer @var{start} argument is\n"
	    "provided, begin matching from that position in the string.\n"
	    "Return a match structure describing the results of the match,\n"
	    "or @code{#f} if no match could be found.\n"
            "\n"
            "The @var{flags} arguments change the matching behavior.\n"
            "The following flags may be supplied:\n"
	    "\n"
	    "@table @code\n"
	    "@item regexp/notbol\n"
            "Operator @samp{^} always fails (unless @code{regexp/newline}\n"
            "is used).  Use this when the beginning of the string should\n"
            "not be considered the beginning of a line.\n"
	    "@item regexp/noteol\n"
            "Operator @samp{$} always fails (unless @code{regexp/newline}\n"
            "is used).  Use this when the end of the string should not be\n"
            "considered the end of a line.\n"
            "@end table")
#define FUNC_NAME s_scm_regexp_exec
{
  int status, nmatches, offset;
  regmatch_t *matches;
  char *c_str;
  SCM mvec = SCM_BOOL_F;
  SCM substr;

  SCM_VALIDATE_RGXP (1, rx);
  SCM_VALIDATE_STRING (2, str);

  if (SCM_UNBNDP (start))
    {
      substr = str;
      offset = 0;
    }
  else
    {
      substr = scm_substring (str, start, SCM_UNDEFINED);
      offset = scm_to_int (start);
    }

  if (SCM_UNBNDP (flags))
    flags = SCM_INUM0;

  /* re_nsub doesn't account for the `subexpression' representing the
     whole regexp, so add 1 to nmatches. */

  c_str = scm_to_locale_string (substr);

  nmatches = SCM_RGX(rx)->re_nsub + 1;
  matches = scm_malloc (sizeof (regmatch_t) * nmatches);
  status = regexec (SCM_RGX (rx), c_str, nmatches, matches,
		    scm_to_int (flags));

  if (!status)
    fixup_multibyte_match (matches, nmatches, c_str);

  free (c_str);

  if (!status)
    {
      int i;
      /* The match vector must include a cell for the string that was matched,
	 so add 1. */
      mvec = scm_c_make_vector (nmatches + 1, SCM_UNSPECIFIED);
      SCM_SIMPLE_VECTOR_SET(mvec,0, str);
      for (i = 0; i < nmatches; ++i)
	if (matches[i].rm_so == -1)
	  SCM_SIMPLE_VECTOR_SET(mvec, i+1,
			 scm_cons (scm_from_int (-1), scm_from_int (-1)));
	else
	  SCM_SIMPLE_VECTOR_SET(mvec, i+1,
			 scm_cons (scm_from_long (matches[i].rm_so + offset),
				   scm_from_long (matches[i].rm_eo + offset)));
    }
  free (matches);

  if (status != 0 && status != REG_NOMATCH)
    scm_error_scm (scm_regexp_error_key,
		   scm_from_utf8_string (FUNC_NAME),
		   scm_regexp_error_msg (status, SCM_RGX (rx)),
		   SCM_BOOL_F, SCM_BOOL_F);
  return mvec;
}
#undef FUNC_NAME

void
scm_init_regex_posix ()
{
  scm_tc16_regex = scm_make_smob_type ("regexp", sizeof (regex_t));
  scm_set_smob_free (scm_tc16_regex, regex_free);

  /* Compilation flags.  */
  scm_c_define ("regexp/basic",    scm_from_int (REG_BASIC));
  scm_c_define ("regexp/extended", scm_from_int (REG_EXTENDED));
  scm_c_define ("regexp/icase",    scm_from_int (REG_ICASE));
  scm_c_define ("regexp/newline",  scm_from_int (REG_NEWLINE));

  /* Execution flags.  */
  scm_c_define ("regexp/notbol", scm_from_int (REG_NOTBOL));
  scm_c_define ("regexp/noteol", scm_from_int (REG_NOTEOL));

#include "regex-posix.x"

  scm_add_feature ("regex");
}
