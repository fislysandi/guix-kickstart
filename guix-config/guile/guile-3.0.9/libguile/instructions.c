/* Copyright 2001,2009-2013,2017-2018,2020
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

#include "extensions.h"
#include "gsubr.h"
#include "list.h"
#include "numbers.h"
#include "pairs.h"
#include "symbols.h"
#include "threads.h"
#include "version.h"

#include "instructions.h"


SCM_SYMBOL (sym_left_arrow, "<-");
SCM_SYMBOL (sym_bang, "!");


#define FOR_EACH_INSTRUCTION_WORD_TYPE(M)       \
    M(X32)                                      \
    M(X8_S24)                                   \
    M(X8_F24)                                   \
    M(X8_L24)                                   \
    M(X8_C24)                                   \
    M(X8_S8_I16)                                \
    M(X8_S8_ZI16)                               \
    M(X8_S12_S12)                               \
    M(X8_S12_C12)                               \
    M(X8_S12_Z12)                               \
    M(X8_C12_C12)                               \
    M(X8_F12_F12)                               \
    M(X8_S8_S8_S8)                              \
    M(X8_S8_C8_S8)                              \
    M(X8_S8_S8_C8)                              \
    M(C8_C24)                                   \
    M(C8_S24)                                   \
    M(C32) /* Unsigned. */                      \
    M(I32) /* Immediate. */                     \
    M(A32) /* Immediate, high bits. */          \
    M(B32) /* Immediate, low bits. */           \
    M(AF32) /* Immediate double, high bits. */  \
    M(BF32) /* Immediate double, low bits. */   \
    M(AU32) /* Immediate uint64, high bits. */  \
    M(BU32) /* Immediate uint64, low bits. */   \
    M(AS32) /* Immediate int64, high bits. */   \
    M(BS32) /* Immediate int64, low bits. */    \
    M(N32) /* Non-immediate. */                 \
    M(R32) /* Scheme value (indirected). */     \
    M(L32) /* Label. */                         \
    M(LO32) /* Label with offset. */            \
    M(B1_C7_L24)                                \
    M(B1_X7_L24)                                \
    M(B1_X7_C24)                                \
    M(B1_X7_S24)                                \
    M(B1_X7_F24)                                \
    M(B1_X31)                                   \
    M(C16_C16)                                  \
    M(V32_X8_L24) /* Length-prefixed array of X8_L24. */ \
    /**/

#define TYPE_WIDTH 6

enum word_type
  {
#define ENUM(type) type,
    FOR_EACH_INSTRUCTION_WORD_TYPE (ENUM)
#undef ENUM
  };

static SCM word_type_symbols[] =
  {
#define FALSE(type) SCM_BOOL_F,
    FOR_EACH_INSTRUCTION_WORD_TYPE (FALSE)
#undef FALSE
  };

#define OP(n,type) (((type) + 1) << (n*TYPE_WIDTH))

/* The VM_DEFINE_OP macro uses a CPP-based DSL to describe what kinds of
   arguments each instruction takes.  This piece of code is the only
   bit that actually interprets that language.  These macro definitions
   encode the operand types into bits in a 64-bit integer.

   (instruction-list) parses those encoded values into lists of symbols,
   one for each 64-bit word that the operator takes.  This list is used
   by Scheme to generate assemblers and disassemblers for the
   instructions.  */

#define NOP UINT64_MAX
#define OP1(type0) \
  (OP (0, type0))
#define OP2(type0, type1) \
  (OP (0, type0) | OP (1, type1))
#define OP3(type0, type1, type2) \
  (OP (0, type0) | OP (1, type1) | OP (2, type2))
#define OP4(type0, type1, type2, type3) \
  (OP (0, type0) | OP (1, type1) | OP (2, type2) | OP (3, type3))
#define OP5(type0, type1, type2, type3, type4) \
  (OP (0, type0) | OP (1, type1) | OP (2, type2) | OP (3, type3) | OP (4, type4))

#define OP_DST (1 << (TYPE_WIDTH * 5))

#define DOP1(t0)                 (OP1(t0) | OP_DST)
#define DOP2(t0, t1)             (OP2(t0, t1) | OP_DST)
#define DOP3(t0, t1, t2)         (OP3(t0, t1, t2) | OP_DST)
#define DOP4(t0, t1, t2, t3)     (OP4(t0, t1, t2, t3) | OP_DST)
#define DOP5(t0, t1, t2, t3, t4) (OP5(t0, t1, t2, t3, t4) | OP_DST)

#define WORD_TYPE_AND_FLAG(n, word) \
  (((word) >> ((n) * TYPE_WIDTH)) & ((1 << TYPE_WIDTH) - 1))
#define WORD_TYPE(n, word) \
  (WORD_TYPE_AND_FLAG (n, word) - 1)
#define HAS_WORD(n, word) \
  (WORD_TYPE_AND_FLAG (n, word) != 0)

/* Scheme interface */

static SCM
parse_instruction (uint8_t opcode, const char *name, uint64_t meta)
{
  SCM tail = SCM_EOL;
  int len;

  /* Format: (name opcode word0 word1 ...) */

  if (HAS_WORD (4, meta))
    len = 5;
  else if (HAS_WORD (3, meta))
    len = 4;
  else if (HAS_WORD (2, meta))
    len = 3;
  else if (HAS_WORD (1, meta))
    len = 2;
  else if (HAS_WORD (0, meta))
    len = 1;
  else
    abort ();

  switch (len)
    {
    case 5:
      tail = scm_cons (word_type_symbols[WORD_TYPE (4, meta)], tail);
    case 4:
      tail = scm_cons (word_type_symbols[WORD_TYPE (3, meta)], tail);
    case 3:
      tail = scm_cons (word_type_symbols[WORD_TYPE (2, meta)], tail);
    case 2:
      tail = scm_cons (word_type_symbols[WORD_TYPE (1, meta)], tail);
    case 1:
      tail = scm_cons (word_type_symbols[WORD_TYPE (0, meta)], tail);
    default:
      tail = scm_cons ((meta & OP_DST) ? sym_left_arrow : sym_bang, tail);
      tail = scm_cons (scm_from_int (opcode), tail);
      tail = scm_cons (scm_from_utf8_symbol (name), tail);
      return tail;
    }
}

SCM_DEFINE (scm_instruction_list, "instruction-list", 0, 0, 0,
	    (void),
	    "")
#define FUNC_NAME s_scm_instruction_list
{
  SCM list = SCM_EOL;

#define INIT(opcode, tag, name, meta) \
  if (name) list = scm_cons (parse_instruction (opcode, name, meta), list);
  FOR_EACH_VM_OPERATION (INIT);
#undef INIT

  return scm_reverse_x (list, SCM_EOL);
}
#undef FUNC_NAME

void
scm_bootstrap_instructions (void)
{
  scm_c_register_extension ("libguile-" SCM_EFFECTIVE_VERSION,
                            "scm_init_instructions",
                            (scm_t_extension_init_func)scm_init_instructions,
                            NULL);
}

void
scm_init_instructions (void)
{
#define INIT(type) \
  word_type_symbols[type] = scm_from_utf8_symbol (#type);
    FOR_EACH_INSTRUCTION_WORD_TYPE (INIT)
#undef INIT

#ifndef SCM_MAGIC_SNARFER
#include "instructions.x"
#endif
}
