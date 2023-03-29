/* Copyright 1995-1997,1999-2001,2003-2004,2006-2012,2014-2021
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

#include <alloca.h>
#include <c-ctype.h>
#include <c-strcase.h>
#include <stdio.h>
#include <string.h>
#include <unicase.h>
#include <unictype.h>
#include <unistd.h>

#include "alist.h"
#include "arrays.h"
#include "bitvectors.h"
#include "boolean.h"
#include "bytevectors.h"
#include "chars.h"
#include "eq.h"
#include "eval.h"
#include "fluids.h"
#include "fports.h"
#include "gsubr.h"
#include "hash.h"
#include "hashtab.h"
#include "keywords.h"
#include "modules.h"
#include "numbers.h"
#include "pairs.h"
#include "ports-internal.h"
#include "ports.h"
#include "private-options.h"
#include "procs.h"
#include "srcprop.h"
#include "srfi-13.h"
#include "srfi-4.h"
#include "strings.h"
#include "strports.h"
#include "symbols.h"
#include "variable.h"
#include "vectors.h"

#include "read.h"




SCM_GLOBAL_SYMBOL (scm_sym_dot, ".");
SCM_SYMBOL (sym_nil, "nil");
SCM_SYMBOL (sym_ISO_8859_1, "ISO-8859-1");

scm_t_option scm_read_opts[] =
  {
    { SCM_OPTION_BOOLEAN, "positions", 1,
      "Record positions of source code expressions." },
    { SCM_OPTION_BOOLEAN, "case-insensitive", 0,
      "Convert symbols to lower case."},
    { SCM_OPTION_SCM, "keywords", (scm_t_bits) SCM_BOOL_F_BITS,
      "Style of keyword recognition: #f, 'prefix or 'postfix."},
    { SCM_OPTION_BOOLEAN, "r6rs-hex-escapes", 0,
      "Use R6RS variable-length character and string hex escapes."},
    { SCM_OPTION_BOOLEAN, "square-brackets", 1,
      "Treat `[' and `]' as parentheses, for R6RS compatibility."},
    { SCM_OPTION_BOOLEAN, "hungry-eol-escapes", 0,
      "In strings, consume leading whitespace after an escaped end-of-line."},
    { SCM_OPTION_BOOLEAN, "curly-infix", 0,
      "Support SRFI-105 curly infix expressions."},
    { SCM_OPTION_BOOLEAN, "r7rs-symbols", 0,
      "Support R7RS |...| symbol notation."},
    { 0, },
  };
 
void
scm_i_input_error (char const *function,
		   SCM port, const char *message, SCM arg)
{
  SCM fn = (scm_is_string (SCM_FILENAME(port))
	    ? SCM_FILENAME(port)
	    : scm_from_utf8_string ("#<unknown port>"));

  SCM string_port = scm_open_output_string ();
  SCM string = SCM_EOL;
  scm_simple_format (string_port,
		     scm_from_utf8_string ("~A:~S:~S: ~A"),
		     scm_list_4 (fn,
				 scm_sum (scm_port_line (port), SCM_INUM1),
				 scm_sum (scm_port_column (port), SCM_INUM1),
				 scm_from_utf8_string (message)));
    
  string = scm_get_output_string (string_port);
  scm_close_output_port (string_port);
  scm_error_scm (scm_from_utf8_symbol ("read-error"),
		 function? scm_from_utf8_string (function) : SCM_BOOL_F,
		 string,
		 arg,
		 SCM_BOOL_F);
}


SCM_DEFINE (scm_read_options, "read-options-interface", 0, 1, 0, 
            (SCM setting),
	    "Option interface for the read options. Instead of using\n"
	    "this procedure directly, use the procedures @code{read-enable},\n"
	    "@code{read-disable}, @code{read-set!} and @code{read-options}.")
#define FUNC_NAME s_scm_read_options
{
  return scm_options (setting, scm_read_opts, FUNC_NAME);
}
#undef FUNC_NAME


/* Token readers.  */


/* Size of the C buffer used to read symbols and numbers.  */
#define READER_BUFFER_SIZE            128

/* Number of 32-bit codepoints in the buffer used to read strings.  */
#define READER_STRING_BUFFER_SIZE     128

/* The maximum size of Scheme character names.  */
#define READER_CHAR_NAME_MAX_SIZE      50

/* The maximum size of reader directive names.  */
#define READER_DIRECTIVE_NAME_MAX_SIZE 50


/* `isblank' is only in C99.  */
#define CHAR_IS_BLANK_(_chr)					\
  (((_chr) == ' ') || ((_chr) == '\t') || ((_chr) == '\n')	\
   || ((_chr) == '\f') || ((_chr) == '\r'))

#ifdef MSDOS
# define CHAR_IS_BLANK(_chr)			\
  ((CHAR_IS_BLANK_ (chr)) || ((_chr) == 26))
#else
# define CHAR_IS_BLANK CHAR_IS_BLANK_
#endif


/* R5RS one-character delimiters (see section 7.1.1, ``Lexical
   structure'').  */
#define CHAR_IS_R5RS_DELIMITER(c)				\
  (CHAR_IS_BLANK (c)						\
   || (c) == ')' || (c) == '(' || (c) == ';' || (c) == '"')

#define CHAR_IS_DELIMITER(c)                                    \
  (CHAR_IS_R5RS_DELIMITER (c)                                   \
   || (((c) == ']' || (c) == '[')))

/* Exponent markers, as defined in section 7.1.1 of R5RS, ``Lexical
   Structure''.  */
#define CHAR_IS_EXPONENT_MARKER(_chr)				\
  (((_chr) == 'e') || ((_chr) == 's') || ((_chr) == 'f')	\
   || ((_chr) == 'd') || ((_chr) == 'l'))

/* Read an SCSH block comment.  */
static SCM scm_read_scsh_block_comment (scm_t_wchar, SCM);
static SCM scm_read_r6rs_block_comment (scm_t_wchar, SCM);
static SCM scm_read_commented_expression (scm_t_wchar, SCM);
static SCM scm_read_shebang (scm_t_wchar, SCM);
static SCM scm_get_hash_procedure (int);

/* Read from PORT until a delimiter (e.g., a whitespace) is read.  Put the
   result in the pre-allocated buffer BUF.  Return zero if the whole token has
   fewer than BUF_SIZE bytes, non-zero otherwise. READ will be set the number of
   bytes actually read.  */
