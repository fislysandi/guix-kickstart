/* Copyright 2010-2016,2018
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <alignof.h>
#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <ffi.h>

#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
#include <complex.h>
#endif

#include "boolean.h"
#include "bytevectors.h"
#include "dynwind.h"
#include "eq.h"
#include "eval.h"
#include "extensions.h"
#include "finalizers.h"
#include "gsubr.h"
#include "instructions.h"
#include "intrinsics.h"
#include "keywords.h"
#include "list.h"
#include "modules.h"
#include "numbers.h"
#include "pairs.h"
#include "ports.h"
#include "stacks.h"
#include "symbols.h"
#include "threads.h"
#include "weak-table.h"
#include "version.h"

#include "foreign.h"



/* Return the first integer greater than or equal to LEN such that
   LEN % ALIGN == 0.  Return LEN if ALIGN is zero.  */
#define ROUND_UP(len, align)					\
  ((align) ? (((len) - 1UL) | ((align) - 1UL)) + 1UL : (len))

SCM_SYMBOL (sym_void, "void");
SCM_SYMBOL (sym_float, "float");
SCM_SYMBOL (sym_double, "double");
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
SCM_SYMBOL (sym_complex_float, "complex-float");
SCM_SYMBOL (sym_complex_double, "complex-double");
#endif
SCM_SYMBOL (sym_uint8, "uint8");
SCM_SYMBOL (sym_int8, "int8");
SCM_SYMBOL (sym_uint16, "uint16");
SCM_SYMBOL (sym_int16, "int16");
SCM_SYMBOL (sym_uint32, "uint32");
SCM_SYMBOL (sym_int32, "int32");
SCM_SYMBOL (sym_uint64, "uint64");
SCM_SYMBOL (sym_int64, "int64");
SCM_SYMBOL (sym_short, "short");
SCM_SYMBOL (sym_int, "int");
SCM_SYMBOL (sym_long, "long");
SCM_SYMBOL (sym_unsigned_short, "unsigned-short");
SCM_SYMBOL (sym_unsigned_int, "unsigned-int");
SCM_SYMBOL (sym_unsigned_long, "unsigned-long");
SCM_SYMBOL (sym_size_t, "size_t");
SCM_SYMBOL (sym_ssize_t, "ssize_t");
SCM_SYMBOL (sym_ptrdiff_t, "ptrdiff_t");
SCM_SYMBOL (sym_intptr_t, "intptr_t");
SCM_SYMBOL (sym_uintptr_t, "uintptr_t");

/* that's for pointers, you know. */
SCM_SYMBOL (sym_asterisk, "*");

SCM_SYMBOL (sym_null, "%null-pointer");
SCM_SYMBOL (sym_null_pointer_error, "null-pointer-error");

/* The cell representing the null pointer.  */
static SCM null_pointer;


/* Raise a null pointer dereference error.  */
static void
null_pointer_error (const char *func_name)
{
  scm_error (sym_null_pointer_error, func_name,
	     "null pointer dereference", SCM_EOL, SCM_EOL);
}


static SCM cif_to_procedure (SCM cif, SCM func_ptr, int with_errno);


static SCM pointer_weak_refs = SCM_BOOL_F;


static void
register_weak_reference (SCM from, SCM to)
{
  scm_weak_table_putq_x (pointer_weak_refs, from, to);
}

static void
pointer_finalizer_trampoline (void *ptr, void *data)
{
  scm_t_pointer_finalizer finalizer = data;
  finalizer (SCM_POINTER_VALUE (SCM_PACK_POINTER (ptr)));
}

SCM_DEFINE (scm_pointer_p, "pointer?", 1, 0, 0,
	    (SCM obj),
	    "Return @code{#t} if @var{obj} is a pointer object, "
	    "@code{#f} otherwise.\n")
#define FUNC_NAME s_scm_pointer_p
{
  return scm_from_bool (SCM_POINTER_P (obj));
}
#undef FUNC_NAME

SCM_DEFINE (scm_make_pointer, "make-pointer", 1, 1, 0,
	    (SCM address, SCM finalizer),
	    "Return a foreign pointer object pointing to @var{address}. "
	    "If @var{finalizer} is passed, it should be a pointer to a "
	    "one-argument C function that will be called when the pointer "
	    "object becomes unreachable.")
#define FUNC_NAME s_scm_make_pointer
{
  void *c_finalizer;
  uintptr_t c_address;

  c_address = scm_to_uintptr_t (address);
  if (SCM_UNBNDP (finalizer))
    c_finalizer = NULL;
  else
    {
      SCM_VALIDATE_POINTER (2, finalizer);
      c_finalizer = SCM_POINTER_VALUE (finalizer);
    }

  return scm_from_pointer ((void *) c_address, c_finalizer);
}
#undef FUNC_NAME

void *
scm_to_pointer (SCM pointer)
#define FUNC_NAME "scm_to_pointer"
{
  SCM_VALIDATE_POINTER (1, pointer);
  return SCM_POINTER_VALUE (pointer);
}
#undef FUNC_NAME

SCM
scm_from_pointer (void *ptr, scm_t_pointer_finalizer finalizer)
{
  SCM ret;

  if (ptr == NULL && finalizer == NULL)
    ret = null_pointer;
  else
    {
      ret = scm_cell (scm_tc7_pointer, (scm_t_bits) ptr);

      if (finalizer)
        scm_i_set_finalizer (SCM2PTR (ret), pointer_finalizer_trampoline,
                             finalizer);
    }

  return ret;
}

SCM_DEFINE (scm_pointer_address, "pointer-address", 1, 0, 0,
	    (SCM pointer),
	    "Return the numerical value of @var{pointer}.")
#define FUNC_NAME s_scm_pointer_address
{
  SCM_VALIDATE_POINTER (1, pointer);

  return scm_from_uintptr_t ((uintptr_t) SCM_POINTER_VALUE (pointer));
}
#undef FUNC_NAME

SCM_DEFINE (scm_pointer_to_scm, "pointer->scm", 1, 0, 0,
	    (SCM pointer),
	    "Unsafely cast @var{pointer} to a Scheme object.\n"
	    "Cross your fingers!")
#define FUNC_NAME s_scm_pointer_to_scm
{
  SCM_VALIDATE_POINTER (1, pointer);
  
  return SCM_PACK ((scm_t_bits) SCM_POINTER_VALUE (pointer));
}
#undef FUNC_NAME

SCM_DEFINE (scm_scm_to_pointer, "scm->pointer", 1, 0, 0,
	    (SCM scm),
	    "Return a foreign pointer object with the @code{object-address}\n"
            "of @var{scm}.")
#define FUNC_NAME s_scm_scm_to_pointer
{
  SCM ret;

  ret = scm_from_pointer ((void*) SCM_UNPACK (scm), NULL);
  if (SCM_HEAP_OBJECT_P (ret))
    register_weak_reference (ret, scm);

  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_pointer_to_bytevector, "pointer->bytevector", 2, 2, 0,
	    (SCM pointer, SCM len, SCM offset, SCM uvec_type),
	    "Return a bytevector aliasing the @var{len} bytes pointed\n"
	    "to by @var{pointer}.\n\n"
            "The user may specify an alternate default interpretation for\n"
            "the memory by passing the @var{uvec_type} argument, to indicate\n"
            "that the memory is an array of elements of that type.\n"
            "@var{uvec_type} should be something that\n"
            "@code{uniform-vector-element-type} would return, like @code{f32}\n"
            "or @code{s16}.\n\n"
	    "When @var{offset} is passed, it specifies the offset in bytes\n"
	    "relative to @var{pointer} of the memory region aliased by the\n"
	    "returned bytevector.")
#define FUNC_NAME s_scm_pointer_to_bytevector
{
  SCM ret;
  int8_t *ptr;
  size_t boffset, blen;
  scm_t_array_element_type btype;

  SCM_VALIDATE_POINTER (1, pointer);
  ptr = SCM_POINTER_VALUE (pointer);

  if (SCM_UNLIKELY (ptr == NULL))
    null_pointer_error (FUNC_NAME);

  if (SCM_UNBNDP (uvec_type))
    btype = SCM_ARRAY_ELEMENT_TYPE_VU8;
  else
    {
      int i;
      for (i = 0; i <= SCM_ARRAY_ELEMENT_TYPE_LAST; i++)
        if (scm_is_eq (uvec_type, scm_i_array_element_types[i]))
          break;
      switch (i)
        {
        case SCM_ARRAY_ELEMENT_TYPE_VU8:
        case SCM_ARRAY_ELEMENT_TYPE_U8:
        case SCM_ARRAY_ELEMENT_TYPE_S8:
        case SCM_ARRAY_ELEMENT_TYPE_U16:
        case SCM_ARRAY_ELEMENT_TYPE_S16:
        case SCM_ARRAY_ELEMENT_TYPE_U32:
        case SCM_ARRAY_ELEMENT_TYPE_S32:
        case SCM_ARRAY_ELEMENT_TYPE_U64:
        case SCM_ARRAY_ELEMENT_TYPE_S64:
        case SCM_ARRAY_ELEMENT_TYPE_F32:
        case SCM_ARRAY_ELEMENT_TYPE_F64:
        case SCM_ARRAY_ELEMENT_TYPE_C32:
        case SCM_ARRAY_ELEMENT_TYPE_C64:
          btype = i;
          break;
        default:
          scm_wrong_type_arg_msg (FUNC_NAME, SCM_ARG1, uvec_type,
                                  "uniform vector type");
        }
    }

  if (SCM_UNBNDP (offset))
    boffset = 0;
  else
    boffset = scm_to_size_t (offset);

  blen = scm_to_size_t (len);

  ret = scm_c_take_typed_bytevector ((signed char *) ptr + boffset,
				     blen, btype, pointer);

  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_bytevector_to_pointer, "bytevector->pointer", 1, 1, 0,
	    (SCM bv, SCM offset),
	    "Return a pointer pointer aliasing the memory pointed to by\n"
            "@var{bv} or @var{offset} bytes after @var{bv} when @var{offset}\n"
	    "is passed.")
#define FUNC_NAME s_scm_bytevector_to_pointer
{
  SCM ret;
  signed char *ptr;
  size_t boffset;

  SCM_VALIDATE_BYTEVECTOR (1, bv);
  ptr = SCM_BYTEVECTOR_CONTENTS (bv);

  if (SCM_UNBNDP (offset))
    boffset = 0;
  else
    boffset = scm_to_unsigned_integer (offset, 0,
                                       SCM_BYTEVECTOR_LENGTH (bv) - 1);

  ret = scm_from_pointer (ptr + boffset, NULL);
  register_weak_reference (ret, bv);
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_set_pointer_finalizer_x, "set-pointer-finalizer!", 2, 0, 0,
            (SCM pointer, SCM finalizer),
            "Arrange for the C procedure wrapped by @var{finalizer} to be\n"
            "called on the pointer wrapped by @var{pointer} when @var{pointer}\n"
            "becomes unreachable. Note: the C procedure should not call into\n"
            "Scheme. If you need a Scheme finalizer, use guardians.")