static int
read_token (SCM port, char *buf, size_t buf_size, size_t *read)
{
   *read = 0;

   while (*read < buf_size)
     {
       int chr;

       chr = scm_get_byte_or_eof (port);

       if (chr == EOF)
        return 0;
      else if (CHAR_IS_DELIMITER (chr))
        {
          scm_unget_byte (chr, port);
          return 0;
        }
      else
        {
          *buf = (char) chr;
          buf++, (*read)++;
        }
     }

   return 1;
 }

/* Like `read_token', but return either BUFFER, or a GC-allocated buffer
   if the token doesn't fit in BUFFER_SIZE bytes.  */
static char *
read_complete_token (SCM port, char *buffer, size_t buffer_size, size_t *read)
{
  int overflow = 0;
  size_t bytes_read, overflow_size = 0;
  char *overflow_buffer = NULL;

  do
    {
      overflow = read_token (port, buffer, buffer_size, &bytes_read);
      if (bytes_read == 0)
        break;
      if (overflow || overflow_size != 0)
        {
          if (overflow_size == 0)
            {
              overflow_buffer = scm_gc_malloc_pointerless (bytes_read, "read");
              memcpy (overflow_buffer, buffer, bytes_read);
              overflow_size = bytes_read;
            }
          else
            {
	      char *new_buf =
		scm_gc_malloc_pointerless (overflow_size + bytes_read, "read");

	      memcpy (new_buf, overflow_buffer, overflow_size);
              memcpy (new_buf + overflow_size, buffer, bytes_read);

	      overflow_buffer = new_buf;
              overflow_size += bytes_read;
            }
        }
    }
  while (overflow);

  if (overflow_size)
    *read = overflow_size;
  else
    *read = bytes_read;

  return overflow_size > 0 ? overflow_buffer : buffer;
}

/* Skip whitespace from PORT and return the first non-whitespace character
   read.  Raise an error on end-of-file.  */
static int
flush_ws (SCM port, const char *eoferr)
{
  scm_t_wchar c;
  while (1)
    switch (c = scm_getc (port))
      {
      case EOF:
      goteof:
	if (eoferr)
	  {
	    scm_i_input_error (eoferr,
			       port,
			       "end of file",
			       SCM_EOL);
	  }
	return c;

      case ';':
      lp:
	switch (c = scm_getc (port))
	  {
	  case EOF:
	    goto goteof;
	  default:
	    goto lp;
	  case SCM_LINE_INCREMENTORS:
	    break;
	  }
	break;

      case '#':
	switch (c = scm_getc (port))
	  {
	  case EOF:
	    eoferr = "read_sharp";
	    goto goteof;
	  case '!':
	    scm_read_shebang (c, port);
	    break;
	  case ';':
	    scm_read_commented_expression (c, port);
	    break;
	  case '|':
	    if (scm_is_false (scm_get_hash_procedure (c)))
	      {
		scm_read_r6rs_block_comment (c, port);
		break;
	      }
	    /* fall through */
	  default:
	    scm_ungetc (c, port);
	    return '#';
	  }
	break;

      case SCM_LINE_INCREMENTORS:
      case SCM_SINGLE_SPACES:
      case '\t':
	break;

      default:
	return c;
      }

  return 0;
}



/* Token readers.  */

static SCM scm_read_expression (SCM port);
static SCM scm_read_sharp (int chr, SCM port);


static SCM
scm_read_sexp (scm_t_wchar chr, SCM port)
#define FUNC_NAME "scm_i_lreadparen"
{
  int c;
  SCM tmp, tl, ans = SCM_EOL;
  const int terminating_char = (chr == '[') ? ']' : ')';

  c = flush_ws (port, FUNC_NAME);
  if (terminating_char == c)
    return SCM_EOL;

  scm_ungetc (c, port);
  tmp = scm_read_expression (port);

  /* Note that it is possible for scm_read_expression to return
     scm_sym_dot, but not as part of a dotted pair: as in #{.}#.  So
     check that it's a real dot by checking `c'.  */
  if (c == '.' && scm_is_eq (scm_sym_dot, tmp))
    {
      ans = scm_read_expression (port);
      if (terminating_char != (c = flush_ws (port, FUNC_NAME)))
	scm_i_input_error (FUNC_NAME, port, "missing close paren",
			   SCM_EOL);
      return ans;
    }

  /* Build the head of the list structure. */
  ans = tl = scm_cons (tmp, SCM_EOL);

  while (terminating_char != (c = flush_ws (port, FUNC_NAME)))
    {
      SCM new_tail;

      if (c == ')' || c == ']')
        scm_i_input_error (FUNC_NAME, port,
                           "in pair: mismatched close paren: ~A",
                           scm_list_1 (SCM_MAKE_CHAR (c)));

      scm_ungetc (c, port);
      tmp = scm_read_expression (port);

      /* See above note about scm_sym_dot.  */
      if (c == '.' && scm_is_eq (scm_sym_dot, tmp))
	{
	  SCM_SETCDR (tl, scm_read_expression (port));

	  c = flush_ws (port, FUNC_NAME);
	  if (terminating_char != c)
	    scm_i_input_error (FUNC_NAME, port,
			       "in pair: missing close paren", SCM_EOL);
	  break;
	}

      new_tail = scm_cons (tmp, SCM_EOL);
      SCM_SETCDR (tl, new_tail);
      tl = new_tail;
    }

  return ans;
}
#undef FUNC_NAME


/* Read a hexadecimal number NDIGITS in length.  Put its value into the variable
   C.  If TERMINATOR is non-null, terminate early if the TERMINATOR character is
   found.  */
#define SCM_READ_HEX_ESCAPE(ndigits, terminator)                   \
  do                                                               \
    {                                                              \
      scm_t_wchar a;                                               \
      size_t i = 0;                                                \
      c = 0;                                                       \
      while (i < ndigits)                                          \
        {                                                          \
          a = scm_getc (port);                                     \
          if (a == EOF)                                            \
            goto str_eof;                                          \
          if (terminator                                           \
              && (a == (scm_t_wchar) terminator)                   \
              && (i > 0))                                          \
            break;                                                 \
          if ('0' <= a && a <= '9')                                \
            a -= '0';                                              \
          else if ('A' <= a && a <= 'F')                           \
            a = a - 'A' + 10;                                      \
          else if ('a' <= a && a <= 'f')                           \
            a = a - 'a' + 10;                                      \
          else                                                     \
            {                                                      \
              c = a;                                               \
              goto bad_escaped;                                    \
            }                                                      \
          c = c * 16 + a;                                          \
          i ++;                                                    \
        }                                                          \
    } while (0)

/* Read either a double-quoted string or an R7RS-style symbol delimited
   by vertical lines, depending on the value of 'chr' ('"' or '|').
   Regardless, the result is always returned as a string.  */
static SCM
scm_read_string_like_syntax (int chr, SCM port)
#define FUNC_NAME "scm_lreadr"
{
  /* For strings smaller than C_STR, this function creates only one Scheme
     object (the string returned).  */

  SCM str = SCM_EOL;
  size_t c_str_len = 0;
  scm_t_wchar c, c_str[READER_STRING_BUFFER_SIZE];

  while (chr != (c = scm_getc (port)))
    {
      if (c == EOF)
        {
        str_eof:
          scm_i_input_error (FUNC_NAME, port,
                             (chr == '|'
                              ? "end of file in symbol"
                              : "end of file in string constant"),
                             SCM_EOL);
        }

      if (c_str_len + 1 >= READER_STRING_BUFFER_SIZE)
	{
	  str = scm_cons (scm_from_utf32_stringn (c_str, c_str_len), str);
	  c_str_len = 0;
	}

      if (c == '\\')
        {
          switch (c = scm_getc (port))
            {
            case EOF:
              goto str_eof;
            case '|':
            case '\\':
            case '(':  /* Accept "\(" for use at the beginning of lines
			  in multiline strings to avoid confusing emacs
			  lisp modes.  */
              break;
            case '\n':
              continue;
            case '0':
              c = '\0';
              break;
            case 'f':
              c = '\f';
              break;
            case 'n':
              c = '\n';
              break;
            case 'r':
              c = '\r';
              break;
            case 't':
              c = '\t';
              break;
            case 'a':
              c = '\007';
              break;
            case 'v':
              c = '\v';
              break;
            case 'b':
              c = '\010';
              break;
            case 'x':
              SCM_READ_HEX_ESCAPE (2, '\0');
              break;
            case 'u':
              SCM_READ_HEX_ESCAPE (4, '\0');
              break;
            case 'U':
              SCM_READ_HEX_ESCAPE (6, '\0');
              break;
            default:
              if (c == chr)
                break;
            bad_escaped:
              scm_i_input_error (FUNC_NAME, port,
                                 "invalid character in escape sequence: ~S",
                                 scm_list_1 (SCM_MAKE_CHAR (c)));
            }
        }

      c_str[c_str_len++] = c;
    }

  if (scm_is_null (str))
    /* Fast path: we got a string that fits in C_STR.  */
    str = scm_from_utf32_stringn (c_str, c_str_len);
  else
    {
      if (c_str_len > 0)
	str = scm_cons (scm_from_utf32_stringn (c_str, c_str_len), str);

      str = scm_string_concatenate_reverse (str, SCM_UNDEFINED, SCM_UNDEFINED);
    }

  return str;
}
#undef FUNC_NAME

static SCM
scm_read_string (int chr, SCM port)
{
  return scm_read_string_like_syntax (chr, port);
}

static SCM
scm_read_number (scm_t_wchar chr, SCM port)
{
  SCM result, str = SCM_EOL;
  char local_buffer[READER_BUFFER_SIZE], *buffer;
  size_t bytes_read;

  scm_ungetc (chr, port);
  buffer = read_complete_token (port, local_buffer, sizeof local_buffer,
				&bytes_read);

  str = scm_from_port_stringn (buffer, bytes_read, port);

  result = scm_string_to_number (str, SCM_UNDEFINED);
  if (scm_is_false (result))
    {
      /* Return a symbol instead of a number */
      result = scm_string_to_symbol (str);
    }

  scm_set_port_column_x (port,
                         scm_sum (scm_port_column (port),
                                  scm_string_length (str)));
  return result;
}

static SCM
scm_read_mixed_case_symbol (scm_t_wchar chr, SCM port)
{
  SCM result;
  size_t bytes_read;
  char local_buffer[READER_BUFFER_SIZE], *buffer;
  SCM str;

  scm_ungetc (chr, port);
  buffer = read_complete_token (port, local_buffer, sizeof local_buffer,
				&bytes_read);

  str = scm_from_port_stringn (buffer, bytes_read, port);
  result = scm_string_to_symbol (str);

  scm_set_port_column_x (port,
                         scm_sum (scm_port_column (port),
                                  scm_string_length (str)));
  return result;
}

static SCM
scm_read_number_and_radix (scm_t_wchar chr, SCM port)
#define FUNC_NAME "scm_lreadr"
{
  SCM result;
  size_t read;
  char local_buffer[READER_BUFFER_SIZE], *buffer;
  unsigned int radix;
  SCM str;

  switch (chr)
    {
    case 'B':
    case 'b':
      radix = 2;
      break;

    case 'o':
    case 'O':
      radix = 8;
      break;

    case 'd':
    case 'D':
      radix = 10;
      break;

    case 'x':
    case 'X':
      radix = 16;
      break;

    default:
      scm_ungetc (chr, port);
      scm_ungetc ('#', port);
      radix = 10;
    }

  buffer = read_complete_token (port, local_buffer, sizeof local_buffer,
				&read);

  str = scm_from_port_stringn (buffer, read, port);

  result = scm_string_to_number (str, scm_from_uint (radix));

  scm_set_port_column_x (port,
                         scm_sum (scm_port_column (port),
                                  scm_string_length (str)));

  if (scm_is_true (result))
    return result;

  scm_i_input_error (FUNC_NAME, port, "unknown # object", SCM_EOL);

  return SCM_BOOL_F;
}
#undef FUNC_NAME

static SCM
scm_read_quote (int chr, SCM port)
{
  SCM p;

  switch (chr)
    {
    case '`':
      p = scm_sym_quasiquote;
      break;

    case '\'':
      p = scm_sym_quote;
      break;

    case ',':
      {
	scm_t_wchar c;

	c = scm_getc (port);
	if ('@' == c)
	  p = scm_sym_uq_splicing;
	else
	  {
	    scm_ungetc (c, port);
	    p = scm_sym_unquote;
	  }
	break;
      }

    default:
      fprintf (stderr, "%s: unhandled quote character (%i)\n",
	       "scm_read_quote", chr);
      abort ();
    }

  return scm_cons2 (p, scm_read_expression (port), SCM_EOL);
}

SCM_SYMBOL (sym_syntax, "syntax");
SCM_SYMBOL (sym_quasisyntax, "quasisyntax");
SCM_SYMBOL (sym_unsyntax, "unsyntax");
SCM_SYMBOL (sym_unsyntax_splicing, "unsyntax-splicing");