#define FUNC_NAME s_scm_set_pointer_finalizer_x
{
  SCM_VALIDATE_POINTER (1, pointer);
  SCM_VALIDATE_POINTER (2, finalizer);

  scm_i_add_finalizer (SCM2PTR (pointer), pointer_finalizer_trampoline,
                       SCM_POINTER_VALUE (finalizer));

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

void
scm_i_pointer_print (SCM pointer, SCM port, scm_print_state *pstate)
{
  scm_puts ("#<pointer 0x", port);
  scm_uintprint (scm_to_uintptr_t (scm_pointer_address (pointer)), 16, port);
  scm_putc ('>', port);
}


/* Non-primitive helpers functions.  These procedures could be
   implemented in terms of the primitives above but would be inefficient
   (heap allocation overhead, Scheme/C round trips, etc.)  */

SCM_DEFINE (scm_dereference_pointer, "dereference-pointer", 1, 0, 0,
	    (SCM pointer),
	    "Assuming @var{pointer} points to a memory region that\n"
	    "holds a pointer, return this pointer.")
#define FUNC_NAME s_scm_dereference_pointer
{
  void **ptr;

  SCM_VALIDATE_POINTER (1, pointer);

  ptr = SCM_POINTER_VALUE (pointer);
  if (SCM_UNLIKELY (ptr == NULL))
    null_pointer_error (FUNC_NAME);

  return scm_from_pointer (*ptr, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (scm_string_to_pointer, "string->pointer", 1, 1, 0,
	    (SCM string, SCM encoding),
	    "Return a foreign pointer to a nul-terminated copy of\n"
	    "@var{string} in the given @var{encoding}, defaulting to\n"
            "the current locale encoding.  The C string is freed when\n"
            "the returned foreign pointer becomes unreachable.\n\n"
            "This is the Scheme equivalent of @code{scm_to_stringn}.")
#define FUNC_NAME s_scm_string_to_pointer
{
  SCM_VALIDATE_STRING (1, string);

  /* XXX: Finalizers slow down libgc; they could be avoided if
     `scm_to_string' & co. were able to use libgc-allocated memory.  */

  if (SCM_UNBNDP (encoding))
    return scm_from_pointer (scm_to_locale_string (string), free);
  else
    {
      char *enc;
      SCM ret;
      
      SCM_VALIDATE_STRING (2, encoding);

      enc = scm_to_locale_string (encoding);
      scm_dynwind_begin (0);
      scm_dynwind_free (enc);

      ret = scm_from_pointer
        (scm_to_stringn (string, NULL, enc,
                         scm_i_default_string_failed_conversion_handler ()),
         free);

      scm_dynwind_end ();

      return ret;
    }
}
#undef FUNC_NAME

SCM_DEFINE (scm_pointer_to_string, "pointer->string", 1, 2, 0,
	    (SCM pointer, SCM length, SCM encoding),
	    "Return the string representing the C string pointed to by\n"
            "@var{pointer}.  If @var{length} is omitted or @code{-1}, the\n"
            "string is assumed to be nul-terminated.  Otherwise\n"
            "@var{length} is the number of bytes in memory pointed to by\n"
            "@var{pointer}.  The C string is assumed to be in the given\n"
            "@var{encoding}, defaulting to the current locale encoding.\n\n"
	    "This is the Scheme equivalent of @code{scm_from_stringn}.")
#define FUNC_NAME s_scm_pointer_to_string
{
  size_t len;

  SCM_VALIDATE_POINTER (1, pointer);

  if (SCM_UNBNDP (length)
      || scm_is_true (scm_eqv_p (length, scm_from_int (-1))))
    len = (size_t)-1;
  else
    len = scm_to_size_t (length);
    
  if (SCM_UNBNDP (encoding))
    return scm_from_locale_stringn (SCM_POINTER_VALUE (pointer), len);
  else
    {
      char *enc;
      SCM ret;
      
      SCM_VALIDATE_STRING (3, encoding);

      enc = scm_to_locale_string (encoding);
      scm_dynwind_begin (0);
      scm_dynwind_free (enc);

      ret = scm_from_stringn (SCM_POINTER_VALUE (pointer), len, enc,
                              scm_i_default_string_failed_conversion_handler ());

      scm_dynwind_end ();

      return ret;
    }
}
#undef FUNC_NAME



SCM_DEFINE (scm_alignof, "alignof", 1, 0, 0, (SCM type),
            "Return the alignment of @var{type}, in bytes.\n\n"
            "@var{type} should be a valid C type, like @code{int}.\n"
            "Alternately @var{type} may be the symbol @code{*}, in which\n"
            "case the alignment of a pointer is returned. @var{type} may\n"
            "also be a list of types, in which case the alignment of a\n"
            "@code{struct} with ABI-conventional packing is returned.")
#define FUNC_NAME s_scm_alignof
{
  if (SCM_I_INUMP (type))
    {
      switch (SCM_I_INUM (type))
        {
        case SCM_FOREIGN_TYPE_FLOAT:
          return scm_from_size_t (alignof_type (float));
        case SCM_FOREIGN_TYPE_DOUBLE:
          return scm_from_size_t (alignof_type (double));
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
        case SCM_FOREIGN_TYPE_COMPLEX_FLOAT:
          return scm_from_size_t (alignof_type (float _Complex));
        case SCM_FOREIGN_TYPE_COMPLEX_DOUBLE:
          return scm_from_size_t (alignof_type (double _Complex));
#endif
        case SCM_FOREIGN_TYPE_UINT8:
          return scm_from_size_t (alignof_type (uint8_t));
        case SCM_FOREIGN_TYPE_INT8:
          return scm_from_size_t (alignof_type (int8_t));
        case SCM_FOREIGN_TYPE_UINT16:
          return scm_from_size_t (alignof_type (uint16_t));
        case SCM_FOREIGN_TYPE_INT16:
          return scm_from_size_t (alignof_type (int16_t));
        case SCM_FOREIGN_TYPE_UINT32:
          return scm_from_size_t (alignof_type (uint32_t));
        case SCM_FOREIGN_TYPE_INT32:
          return scm_from_size_t (alignof_type (int32_t));
        case SCM_FOREIGN_TYPE_UINT64:
          return scm_from_size_t (alignof_type (uint64_t));
        case SCM_FOREIGN_TYPE_INT64:
          return scm_from_size_t (alignof_type (int64_t));
        default:
          scm_wrong_type_arg (FUNC_NAME, 1, type);
        }
    }
  else if (scm_is_eq (type, sym_asterisk))
    /* a pointer */
    return scm_from_size_t (alignof_type (void*));
  else if (scm_is_pair (type))
    {
      /* TYPE is a structure.  Section 3-3 of the i386, x86_64, PowerPC,
	 and SPARC P.S. of the System V ABI all say: "Aggregates
	 (structures and arrays) and unions assume the alignment of
	 their most strictly aligned component."  */
      size_t max;

      for (max = 0; scm_is_pair (type); type = SCM_CDR (type))
	{
	  size_t align;

	  align = scm_to_size_t (scm_alignof (SCM_CAR (type)));
	  if (align  > max)
	    max = align;
	}

      return scm_from_size_t (max);
    }
  else
    scm_wrong_type_arg (FUNC_NAME, 1, type);
}
#undef FUNC_NAME

SCM_DEFINE (scm_sizeof, "sizeof", 1, 0, 0, (SCM type),
            "Return the size of @var{type}, in bytes.\n\n"
            "@var{type} should be a valid C type, like @code{int}.\n"
            "Alternately @var{type} may be the symbol @code{*}, in which\n"
            "case the size of a pointer is returned. @var{type} may also\n"
            "be a list of types, in which case the size of a @code{struct}\n"
            "with ABI-conventional packing is returned.")
#define FUNC_NAME s_scm_sizeof
{
  if (SCM_I_INUMP (type))
    {
      switch (SCM_I_INUM (type))
        {
        case SCM_FOREIGN_TYPE_FLOAT:
          return scm_from_size_t (sizeof (float));
        case SCM_FOREIGN_TYPE_DOUBLE:
          return scm_from_size_t (sizeof (double));
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
        case SCM_FOREIGN_TYPE_COMPLEX_FLOAT:
          return scm_from_size_t (sizeof (float _Complex));
        case SCM_FOREIGN_TYPE_COMPLEX_DOUBLE:
          return scm_from_size_t (sizeof (double _Complex));
#endif
        case SCM_FOREIGN_TYPE_UINT8:
          return scm_from_size_t (sizeof (uint8_t));
        case SCM_FOREIGN_TYPE_INT8:
          return scm_from_size_t (sizeof (int8_t));
        case SCM_FOREIGN_TYPE_UINT16:
          return scm_from_size_t (sizeof (uint16_t));
        case SCM_FOREIGN_TYPE_INT16:
          return scm_from_size_t (sizeof (int16_t));
        case SCM_FOREIGN_TYPE_UINT32:
          return scm_from_size_t (sizeof (uint32_t));
        case SCM_FOREIGN_TYPE_INT32:
          return scm_from_size_t (sizeof (int32_t));
        case SCM_FOREIGN_TYPE_UINT64:
          return scm_from_size_t (sizeof (uint64_t));
        case SCM_FOREIGN_TYPE_INT64:
          return scm_from_size_t (sizeof (int64_t));
        default:
          scm_wrong_type_arg (FUNC_NAME, 1, type);
        }
    }
  else if (scm_is_eq (type, sym_asterisk))
    /* a pointer */
    return scm_from_size_t (sizeof (void*));
  else if (scm_is_pair (type))
    {
      /* a struct */
      size_t off = 0;
      size_t align = scm_to_size_t (scm_alignof(type));
      while (scm_is_pair (type))
        {
          off = ROUND_UP (off, scm_to_size_t (scm_alignof (scm_car (type))));
          off += scm_to_size_t (scm_sizeof (scm_car (type)));
          type = scm_cdr (type);
        }
      return scm_from_size_t (ROUND_UP(off, align));
    }
  else
    scm_wrong_type_arg (FUNC_NAME, 1, type);
}
#undef FUNC_NAME


/* return 1 on success, 0 on failure */
static int
parse_ffi_type (SCM type, int return_p, long *n_structs, long *n_struct_elts)
{
  if (SCM_I_INUMP (type))
    {
      if ((SCM_I_INUM (type) < 0 )
          || (SCM_I_INUM (type) > SCM_FOREIGN_TYPE_LAST))
        return 0;
      else if (SCM_I_INUM (type) == SCM_FOREIGN_TYPE_VOID && !return_p)
        return 0;
      else
        return 1;
    }
  else if (scm_is_eq (type, sym_asterisk))
    /* a pointer */
    return 1;
  else
    {
      long len;
      
      len = scm_ilength (type);
      if (len < 1)
        return 0;
      while (len--)
        {
          if (!parse_ffi_type (scm_car (type), 0, n_structs, n_struct_elts))
            return 0;
          (*n_struct_elts)++;
          type = scm_cdr (type);
        }
      (*n_structs)++;
      return 1;
    }
}
    
static void
fill_ffi_type (SCM type, ffi_type *ftype, ffi_type ***type_ptrs,
               ffi_type **types)
{
  if (SCM_I_INUMP (type))
    {
      switch (SCM_I_INUM (type))
        {
        case SCM_FOREIGN_TYPE_FLOAT:
          *ftype = ffi_type_float;
          return;
        case SCM_FOREIGN_TYPE_DOUBLE:
          *ftype = ffi_type_double;
          return;
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
        case SCM_FOREIGN_TYPE_COMPLEX_FLOAT:
          *ftype = ffi_type_complex_float;
          return;
        case SCM_FOREIGN_TYPE_COMPLEX_DOUBLE:
          *ftype = ffi_type_complex_double;
          return;
#endif
        case SCM_FOREIGN_TYPE_UINT8:
          *ftype = ffi_type_uint8;
          return;
        case SCM_FOREIGN_TYPE_INT8:
          *ftype = ffi_type_sint8;
          return;
        case SCM_FOREIGN_TYPE_UINT16:
          *ftype = ffi_type_uint16;
          return;
        case SCM_FOREIGN_TYPE_INT16:
          *ftype = ffi_type_sint16;
          return;
        case SCM_FOREIGN_TYPE_UINT32:
          *ftype = ffi_type_uint32;
          return;
        case SCM_FOREIGN_TYPE_INT32:
          *ftype = ffi_type_sint32;
          return;
        case SCM_FOREIGN_TYPE_UINT64:
          *ftype = ffi_type_uint64;
          return;
        case SCM_FOREIGN_TYPE_INT64:
          *ftype = ffi_type_sint64;
          return;
        case SCM_FOREIGN_TYPE_VOID:
          *ftype = ffi_type_void;
          return;
        default:
          scm_wrong_type_arg_msg ("pointer->procedure", 0, type,
                                  "foreign type");
        }
    }
  else if (scm_is_eq (type, sym_asterisk))
    /* a pointer */
    {
      *ftype = ffi_type_pointer;
      return;
    }
  else
    {
      long i, len;
      
      len = scm_ilength (type);

      ftype->size = 0;
      ftype->alignment = 0;
      ftype->type = FFI_TYPE_STRUCT;
      ftype->elements = *type_ptrs;
      *type_ptrs += len + 1;

      for (i = 0; i < len; i++)
        {
          ftype->elements[i] = *types;
          *types += 1;
          fill_ffi_type (scm_car (type), ftype->elements[i],
                         type_ptrs, types);
          type = scm_cdr (type);
        }
      ftype->elements[i] = NULL;
    }
}

/* Return a "cif" (call interface) for the given RETURN_TYPE and
   ARG_TYPES.  */
static ffi_cif *
make_cif (SCM return_type, SCM arg_types, const char *caller)
#define FUNC_NAME caller
{
  SCM walk;
  long i, nargs, n_structs, n_struct_elts;
  size_t cif_len;
  char *mem;
  ffi_cif *cif;
  ffi_type **type_ptrs;
  ffi_type *types;

  nargs = scm_ilength (arg_types);
  SCM_ASSERT (nargs >= 0, arg_types, 3, FUNC_NAME);
  /* fixme: assert nargs < 1<<32 */
  n_structs = n_struct_elts = 0;

  /* For want of talloc, we're going to have to do this in two passes: first we
     figure out how much memory is needed for all types, then we allocate the
     cif and the types all in one block. */
  if (!parse_ffi_type (return_type, 1, &n_structs, &n_struct_elts))
    scm_wrong_type_arg (FUNC_NAME, 1, return_type);
  for (walk = arg_types; scm_is_pair (walk); walk = scm_cdr (walk))
    if (!parse_ffi_type (scm_car (walk), 0, &n_structs, &n_struct_elts))
      scm_wrong_type_arg (FUNC_NAME, 3, scm_car (walk));

  /* the memory: with space for the cif itself */
  cif_len = sizeof (ffi_cif);

  /* then ffi_type pointers: one for each arg, one for each struct
     element, and one for each struct (for null-termination) */
  cif_len = (ROUND_UP (cif_len, alignof_type (void *))
	     + (nargs + n_structs + n_struct_elts)*sizeof(void*));

  /* then the ffi_type structs themselves, one per arg and struct element, and
     one for the return val */
  cif_len = (ROUND_UP (cif_len, alignof_type (ffi_type))
	     + (nargs + n_struct_elts + 1)*sizeof(ffi_type));

  mem = scm_gc_malloc_pointerless (cif_len, "foreign");
  /* ensure all the memory is initialized, even the holes */
  memset (mem, 0, cif_len);
  cif = (ffi_cif *) mem;

  /* reuse cif_len to walk through the mem */
  cif_len = ROUND_UP (sizeof (ffi_cif), alignof_type (void *));
  type_ptrs = (ffi_type**)(mem + cif_len);
  cif_len = ROUND_UP (cif_len
		      + (nargs + n_structs + n_struct_elts)*sizeof(void*),
		      alignof_type (ffi_type));
  types = (ffi_type*)(mem + cif_len);

  /* whew. now knit the pointers together. */
  cif->rtype = types++;
  fill_ffi_type (return_type, cif->rtype, &type_ptrs, &types);
  cif->arg_types = type_ptrs;
  type_ptrs += nargs;
  for (walk = arg_types, i = 0; scm_is_pair (walk); walk = scm_cdr (walk), i++)
    {
      cif->arg_types[i] = types++;
      fill_ffi_type (scm_car (walk), cif->arg_types[i], &type_ptrs, &types);
    }

  /* round out the cif, and we're done. */
  cif->abi = FFI_DEFAULT_ABI;
  cif->nargs = nargs;
  cif->bytes = 0;
  cif->flags = 0;

  if (FFI_OK != ffi_prep_cif (cif, FFI_DEFAULT_ABI, cif->nargs, cif->rtype,
			      cif->arg_types))
    SCM_MISC_ERROR ("ffi_prep_cif failed", SCM_EOL);

  return cif;
}
#undef FUNC_NAME

static SCM
pointer_to_procedure (SCM return_type, SCM func_ptr, SCM arg_types,
                      int with_errno)
#define FUNC_NAME "pointer->procedure"
{
  ffi_cif *cif;

  SCM_VALIDATE_POINTER (2, func_ptr);

  cif = make_cif (return_type, arg_types, FUNC_NAME);

  return cif_to_procedure (scm_from_pointer (cif, NULL), func_ptr,
                           with_errno);
}
#undef FUNC_NAME

SCM
scm_pointer_to_procedure (SCM return_type, SCM func_ptr, SCM arg_types)
{
  return pointer_to_procedure (return_type, func_ptr, arg_types, 0);
}

SCM
scm_pointer_to_procedure_with_errno (SCM return_type, SCM func_ptr,
                                     SCM arg_types)
{
  return pointer_to_procedure (return_type, func_ptr, arg_types, 1);
}

SCM_KEYWORD (k_return_errno, "return-errno?");

SCM_INTERNAL SCM scm_i_pointer_to_procedure (SCM, SCM, SCM, SCM);
SCM_DEFINE (scm_i_pointer_to_procedure, "pointer->procedure", 3, 0, 1,
            (SCM return_type, SCM func_ptr, SCM arg_types, SCM keyword_args),
            "Make a foreign function.\n\n"
            "Given the foreign void pointer @var{func_ptr}, its argument and\n"
            "return types @var{arg_types} and @var{return_type}, return a\n"
            "procedure that will pass arguments to the foreign function\n"
            "and return appropriate values.\n\n"
            "@var{arg_types} should be a list of foreign types.\n"
            "@code{return_type} should be a foreign type.\n"
            "If the @code{#:return-errno?} keyword argument is provided and\n"
            "its value is true, then the returned procedure will return two\n"
            "values, with @code{errno} as the second value.")
#define FUNC_NAME s_scm_i_pointer_to_procedure
{
  SCM return_errno = SCM_BOOL_F;

  scm_c_bind_keyword_arguments (FUNC_NAME, keyword_args, 0,
                                k_return_errno, &return_errno,
                                SCM_UNDEFINED);

  return pointer_to_procedure (return_type, func_ptr, arg_types,
                               scm_to_bool (return_errno));
}
#undef FUNC_NAME



static const uint32_t *
get_foreign_stub_code (unsigned int nargs, int with_errno)
{
  size_t i;
  size_t code_len = with_errno ? 4 : 5;
  uint32_t *ret, *code;

  if (nargs >= (1 << 24) + 1)
    scm_misc_error ("make-foreign-function", "too many arguments: ~a",
                    scm_list_1 (scm_from_uint (nargs)));

  ret = scm_i_alloc_primitive_code_with_instrumentation (code_len, &code);

  i = 0;
  code[i++] = SCM_PACK_OP_24 (assert_nargs_ee, nargs + 1);
  code[i++] = SCM_PACK_OP_12_12 (foreign_call, 0, 1);
  code[i++] = SCM_PACK_OP_24 (handle_interrupts, 0);
  if (!with_errno)
    code[i++] = SCM_PACK_OP_24 (reset_frame, 1);
  code[i++] = SCM_PACK_OP_24 (return_values, 0);

  return ret;
}

static SCM
cif_to_procedure (SCM cif, SCM func_ptr, int with_errno)
{
  ffi_cif *c_cif;
  SCM ret;
  scm_t_bits nfree = 2;
  scm_t_bits flags = SCM_F_PROGRAM_IS_FOREIGN;

  c_cif = (ffi_cif *) SCM_POINTER_VALUE (cif);

  ret = scm_words (scm_tc7_program | (nfree << 16) | flags, nfree + 2);
  SCM_SET_CELL_WORD_1 (ret, get_foreign_stub_code (c_cif->nargs, with_errno));
  SCM_PROGRAM_FREE_VARIABLE_SET (ret, 0, cif);
  SCM_PROGRAM_FREE_VARIABLE_SET (ret, 1, func_ptr);
  
  return ret;
}

/* Set *LOC to the foreign representation of X with TYPE.  */
static void
unpack (const ffi_type *type, void *loc, SCM x, int return_value_p)
#define FUNC_NAME "foreign-call"
{
  switch (type->type)
    {
    case FFI_TYPE_FLOAT:
      *(float *) loc = scm_to_double (x);
      break;
    case FFI_TYPE_DOUBLE:
      *(double *) loc = scm_to_double (x);
      break;

      /* no FFI_TYPE_xxx_COMPLEX or (FFI_TYPE_COMPLEX_xxx) :-| */

#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
    case FFI_TYPE_COMPLEX:
      {
        double re = scm_to_double (scm_real_part(x));
        double im = scm_to_double (scm_imag_part(x));
        if (sizeof (float _Complex) == type->size)
          *(float _Complex *) loc = (float)re + _Complex_I * (float)im;
        else if (sizeof (double _Complex) == type->size)
          *(double _Complex *) loc = re + _Complex_I * im;
        else
          abort();
      }
      break;
#endif

    /* For integer return values smaller than `int', libffi expects the
       result in an `ffi_arg'-long buffer.  */

    case FFI_TYPE_UINT8:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_uint8 (x);
      else
	*(uint8_t *) loc = scm_to_uint8 (x);
      break;
    case FFI_TYPE_SINT8:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_int8 (x);
      else
	*(int8_t *) loc = scm_to_int8 (x);
      break;
    case FFI_TYPE_UINT16:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_uint16 (x);
      else
	*(uint16_t *) loc = scm_to_uint16 (x);
      break;
    case FFI_TYPE_SINT16:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_int16 (x);
      else
	*(int16_t *) loc = scm_to_int16 (x);
      break;
    case FFI_TYPE_UINT32:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_uint32 (x);
      else
	*(uint32_t *) loc = scm_to_uint32 (x);
      break;
    case FFI_TYPE_SINT32:
      if (return_value_p)
	*(ffi_arg *) loc = scm_to_int32 (x);
      else
	*(int32_t *) loc = scm_to_int32 (x);
      break;
    case FFI_TYPE_UINT64:
      *(uint64_t *) loc = scm_to_uint64 (x);
      break;
    case FFI_TYPE_SINT64:
      *(int64_t *) loc = scm_to_int64 (x);
      break;
    case FFI_TYPE_STRUCT:
      SCM_VALIDATE_POINTER (1, x);
      memcpy (loc, SCM_POINTER_VALUE (x), type->size);
      break;
    case FFI_TYPE_POINTER:
      SCM_VALIDATE_POINTER (1, x);
      *(void **) loc = SCM_POINTER_VALUE (x);
      break;
    case FFI_TYPE_VOID:
      /* Do nothing.  */
      break;
    default:
      abort ();
    }
}
#undef FUNC_NAME

/* Return a Scheme representation of the foreign value at LOC of type
   TYPE.  When RETURN_VALUE_P is true, LOC is assumed to point to a
   return value buffer; otherwise LOC is assumed to point to an
   argument buffer.  */
static SCM
pack (const ffi_type * type, const void *loc, int return_value_p)
{
  switch (type->type)
    {
    case FFI_TYPE_VOID:
      return SCM_UNSPECIFIED;
    case FFI_TYPE_FLOAT:
      return scm_from_double (*(float *) loc);
    case FFI_TYPE_DOUBLE:
      return scm_from_double (*(double *) loc);

      /* no FFI_TYPE_xxx_COMPLEX or (FFI_TYPE_COMPLEX_xxx) :-| */

#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
    case FFI_TYPE_COMPLEX:
      {
        double re, im;
        if (sizeof (float _Complex) == type->size)
          {
            re = crealf(*(float _Complex *) loc);
            im = cimagf(*(float _Complex *) loc);
          }
        else if (sizeof (double _Complex) == type->size)
          {
            re = creal(*(double _Complex *) loc);
            im = cimag(*(double _Complex *) loc);
          }
        else
          abort ();
        return scm_make_rectangular (scm_from_double (re), scm_from_double (im));
      }
#endif

      /* For integer return values smaller than `int', libffi stores the
	 result in an `ffi_arg'-long buffer, of which only the
	 significant bits must be kept---hence the pair of casts below.
	 See <http://thread.gmane.org/gmane.comp.lib.ffi.general/406>
	 for details.  */

    case FFI_TYPE_UINT8:
      if (return_value_p)
	return scm_from_uint8 ((uint8_t) *(ffi_arg *) loc);
      else
	return scm_from_uint8 (* (uint8_t *) loc);
    case FFI_TYPE_SINT8:
      if (return_value_p)
	return scm_from_int8 ((int8_t) *(ffi_arg *) loc);
      else
	return scm_from_int8 (* (int8_t *) loc);
    case FFI_TYPE_UINT16:
      if (return_value_p)
	return scm_from_uint16 ((uint16_t) *(ffi_arg *) loc);
      else
	return scm_from_uint16 (* (uint16_t *) loc);
    case FFI_TYPE_SINT16:
      if (return_value_p)
	return scm_from_int16 ((int16_t) *(ffi_arg *) loc);
      else
	return scm_from_int16 (* (int16_t *) loc);
    case FFI_TYPE_UINT32:
      if (return_value_p)
	return scm_from_uint32 ((uint32_t) *(ffi_arg *) loc);
      else
	return scm_from_uint32 (* (uint32_t *) loc);
    case FFI_TYPE_SINT32:
      if (return_value_p)
	return scm_from_int32 ((int32_t) *(ffi_arg *) loc);
      else
	return scm_from_int32 (* (int32_t *) loc);
    case FFI_TYPE_UINT64:
      return scm_from_uint64 (*(uint64_t *) loc);
    case FFI_TYPE_SINT64:
      return scm_from_int64 (*(int64_t *) loc);

    case FFI_TYPE_STRUCT:
      {
	void *mem = scm_gc_malloc_pointerless (type->size, "foreign");
	memcpy (mem, loc, type->size);
	return scm_from_pointer (mem, NULL);
      }
    case FFI_TYPE_POINTER:
      return scm_from_pointer (*(void **) loc, NULL);
    default:
      abort ();
    }
}


#define MAX(A, B) ((A) >= (B) ? (A) : (B))

SCM
scm_i_foreign_call (SCM cif_scm, SCM pointer_scm, int *errno_ret,
                    const union scm_vm_stack_element *argv)
{
  /* FOREIGN is the pair that cif_to_procedure set as the 0th element of the
     objtable. */
  ffi_cif *cif;
  void (*func) (void);
  uint8_t *data;
  void *rvalue;
  void **args;
  unsigned i;
  size_t arg_size;
  ptrdiff_t off;

  cif = SCM_POINTER_VALUE (cif_scm);
  func = SCM_POINTER_VALUE (pointer_scm);

  /* Argument pointers.  */
  args = alloca (sizeof (void *) * cif->nargs);

  /* Compute the worst-case amount of memory needed to store all the argument
     values.  Note: as of libffi 3.0.9 `cif->bytes' is undocumented and is zero,
     so it can't be used for that purpose.  */
  for (i = 0, arg_size = 0; i < cif->nargs; i++)
    arg_size += cif->arg_types[i]->size + cif->arg_types[i]->alignment - 1;

  /* Space for argument values, followed by return value.  */
  data = alloca (arg_size + cif->rtype->size
		 + MAX (sizeof (void *), cif->rtype->alignment));

  /* Unpack ARGV to native values, setting ARGV pointers.  */
  for (i = 0, off = 0;
       i < cif->nargs;
       off = (uint8_t *) args[i] - data + cif->arg_types[i]->size,
	 i++)
    {
      /* Suitably align the storage area for argument I.  */
      args[i] = (void *) ROUND_UP ((uintptr_t) data + off,
				   cif->arg_types[i]->alignment);
      assert ((uintptr_t) args[i] % cif->arg_types[i]->alignment == 0);
      unpack (cif->arg_types[i], args[i], argv[cif->nargs - i - 1].as_scm, 0);
    }

  /* Prepare space for the return value.  On some platforms, such as
     `armv5tel-*-linux-gnueabi', the return value has to be at least
     word-aligned, even if its type doesn't have any alignment requirement as is
     the case with `char'.  */
  rvalue = (void *) ROUND_UP ((uintptr_t) data + off,
			      MAX (sizeof (void *), cif->rtype->alignment));

  /* off we go! */
  errno = 0;
  ffi_call (cif, func, rvalue, args);
  *errno_ret = errno;

  return pack (cif->rtype, rvalue, 1);
}


/* Function pointers aka. "callbacks" or "closures".  */

#ifdef FFI_CLOSURES

/* Trampoline to invoke a libffi closure that wraps a Scheme
   procedure.  */
static void
invoke_closure (ffi_cif *cif, void *ret, void **args, void *data)
{
  size_t i;
  SCM proc, *argv, result;

  proc = SCM_PACK_POINTER (data);

  argv = alloca (cif->nargs * sizeof (*argv));

  /* Pack ARGS to SCM values, setting ARGV pointers.  */
  for (i = 0; i < cif->nargs; i++)
    argv[i] = pack (cif->arg_types[i], args[i], 0);

  result = scm_call_n (proc, argv, cif->nargs);

  unpack (cif->rtype, ret, result, 1);
}

SCM_DEFINE (scm_procedure_to_pointer, "procedure->pointer", 3, 0, 0,
	    (SCM return_type, SCM proc, SCM arg_types),
	    "Return a pointer to a C function of type @var{return_type}\n"
	    "taking arguments of types @var{arg_types} (a list) and\n"
	    "behaving as a proxy to procedure @var{proc}.  Thus\n"
	    "@var{proc}'s arity, supported argument types, and return\n"
	    "type should match @var{return_type} and @var{arg_types}.\n")
#define FUNC_NAME s_scm_procedure_to_pointer
{
  SCM cif_pointer, pointer;
  ffi_cif *cif;
  ffi_status err;
  void *closure, *executable;

  cif = make_cif (return_type, arg_types, FUNC_NAME);

  closure = ffi_closure_alloc (sizeof (ffi_closure), &executable);
  err = ffi_prep_closure_loc ((ffi_closure *) closure, cif,
			      invoke_closure, SCM_UNPACK_POINTER (proc),
			      executable);
  if (err != FFI_OK)
    {
      ffi_closure_free (closure);
      SCM_MISC_ERROR ("`ffi_prep_closure_loc' failed", SCM_EOL);
    }

  /* CIF points to GC-managed memory and it should remain as long as
     POINTER (see below) is live.  Wrap it in a Scheme pointer to then
     hold a weak reference on it.  */
  cif_pointer = scm_from_pointer (cif, NULL);

  if (closure == executable)
    {
      pointer = scm_from_pointer (executable, ffi_closure_free);
      register_weak_reference (pointer,
			       scm_list_2 (proc, cif_pointer));
    }
  else
    {
      /* CLOSURE needs to be freed eventually.  However, since
	 `GC_all_interior_pointers' is disabled, we can't just register
	 a finalizer for CLOSURE.  Instead, we create a pointer object
	 for CLOSURE, with a finalizer, and register it as a weak
	 reference of POINTER.  */
      SCM friend;

      pointer = scm_from_pointer (executable, NULL);
      friend = scm_from_pointer (closure, ffi_closure_free);

      register_weak_reference (pointer,
			       scm_list_3 (proc, cif_pointer, friend));
    }

  return pointer;
}
#undef FUNC_NAME

#endif /* FFI_CLOSURES */



static void
scm_init_foreign (void)
{
#ifndef SCM_MAGIC_SNARFER
#include "foreign.x"
#endif
  scm_define (sym_void, scm_from_uint8 (SCM_FOREIGN_TYPE_VOID));
  scm_define (sym_float, scm_from_uint8 (SCM_FOREIGN_TYPE_FLOAT));
  scm_define (sym_double, scm_from_uint8 (SCM_FOREIGN_TYPE_DOUBLE));
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
  scm_define (sym_complex_float, scm_from_uint8 (SCM_FOREIGN_TYPE_COMPLEX_FLOAT));
  scm_define (sym_complex_double, scm_from_uint8 (SCM_FOREIGN_TYPE_COMPLEX_DOUBLE));
#endif
  scm_define (sym_uint8, scm_from_uint8 (SCM_FOREIGN_TYPE_UINT8));
  scm_define (sym_int8, scm_from_uint8 (SCM_FOREIGN_TYPE_INT8));
  scm_define (sym_uint16, scm_from_uint8 (SCM_FOREIGN_TYPE_UINT16));
  scm_define (sym_int16, scm_from_uint8 (SCM_FOREIGN_TYPE_INT16));
  scm_define (sym_uint32, scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32));
  scm_define (sym_int32, scm_from_uint8 (SCM_FOREIGN_TYPE_INT32));
  scm_define (sym_uint64, scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64));
  scm_define (sym_int64, scm_from_uint8 (SCM_FOREIGN_TYPE_INT64));

  scm_define (sym_short,
#if SIZEOF_SHORT == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SIZEOF_SHORT == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#elif SIZEOF_SHORT == 2
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT16)
#else
# error unsupported sizeof (short)
#endif
	      );

  scm_define (sym_unsigned_short,
#if SIZEOF_SHORT == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64)
#elif SIZEOF_SHORT == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32)
#elif SIZEOF_SHORT == 2
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT16)
#else
# error unsupported sizeof (short)
#endif
	      );

  scm_define (sym_int,
#if SIZEOF_INT == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SIZEOF_INT == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#else
# error unsupported sizeof (int)
#endif
	      );

  scm_define (sym_unsigned_int,
#if SIZEOF_UNSIGNED_INT == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64)
#elif SIZEOF_UNSIGNED_INT == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32)
#else
# error unsupported sizeof (unsigned int)
#endif
	      );

  scm_define (sym_long,
#if SIZEOF_LONG == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SIZEOF_LONG == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#else
# error unsupported sizeof (long)
#endif
	      );

  scm_define (sym_unsigned_long,
#if SIZEOF_UNSIGNED_LONG == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64)
#elif SIZEOF_UNSIGNED_LONG == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32)
#else
# error unsupported sizeof (unsigned long)
#endif
	      );

  scm_define (sym_size_t,
#if SIZEOF_SIZE_T == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64)
#elif SIZEOF_SIZE_T == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32)
#else
# error unsupported sizeof (size_t)
#endif
	      );

  scm_define (sym_ssize_t,
#if SIZEOF_SIZE_T == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SIZEOF_SIZE_T == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#else
# error unsupported sizeof (ssize_t)
#endif
	      );

  scm_define (sym_ptrdiff_t,
#if SCM_SIZEOF_SCM_T_PTRDIFF == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SCM_SIZEOF_SCM_T_PTRDIFF == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#else
# error unsupported sizeof (ptrdiff_t)
#endif
	      );

  scm_define (sym_intptr_t,
#if SCM_SIZEOF_INTPTR_T == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT64)
#elif SCM_SIZEOF_INTPTR_T == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_INT32)
#else
# error unsupported sizeof (intptr_t)
#endif
	      );

  scm_define (sym_uintptr_t,
#if SCM_SIZEOF_UINTPTR_T == 8
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT64)
#elif SCM_SIZEOF_UINTPTR_T == 4
	      scm_from_uint8 (SCM_FOREIGN_TYPE_UINT32)
#else
# error unsupported sizeof (uintptr_t)
#endif
	      );

  null_pointer = scm_cell (scm_tc7_pointer, 0);
  scm_define (sym_null, null_pointer);
}

void
scm_register_foreign (void)
{
  scm_c_register_extension ("libguile-" SCM_EFFECTIVE_VERSION,
                            "scm_init_foreign",
                            (scm_t_extension_init_func)scm_init_foreign,
                            NULL);
  pointer_weak_refs = scm_c_make_weak_table (0, SCM_WEAK_TABLE_KIND_KEY);
}