static SCM
scm_read_syntax (int chr, SCM port)
{
  SCM p;

  switch (chr)
    {
    case '`':
      p = sym_quasisyntax;
      break;

    case '\'':
      p = sym_syntax;
      break;

    case ',':
      {
	int c;

	c = scm_getc (port);
	if ('@' == c)
	  p = sym_unsyntax_splicing;
	else
	  {
	    scm_ungetc (c, port);
	    p = sym_unsyntax;
	  }
	break;
      }

    default:
      fprintf (stderr, "%s: unhandled syntax character (%i)\n",
	       "scm_read_syntax", chr);
      abort ();
    }

  return scm_cons2 (p, scm_read_expression (port), SCM_EOL);
}

static SCM
scm_read_nil (int chr, SCM port)
{
  SCM id = scm_read_mixed_case_symbol (chr, port);

  if (!scm_is_eq (id, sym_nil))
    scm_i_input_error ("scm_read_nil", port,
                       "unexpected input while reading #nil: ~a",
                       scm_list_1 (id));

  return SCM_ELISP_NIL;
}
  
static SCM
scm_read_semicolon_comment (int chr, SCM port)
{
  int c;

  /* We use the get_byte here because there is no need to get the
     locale correct with comment input. This presumes that newline
     always represents itself no matter what the encoding is.  */
  for (c = scm_get_byte_or_eof (port);
       (c != EOF) && (c != '\n');
       c = scm_get_byte_or_eof (port));

  return SCM_UNSPECIFIED;
}

/* If the EXPECTED_CHARS are the next ones available from PORT, then
   consume them and return 1.  Otherwise leave the port position where
   it was and return 0.  EXPECTED_CHARS should be all lowercase, and
   will be matched case-insensitively against the characters read from
   PORT. */
static int
try_read_ci_chars (SCM port, const char *expected_chars)
{
  int num_chars_wanted = strlen (expected_chars);
  int num_chars_read = 0;
  char *chars_read = alloca (num_chars_wanted);
  int c;

  while (num_chars_read < num_chars_wanted)
    {
      c = scm_getc (port);
      if (c == EOF)
        break;
      else if (c_tolower (c) != expected_chars[num_chars_read])
        {
          scm_ungetc (c, port);
          break;
        }
      else
        chars_read[num_chars_read++] = c;
    }

  if (num_chars_read == num_chars_wanted)
    return 1;
  else
    {
      while (num_chars_read > 0)
        scm_ungetc (chars_read[--num_chars_read], port);
      return 0;
    }
}


/* Sharp readers, i.e. readers called after a `#' sign has been read.  */

static SCM
scm_read_boolean (int chr, SCM port)
{
  switch (chr)
    {
    case 't':
    case 'T':
      try_read_ci_chars (port, "rue");
      return SCM_BOOL_T;

    case 'f':
    case 'F':
      try_read_ci_chars (port, "alse");
      return SCM_BOOL_F;
    }

  return SCM_UNSPECIFIED;
}

static SCM
scm_read_character (scm_t_wchar chr, SCM port)
#define FUNC_NAME "scm_lreadr"
{
  char buffer[READER_CHAR_NAME_MAX_SIZE];
  SCM charname;
  size_t charname_len, bytes_read;
  scm_t_wchar cp;
  int overflow;
  scm_t_port *pt;

  overflow = read_token (port, buffer, READER_CHAR_NAME_MAX_SIZE,
                         &bytes_read);
  if (overflow)
    scm_i_input_error (FUNC_NAME, port, "character name too long", SCM_EOL);

  if (bytes_read == 0)
    {
      chr = scm_getc (port);
      if (chr == EOF)
	scm_i_input_error (FUNC_NAME, port, "unexpected end of file "
			   "while reading character", SCM_EOL);

      /* CHR must be a token delimiter, like a whitespace.  */
      return SCM_MAKE_CHAR (chr);
    }

  pt = SCM_PORT (port);

  /* Simple ASCII characters can be processed immediately.  Also, simple
     ISO-8859-1 characters can be processed immediately if the encoding for this
     port is ISO-8859-1.  */
  if (bytes_read == 1 &&
      ((unsigned char) buffer[0] <= 127
       || scm_is_eq (pt->encoding, sym_ISO_8859_1)))
    {
      scm_set_port_column_x (port, scm_sum (scm_port_column (port), SCM_INUM1));
      return SCM_MAKE_CHAR (buffer[0]);
    }

  /* Otherwise, convert the buffer into a proper scheme string for
     processing.  */
  charname = scm_from_port_stringn (buffer, bytes_read, port);
  charname_len = scm_i_string_length (charname);
  scm_set_port_column_x (port,
                         scm_sum (scm_port_column (port),
                                  scm_from_size_t (charname_len)));
  cp = scm_i_string_ref (charname, 0);
  if (charname_len == 1)
    return SCM_MAKE_CHAR (cp);

  /* Ignore dotted circles, which may be used to keep combining characters from
     combining with the backslash in #\charname.  */
  if (cp == SCM_CODEPOINT_DOTTED_CIRCLE && charname_len == 2)
    return scm_c_make_char (scm_i_string_ref (charname, 1));

  if (cp >= '0' && cp < '8')
    {
      /* Dirk:FIXME::  This type of character syntax is not R5RS
       * compliant.  Further, it should be verified that the constant
       * does only consist of octal digits.  */
      SCM p = scm_string_to_number (charname, scm_from_uint (8));
      if (SCM_I_INUMP (p))
        {
          scm_t_wchar c = scm_to_uint32 (p);
          if (SCM_IS_UNICODE_CHAR (c))
            return SCM_MAKE_CHAR (c);
          else
            scm_i_input_error (FUNC_NAME, port,
                               "out-of-range octal character escape: ~a",
                               scm_list_1 (charname));
        }
    }

  if (cp == 'x' && (charname_len > 1))
    {
      SCM p;

      /* Convert from hex, skipping the initial 'x' character in CHARNAME */
      p = scm_string_to_number (scm_c_substring (charname, 1, charname_len),
                                scm_from_uint (16));
      if (SCM_I_INUMP (p))
        {
          scm_t_wchar c = scm_to_uint32 (p);
          if (SCM_IS_UNICODE_CHAR (c))
            return SCM_MAKE_CHAR (c);
          else
            scm_i_input_error (FUNC_NAME, port,
                               "out-of-range hex character escape: ~a",
                               scm_list_1 (charname));
        }
    }

  /* The names of characters should never have non-Latin1
     characters.  */
  if (scm_i_is_narrow_string (charname)
      || scm_i_try_narrow_string (charname))
    { SCM ch = scm_i_charname_to_char (scm_i_string_chars (charname),
                                       charname_len);
      if (scm_is_true (ch))
        return ch;
    }

  scm_i_input_error (FUNC_NAME, port, "unknown character name ~a",
		     scm_list_1 (charname));

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

static SCM
scm_read_keyword (int chr, SCM port)
{
  SCM symbol;

  /* Read the symbol that comprises the keyword.  Doing this instead of
     invoking a specific symbol reader function allows `scm_read_keyword ()'
     to adapt to the delimiters currently valid of symbols.

     XXX: This implementation allows sloppy syntaxes like `#:  key'.  */
  symbol = scm_read_expression (port);
  if (!scm_is_symbol (symbol))
    scm_i_input_error ("scm_read_keyword", port,
		       "keyword prefix `~a' not followed by a symbol: ~s",
		       scm_list_2 (SCM_MAKE_CHAR (chr), symbol));

  return scm_symbol_to_keyword (symbol);
}

static SCM
scm_read_vector (int chr, SCM port)
{
  /* Note: We call `scm_read_sexp ()' rather than READER here in order to
     guarantee that it's going to do what we want.  After all, this is an
     implementation detail of `scm_read_vector ()', not a desirable
     property.  */
  return scm_vector (scm_read_sexp (chr, port));
}

/* Helper used by scm_read_array */
static int
read_decimal_integer (SCM port, int c, ssize_t *resp)
{
  ssize_t sign = 1;
  ssize_t res = 0;
  int got_it = 0;

  if (c == '-')
    {
      sign = -1;
      c = scm_getc (port);
    }

  while ('0' <= c && c <= '9')
    {
      if (((SSIZE_MAX - (c-'0')) / 10) <= res)
        scm_i_input_error ("read_decimal_integer", port,
                           "number too large", SCM_EOL);
      res = 10*res + c-'0';
      got_it = 1;
      c = scm_getc (port);
    }

  if (got_it)
    *resp = sign * res;
  return c;
}

/* Read an array.  This function can also read vectors and uniform
   vectors.  Also, the conflict between '#f' and '#f32' and '#f64' is
   handled here.

   C is the first character read after the '#'. */
static SCM
scm_read_array (int c, SCM port)
{
  ssize_t rank;
  scm_t_wchar tag_buf[8];
  int tag_len;

  SCM tag, shape = SCM_BOOL_F, elements;

  /* XXX - shortcut for ordinary vectors.  Shouldn't be necessary but
     the array code can not deal with zero-length dimensions yet, and
     we want to allow zero-length vectors, of course. */
  if (c == '(')
    return scm_read_vector (c, port);

  /* Disambiguate between '#f' and uniform floating point vectors. */
  if (c == 'f')
    {
      c = scm_getc (port);
      if (c != '3' && c != '6')
	{
          if (c == 'a' && try_read_ci_chars (port, "lse"))
            return SCM_BOOL_F;
          else if (c != EOF)
            scm_ungetc (c, port);
	  return SCM_BOOL_F;
	}
      rank = 1;
      tag_buf[0] = 'f';
      tag_len = 1;
      goto continue_reading_tag;
    }

  /* Read rank. */
  rank = 1;
  c = read_decimal_integer (port, c, &rank);
  if (rank < 0)
    scm_i_input_error (NULL, port, "array rank must be non-negative",
		       SCM_EOL);

  /* Read tag. */
  tag_len = 0;
 continue_reading_tag:
  while (c != EOF && c != '(' && c != '@' && c != ':'
         && tag_len < sizeof tag_buf / sizeof tag_buf[0])
    {
      tag_buf[tag_len++] = c;
      c = scm_getc (port);
    }
  if (tag_len == 0)
    tag = SCM_BOOL_T;
  else
    {
      tag = scm_string_to_symbol (scm_from_utf32_stringn (tag_buf, tag_len));
      if (tag_len == sizeof tag_buf / sizeof tag_buf[0])
        scm_i_input_error (NULL, port, "invalid array tag, starting with: ~a",
                           scm_list_1 (tag));
    }

  /* Read shape. */
  if (c == '@' || c == ':')
    {
      shape = SCM_EOL;

      do
	{
	  ssize_t lbnd = 0, len = 0;
	  SCM s;

	  if (c == '@')
	    {
	      c = scm_getc (port);
	      c = read_decimal_integer (port, c, &lbnd);
	    }

	  s = scm_from_ssize_t (lbnd);

	  if (c == ':')
	    {
	      c = scm_getc (port);
	      c = read_decimal_integer (port, c, &len);
	      if (len < 0)
		scm_i_input_error (NULL, port,
				   "array length must be non-negative",
				   SCM_EOL);

	      s = scm_list_2 (s, scm_from_ssize_t (lbnd+len-1));
	    }

	  shape = scm_cons (s, shape);
	} while (c == '@' || c == ':');

      shape = scm_reverse_x (shape, SCM_EOL);
    }

  /* Read nested lists of elements. */
  if (c != '(')
    scm_i_input_error (NULL, port,
		       "missing '(' in vector or array literal",
		       SCM_EOL);
  elements = scm_read_sexp (c, port);

  if (scm_is_false (shape))
    shape = scm_from_ssize_t (rank);
  else if (scm_ilength (shape) != rank)
    scm_i_input_error
      (NULL, port,
       "the number of shape specifications must match the array rank",
       SCM_EOL);

  /* Handle special print syntax of rank zero arrays; see
     scm_i_print_array for a rationale. */
  if (rank == 0)
    {
      if (!scm_is_pair (elements))
	scm_i_input_error (NULL, port,
			   "too few elements in array literal, need 1",
			   SCM_EOL);
      if (!scm_is_null (SCM_CDR (elements)))
	scm_i_input_error (NULL, port,
			   "too many elements in array literal, want 1",
			   SCM_EOL);
      elements = SCM_CAR (elements);
    }

  /* Construct array, annotate with source location, and return. */
  return scm_list_to_typed_array (tag, shape, elements);
}

static SCM
scm_read_srfi4_vector (int chr, SCM port)
{
  return scm_read_array (chr, port);
}

static SCM
scm_read_bytevector (scm_t_wchar chr, SCM port)
{
  chr = scm_getc (port);
  if (chr != 'u')
    goto syntax;

  chr = scm_getc (port);
  if (chr != '8')
    goto syntax;

  chr = scm_getc (port);
  if (chr != '(')
    goto syntax;

  return scm_u8_list_to_bytevector (scm_read_sexp (chr, port));

 syntax:
  scm_i_input_error ("read_bytevector", port,
		     "invalid bytevector prefix",
		     SCM_MAKE_CHAR (chr));
  return SCM_UNSPECIFIED;
}

static SCM
scm_read_guile_bit_vector (scm_t_wchar chr, SCM port)
{
  /* Read the `#*10101'-style read syntax for bit vectors in Guile.  This is
     terribly inefficient but who cares?  */
  SCM s_bits = SCM_EOL;

  for (chr = scm_getc (port);
       (chr != EOF) && ((chr == '0') || (chr == '1'));
       chr = scm_getc (port))
    {
      s_bits = scm_cons ((chr == '0') ? SCM_BOOL_F : SCM_BOOL_T, s_bits);
    }

  if (chr != EOF)
    scm_ungetc (chr, port);

  return scm_list_to_bitvector (scm_reverse_x (s_bits, SCM_EOL));
}

static SCM
scm_read_scsh_block_comment (scm_t_wchar chr, SCM port)
{
  int bang_seen = 0;

  for (;;)
    {
      int c = scm_getc (port);

      if (c == EOF)
	scm_i_input_error ("skip_block_comment", port,
			   "unterminated `#! ... !#' comment", SCM_EOL);

      if (c == '!')
	bang_seen = 1;
      else if (c == '#' && bang_seen)
	break;
      else
	bang_seen = 0;
    }

  return SCM_UNSPECIFIED;
}

static SCM
scm_read_shebang (scm_t_wchar chr, SCM port)
{
  char name[READER_DIRECTIVE_NAME_MAX_SIZE + 1];
  int c;
  int i = 0;

  while (i <= READER_DIRECTIVE_NAME_MAX_SIZE)
    {
      c = scm_getc (port);
      if (c == EOF)
	scm_i_input_error ("skip_block_comment", port,
			   "unterminated `#! ... !#' comment", SCM_EOL);
      else if (('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '-')
        name[i++] = c;
      else if (CHAR_IS_DELIMITER (c))
        {
          scm_ungetc (c, port);
          name[i] = '\0';
          if (0 == strcmp ("r6rs", name)
              || 0 == strcmp ("fold-case", name)
              || 0 == strcmp ("no-fold-case", name)
              || 0 == strcmp ("curly-infix", name)
              || 0 == strcmp ("curly-infix-and-bracket-lists", name))
            scm_i_input_error ("skip_block_comment", port,
                               "unsupported directive: ~s",
                               scm_list_1 (scm_from_latin1_string (name)));
          break;
        }
      else
        {
          scm_ungetc (c, port);
          break;
        }
    }
  while (i > 0)
    scm_ungetc (name[--i], port);
  return scm_read_scsh_block_comment (chr, port);
}

static SCM
scm_read_r6rs_block_comment (scm_t_wchar chr, SCM port)
{
  /* Unlike SCSH-style block comments, SRFI-30/R6RS block comments may be
     nested.  So care must be taken.  */
  int nesting_level = 1;

  int a = scm_getc (port);

  if (a == EOF)
    scm_i_input_error ("scm_read_r6rs_block_comment", port,
                       "unterminated `#| ... |#' comment", SCM_EOL);

  while (nesting_level > 0)
    {
      int b = scm_getc (port);

      if (b == EOF)
	scm_i_input_error ("scm_read_r6rs_block_comment", port,
			   "unterminated `#| ... |#' comment", SCM_EOL);

      if (a == '|' && b == '#')
        {
          nesting_level--;
          b = EOF;
        }
      else if (a == '#' && b == '|')
        {
          nesting_level++;
          b = EOF;
        }

      a = b;
    }

  return SCM_UNSPECIFIED;
}

static SCM
scm_read_commented_expression (scm_t_wchar chr, SCM port)
{
  scm_t_wchar c;
  
  c = flush_ws (port, (char *) NULL);
  if (EOF == c)
    scm_i_input_error ("read_commented_expression", port,
                       "no expression after #; comment", SCM_EOL);
  scm_ungetc (c, port);
  scm_read_expression (port);
  return SCM_UNSPECIFIED;
}

static SCM
scm_read_extended_symbol (scm_t_wchar chr, SCM port)
{
  /* Guile's extended symbol read syntax looks like this:

       #{This is all a symbol name}#

     So here, CHR is expected to be `{'.  */
  int saw_brace = 0;
  size_t len = 0;
  SCM buf = scm_i_make_string (1024, NULL, 0);

  /* No need to scm_i_string_start_writing (), as the string isn't
     visible to any other thread.  */

  while ((chr = scm_getc (port)) != EOF)
    {
      if (saw_brace)
	{
	  if (chr == '#')
	    {
	      break;
	    }
	  else
	    {
	      saw_brace = 0;
	      scm_i_string_set_x (buf, len++, '}');
	    }
	}

      if (chr == '}')
	saw_brace = 1;
      else if (chr == '\\')
        {
          /* It used to be that print.c would print extended-read-syntax
             symbols with backslashes before "non-standard" chars, but
             this routine wouldn't do anything with those escapes.
             Bummer.  What we've done is to change print.c to output
             R6RS hex escapes for those characters, relying on the fact
             that the extended read syntax would never put a `\' before
             an `x'.  For now, we just ignore other instances of
             backslash in the string.  */
          switch ((chr = scm_getc (port)))
            {
            case EOF:
              goto done;
            case 'x':
              {
                scm_t_wchar c;
                
                SCM_READ_HEX_ESCAPE (10, ';');
                scm_i_string_set_x (buf, len++, c);
                break;

              str_eof:
                chr = EOF;
                goto done;

              bad_escaped:
                scm_i_string_stop_writing ();
                scm_i_input_error ("scm_read_extended_symbol", port,
                                   "illegal character in escape sequence: ~S",
                                   scm_list_1 (SCM_MAKE_CHAR (c)));
                break;
              }
            default:
	      scm_i_string_set_x (buf, len++, chr);
              break;
            }
        }
      else
        scm_i_string_set_x (buf, len++, chr);

      if (len >= scm_i_string_length (buf) - 2)
	{
	  SCM addy;

	  addy = scm_i_make_string (1024, NULL, 0);
	  buf = scm_string_append (scm_list_2 (buf, addy));
	  len = 0;
	}
    }

 done:
  if (chr == EOF)
    scm_i_input_error ("scm_read_extended_symbol", port,
                       "end of file while reading symbol", SCM_EOL);

  return scm_string_to_symbol (scm_c_substring (buf, 0, len));
}



/* Top-level token readers, i.e., dispatchers.  */

/* The reader for the sharp `#' character.  It basically dispatches reads
   among the above token readers.   */
static SCM
scm_read_sharp (scm_t_wchar chr, SCM port)
#define FUNC_NAME "scm_lreadr"
{
  chr = scm_getc (port);

  switch (chr)
    {
    case '\\':
      return scm_read_character (chr, port);
    case '(':
      return scm_read_vector (chr, port);
    case 's':
    case 'u':
    case 'f':
    case 'c':
      /* This one may return either a boolean or an SRFI-4 vector.  */
      return scm_read_srfi4_vector (chr, port);
    case 'v':
      return scm_read_bytevector (chr, port);
    case '*':
      return scm_read_guile_bit_vector (chr, port);
    case 't':
    case 'T':
    case 'F':
      return scm_read_boolean (chr, port);
    case ':':
      return scm_read_keyword (chr, port);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '@':
      return scm_read_array (chr, port);

    case 'i':
    case 'e':
    case 'b':
    case 'B':
    case 'o':
    case 'O':
    case 'd':
    case 'D':
    case 'x':
    case 'X':
    case 'I':
    case 'E':
      return scm_read_number_and_radix (chr, port);
    case '{':
      return scm_read_extended_symbol (chr, port);
    case '!':
      return scm_read_shebang (chr, port);
    case ';':
      return scm_read_commented_expression (chr, port);
    case '`':
    case '\'':
    case ',':
      return scm_read_syntax (chr, port);
    case 'n':
      return scm_read_nil (chr, port);
    case '|':
      return scm_read_r6rs_block_comment (chr, port);
    default:
      scm_i_input_error (FUNC_NAME, port, "Unknown # object: ~S",
                         scm_list_1 (SCM_MAKE_CHAR (chr)));
      return SCM_UNSPECIFIED;
    }
}
#undef FUNC_NAME

static SCM
scm_read_expression (SCM port)
#define FUNC_NAME "scm_read_expression"
{
  while (1)
    {
      scm_t_wchar chr;

      chr = scm_getc (port);

      switch (chr)
	{
	case SCM_WHITE_SPACES:
	case SCM_LINE_INCREMENTORS:
	  break;
	case ';':
	  scm_read_semicolon_comment (chr, port);
	  break;
	case '[':
	case '(':
          return scm_read_sexp (chr, port);
	case '"':
	  return scm_read_string (chr, port);
	case '\'':
	case '`':
	case ',':
	  return scm_read_quote (chr, port);
	case '#':
	  {
	    SCM result = scm_read_sharp (chr, port);
	    if (scm_is_eq (result, SCM_UNSPECIFIED))
	      /* We read a comment or some such.  */
	      break;
	    else
	      return result;
	  }
	case ')':
	  scm_i_input_error (FUNC_NAME, port, "unexpected \")\"", SCM_EOL);
	  break;
	case ']':
          scm_i_input_error (FUNC_NAME, port, "unexpected \"]\"", SCM_EOL);
          break;
	case EOF:
	  return SCM_EOF_VAL;

	default:
	  {
	    if (((chr >= '0') && (chr <= '9'))
		|| (strchr ("+-.", chr)))
	      return scm_read_number (chr, port);
	    else
	      return scm_read_mixed_case_symbol (chr, port);
	  }
	}
    }
}
#undef FUNC_NAME


/* Actual reader.  */

SCM_DEFINE (scm_primitive_read, "primitive-read", 0, 1, 0,
            (SCM port),
	    "Read an s-expression from the input port @var{port}, or from\n"
	    "the current input port if @var{port} is not specified.\n"
	    "Any whitespace before the next token is discarded.")
#define FUNC_NAME s_scm_primitive_read
{
  int c;

  if (SCM_UNBNDP (port))
    port = scm_current_input_port ();
  SCM_VALIDATE_OPINPORT (1, port);

  c = flush_ws (port, (char *) NULL);
  if (EOF == c)
    return SCM_EOF_VAL;
  scm_ungetc (c, port);

  return scm_read_expression (port);
}
#undef FUNC_NAME

static SCM scm_read_var;

SCM
scm_read (SCM port)
#define FUNC_NAME "read"
{
  if (SCM_UNBNDP (port))
    return scm_call_0 (scm_variable_ref (scm_read_var));

  return scm_call_1 (scm_variable_ref (scm_read_var), port);
}
#undef FUNC_NAME




/* A fluid referring to an association list mapping extra hash
   characters to procedures.  */
static SCM *scm_i_read_hash_procedures;

static SCM
scm_i_read_hash_procedures_ref (void)
{
  return scm_fluid_ref (*scm_i_read_hash_procedures);
}

static void
scm_i_read_hash_procedures_set_x (SCM value)
{
  scm_fluid_set_x (*scm_i_read_hash_procedures, value);
}

/* Manipulate the read-hash-procedures alist.  This could be written in
   Scheme, but maybe it will also be used by C code during initialisation.  */
SCM_DEFINE (scm_read_hash_extend, "read-hash-extend", 2, 0, 0,
            (SCM chr, SCM proc),
	    "Install the procedure @var{proc} for reading expressions\n"
	    "starting with the character sequence @code{#} and @var{chr}.\n"
	    "@var{proc} will be called with two arguments:  the character\n"
	    "@var{chr} and the port to read further data from. The object\n"
	    "returned will be the return value of @code{read}. \n"
	    "Passing @code{#f} for @var{proc} will remove a previous setting. \n"
	    )
#define FUNC_NAME s_scm_read_hash_extend
{
  SCM this;
  SCM prev;

  SCM_VALIDATE_CHAR (1, chr);
  SCM_ASSERT (scm_is_false (proc)
	      || scm_is_eq (scm_procedure_p (proc), SCM_BOOL_T),
	      proc, SCM_ARG2, FUNC_NAME);

  /* Check if chr is already in the alist.  */
  this = scm_i_read_hash_procedures_ref ();
  prev = SCM_BOOL_F;
  while (1)
    {
      if (scm_is_null (this))
	{
	  /* not found, so add it to the beginning.  */
	  if (scm_is_true (proc))
	    {
              SCM new = scm_cons (scm_cons (chr, proc),
                                  scm_i_read_hash_procedures_ref ());
	      scm_i_read_hash_procedures_set_x (new);
	    }
	  break;
	}
      if (scm_is_eq (chr, SCM_CAAR (this)))
	{
	  /* already in the alist.  */
	  if (scm_is_false (proc))
	    {
	      /* remove it.  */
	      if (scm_is_false (prev))
		{
                  SCM rest = SCM_CDR (scm_i_read_hash_procedures_ref ());
		  scm_i_read_hash_procedures_set_x (rest);
		}
	      else
		scm_set_cdr_x (prev, SCM_CDR (this));
	    }
	  else
	    {
	      /* replace it.  */
	      scm_set_cdr_x (SCM_CAR (this), proc);
	    }
	  break;
	}
      prev = this;
      this = SCM_CDR (this);
    }

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

/* Recover the read-hash procedure corresponding to char c.  */
static SCM
scm_get_hash_procedure (int c)
{
  SCM rest = scm_i_read_hash_procedures_ref ();

  while (1)
    {
      if (scm_is_null (rest))
	return SCM_BOOL_F;
  
      if (SCM_CHAR (SCM_CAAR (rest)) == c)
	return SCM_CDAR (rest);
     
      rest = SCM_CDR (rest);
    }
}

static int
is_encoding_char (char c)
{
  if (c >= 'a' && c <= 'z') return 1;
  if (c >= 'A' && c <= 'Z') return 1;
  if (c >= '0' && c <= '9') return 1;
  return strchr ("_-.:/,+=()", c) != NULL;
}

/* Maximum size of an encoding name.  This is a bit more than the
   longest name listed at
   <http://www.iana.org/assignments/character-sets> ("ISO-2022-JP-2", 13
   characters.)  */
#define ENCODING_NAME_MAX_SIZE 20

/* Number of bytes at the beginning or end of a file that are scanned
   for a "coding:" declaration.  */
#define SCM_ENCODING_SEARCH_SIZE (500 + ENCODING_NAME_MAX_SIZE)


/* Search the SCM_ENCODING_SEARCH_SIZE bytes of a file for an Emacs-like
   coding declaration.  Returns either NULL or a string whose storage
   has been allocated with `scm_gc_malloc'.  */
char *
scm_i_scan_for_encoding (SCM port)
{
  scm_t_port *pt;
  SCM buf;
  char header[SCM_ENCODING_SEARCH_SIZE+1];
  size_t cur, bytes_read, encoding_length, i;
  char *encoding = NULL;
  char *pos, *encoding_start;
  int in_comment;

  pt = SCM_PORT (port);
  buf = pt->read_buf;

  if (pt->rw_random)
    scm_flush (port);

  if (scm_port_buffer_can_take (buf, &cur) == 0)
    {
      /* We can use the read buffer, and thus avoid a seek. */
      buf = scm_fill_input (port, 0, &cur, &bytes_read);
      if (bytes_read > SCM_ENCODING_SEARCH_SIZE)
        bytes_read = SCM_ENCODING_SEARCH_SIZE;

      if (bytes_read <= 1)
        /* An unbuffered port -- don't scan.  */
        return NULL;

      memcpy (header, scm_port_buffer_take_pointer (buf, cur), bytes_read);
      header[bytes_read] = '\0';
    }
  else if (pt->rw_random)
    {
      /* The port is seekable.  This is OK but grubbing in the read
         buffer is better, so this case is just a fallback.  */
      bytes_read = scm_c_read (port, header, SCM_ENCODING_SEARCH_SIZE);
      header[bytes_read] = '\0';
      scm_seek (port, scm_from_int (0), scm_from_int (SEEK_SET));
    }
  else
    /* No input available and not seekable; scan fails.  */
    return NULL;

  /* search past "coding[:=]" */
  pos = header;
  while (1)
    {
      if ((pos = strstr(pos, "coding")) == NULL)
        return NULL;

      pos += strlen ("coding");
      if (pos - header >= SCM_ENCODING_SEARCH_SIZE ||
          (*pos == ':' || *pos == '='))
        {
          pos ++;
          break;
        }
    }

  /* skip spaces */
  while (pos - header <= SCM_ENCODING_SEARCH_SIZE &&
	 (*pos == ' ' || *pos == '\t'))
    pos ++;

  if (pos - header >= SCM_ENCODING_SEARCH_SIZE - ENCODING_NAME_MAX_SIZE)
    /* We found the "coding:" string, but there is probably not enough
       room to store an encoding name in its entirety, so ignore it.
       This makes sure we do not end up returning a truncated encoding
       name.  */
    return NULL;

  /* grab the next token */
  encoding_start = pos;
  i = 0;
  while (encoding_start + i - header <= SCM_ENCODING_SEARCH_SIZE
         && encoding_start + i - header < bytes_read
	 && is_encoding_char (encoding_start[i]))
    i++;

  encoding_length = i;
  if (encoding_length == 0)
    return NULL;

  encoding = scm_gc_strndup (encoding_start, encoding_length, "encoding");

  /* push backwards to make sure we were in a comment */
  in_comment = 0;
  pos = encoding_start;
  while (pos >= header)
    {
      if (*pos == ';')
	{
	  in_comment = 1;
	  break;
	}
      else if (*pos == '\n' || pos == header)
	{
	  /* This wasn't in a semicolon comment. Check for a
	   hash-bang comment. */
	  char *beg = strstr (header, "#!");
	  char *end = strstr (header, "!#");
	  if (beg < encoding_start && encoding_start + encoding_length <= end)
	    in_comment = 1;
	  break;
	}
      else
        {
          pos --;
          continue;
        }
    }
  if (!in_comment)
    /* This wasn't in a comment */
    return NULL;

  return encoding;
}

SCM_DEFINE (scm_file_encoding, "file-encoding", 1, 0, 0,
            (SCM port),
            "Scans the port for an Emacs-like character coding declaration\n"
            "near the top of the contents of a port with random-accessible contents.\n"
            "The coding declaration is of the form\n"
            "@code{coding: XXXXX} and must appear in a scheme comment.\n"
            "\n"
            "Returns a string containing the character encoding of the file\n"
            "if a declaration was found, or @code{#f} otherwise.\n")
#define FUNC_NAME s_scm_file_encoding
{
  char *enc;
  SCM s_enc;

  SCM_VALIDATE_OPINPORT (SCM_ARG1, port);

  enc = scm_i_scan_for_encoding (port);
  if (enc == NULL)
    return SCM_BOOL_F;
  else
    {
      /* It's not obvious what encoding to use here, but latin1 has the
         advantage of never causing a decoding error, and a valid
         encoding name should be ASCII anyway. */
      s_enc = scm_string_upcase (scm_from_latin1_string (enc));
      return s_enc;
    }

  return SCM_BOOL_F;
}
#undef FUNC_NAME



void
scm_init_read ()
{
  SCM read_hash_procs;

  read_hash_procs = scm_make_fluid_with_default (SCM_EOL);
  
  scm_i_read_hash_procedures =
    SCM_VARIABLE_LOC (scm_c_define ("%read-hash-procedures", read_hash_procs));

  scm_init_opts (scm_read_options, scm_read_opts);
#include "read.x"

  scm_read_var = scm_c_define
    ("read", scm_variable_ref (scm_c_lookup (s_scm_primitive_read)));
}
