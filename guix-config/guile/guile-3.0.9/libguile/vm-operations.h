/* This file was generated automatically from vm-engine.c; do not
   edit.  See the source file for copyright information.  */

#define FOR_EACH_VM_OPERATION(M) \
    M (0, halt, "halt", OP1 (X32)) \
    M (1, instrument_entry, "instrument-entry", OP2 (X32, N32)) \
    M (2, instrument_loop, "instrument-loop", OP2 (X32, N32)) \
    M (3, call, "call", OP2 (X8_F24, X8_C24)) \
    M (4, call_label, "call-label", OP3 (X8_F24, X8_C24, L32)) \
    M (5, tail_call, "tail-call", OP1 (X32)) \
    M (6, tail_call_label, "tail-call-label", OP2 (X32, L32)) \
    M (7, return_values, "return-values", OP1 (X32)) \
    M (8, receive, "receive", DOP2 (X8_F12_F12, X8_C24)) \
    M (9, receive_values, "receive-values", OP2 (X8_F24, B1_X7_C24)) \
    M (10, assert_nargs_ee, "assert-nargs-ee", OP1 (X8_C24)) \
    M (11, assert_nargs_ge, "assert-nargs-ge", OP1 (X8_C24)) \
    M (12, assert_nargs_le, "assert-nargs-le", OP1 (X8_C24)) \
    M (13, assert_nargs_ee_locals, "assert-nargs-ee/locals", OP1 (X8_C12_C12)) \
    M (14, check_arguments, "arguments<=?", OP1 (X8_C24)) \
    M (15, check_positional_arguments, "positional-arguments<=?", OP2 (X8_C24, X8_C24)) \
    M (16, bind_kwargs, "bind-kwargs", OP4 (X8_C24, C8_C24, X8_C24, N32)) \
    M (17, bind_rest, "bind-rest", DOP1 (X8_F24)) \
    M (18, alloc_frame, "alloc-frame", OP1 (X8_C24)) \
    M (19, reset_frame, "reset-frame", OP1 (X8_C24)) \
    M (20, mov, "mov", DOP1 (X8_S12_S12)) \
    M (21, long_mov, "long-mov", DOP2 (X8_S24, X8_S24)) \
    M (22, long_fmov, "long-fmov", DOP2 (X8_F24, X8_F24)) \
    M (23, push, "push", OP1 (X8_S24)) \
    M (24, pop, "pop", DOP1 (X8_S24)) \
    M (25, drop, "drop", OP1 (X8_C24)) \
    M (26, shuffle_down, "shuffle-down", OP1 (X8_F12_F12)) \
    M (27, expand_apply_argument, "expand-apply-argument", OP1 (X32)) \
    M (28, subr_call, "subr-call", OP1 (X8_C24)) \
    M (29, foreign_call, "foreign-call", OP1 (X8_C12_C12)) \
    M (30, continuation_call, "continuation-call", OP1 (X8_C24)) \
    M (31, compose_continuation, "compose-continuation", OP1 (X8_C24)) \
    M (32, capture_continuation, "capture-continuation", DOP1 (X8_S24)) \
    M (33, abort, "abort", OP1 (X32)) \
    M (34, prompt, "prompt", OP3 (X8_S24, B1_X7_F24, X8_L24)) \
    M (35, builtin_ref, "builtin-ref", DOP1 (X8_S12_C12)) \
    M (36, throw, "throw", OP1 (X8_S12_S12)) \
    M (37, throw_value, "throw/value", OP2 (X8_S24, N32)) \
    M (38, throw_value_and_data, "throw/value+data", OP2 (X8_S24, N32)) \
    M (39, handle_interrupts, "handle-interrupts", OP1 (X32)) \
    M (40, return_from_interrupt, "return-from-interrupt", OP1 (X32)) \
    M (41, call_thread, "call-thread", OP2 (X32, C32)) \
    M (42, call_thread_scm, "call-thread-scm", OP2 (X8_S24, C32)) \
    M (43, call_thread_scm_scm, "call-thread-scm-scm", OP2 (X8_S12_S12, C32)) \
    M (44, call_scm_sz_u32, "call-scm-sz-u32", OP2 (X8_S8_S8_S8, C32)) \
    M (45, call_scm_from_thread, "call-scm<-thread", DOP2 (X8_S24, C32)) \
    M (46, call_s64_from_scm, "call-s64<-scm", DOP2 (X8_S12_S12, C32)) \
    M (47, call_scm_from_u64, "call-scm<-u64", DOP2 (X8_S12_S12, C32)) \
    M (48, call_scm_from_s64, "call-scm<-s64", DOP2 (X8_S12_S12, C32)) \
    M (49, call_scm_from_scm, "call-scm<-scm", DOP2 (X8_S12_S12, C32)) \
    M (50, call_f64_from_scm, "call-f64<-scm", DOP2 (X8_S12_S12, C32)) \
    M (51, call_u64_from_scm, "call-u64<-scm", DOP2 (X8_S12_S12, C32)) \
    M (52, call_scm_from_scm_scm, "call-scm<-scm-scm", DOP2 (X8_S8_S8_S8, C32)) \
    M (53, call_scm_from_scm_uimm, "call-scm<-scm-uimm", DOP2 (X8_S8_S8_C8, C32)) \
    M (54, call_scm_from_thread_scm, "call-scm<-thread-scm", DOP2 (X8_S12_S12, C32)) \
    M (55, call_scm_from_scm_u64, "call-scm<-scm-u64", DOP2 (X8_S8_S8_S8, C32)) \
    M (56, make_short_immediate, "make-short-immediate", DOP1 (X8_S8_I16)) \
    M (57, make_long_immediate, "make-long-immediate", DOP2 (X8_S24, I32)) \
    M (58, make_long_long_immediate, "make-long-long-immediate", DOP3 (X8_S24, A32, B32)) \
    M (59, make_non_immediate, "make-non-immediate", DOP2 (X8_S24, N32)) \
    M (60, load_label, "load-label", DOP2 (X8_S24, L32)) \
    M (61, load_f64, "load-f64", DOP3 (X8_S24, AF32, BF32)) \
    M (62, load_u64, "load-u64", DOP3 (X8_S24, AU32, BU32)) \
    M (63, load_s64, "load-s64", DOP3 (X8_S24, AS32, BS32)) \
    M (64, current_thread, "current-thread", DOP1 (X8_S24)) \
    M (65, allocate_words, "allocate-words", DOP1 (X8_S12_S12)) \
    M (66, allocate_words_immediate, "allocate-words/immediate", DOP1 (X8_S12_C12)) \
    M (67, scm_ref, "scm-ref", DOP1 (X8_S8_S8_S8)) \
    M (68, scm_set, "scm-set!", OP1 (X8_S8_S8_S8)) \
    M (69, scm_ref_tag, "scm-ref/tag", DOP1 (X8_S8_S8_C8)) \
    M (70, scm_set_tag, "scm-set!/tag", OP1 (X8_S8_C8_S8)) \
    M (71, scm_ref_immediate, "scm-ref/immediate", DOP1 (X8_S8_S8_C8)) \
    M (72, scm_set_immediate, "scm-set!/immediate", OP1 (X8_S8_C8_S8)) \
    M (73, word_ref, "word-ref", DOP1 (X8_S8_S8_S8)) \
    M (74, word_set, "word-set!", OP1 (X8_S8_S8_S8)) \
    M (75, word_ref_immediate, "word-ref/immediate", DOP1 (X8_S8_S8_C8)) \
    M (76, word_set_immediate, "word-set!/immediate", OP1 (X8_S8_C8_S8)) \
    M (77, pointer_ref_immediate, "pointer-ref/immediate", DOP1 (X8_S8_S8_C8)) \
    M (78, pointer_set_immediate, "pointer-set!/immediate", OP1 (X8_S8_C8_S8)) \
    M (79, tail_pointer_ref_immediate, "tail-pointer-ref/immediate", DOP1 (X8_S8_S8_C8)) \
    M (80, atomic_scm_ref_immediate, "atomic-scm-ref/immediate", DOP1 (X8_S8_S8_C8)) \
    M (81, atomic_scm_set_immediate, "atomic-scm-set!/immediate", OP1 (X8_S8_C8_S8)) \
    M (82, atomic_scm_swap_immediate, "atomic-scm-swap!/immediate", DOP3 (X8_S24, X8_S24, C8_S24)) \
    M (83, atomic_scm_compare_and_swap_immediate, "atomic-scm-compare-and-swap!/immediate", DOP4 (X8_S24, X8_S24, C8_S24, X8_S24)) \
    M (84, static_ref, "static-ref", DOP2 (X8_S24, R32)) \
    M (85, static_set, "static-set!", OP2 (X8_S24, LO32)) \
    M (86, static_patch, "static-patch!", OP3 (X32, LO32, L32)) \
    M (87, tag_char, "tag-char", DOP1 (X8_S12_S12)) \
    M (88, untag_char, "untag-char", DOP1 (X8_S12_S12)) \
    M (89, tag_fixnum, "tag-fixnum", DOP1 (X8_S12_S12)) \
    M (90, untag_fixnum, "untag-fixnum", DOP1 (X8_S12_S12)) \
    M (91, uadd, "uadd", DOP1 (X8_S8_S8_S8)) \
    M (92, usub, "usub", DOP1 (X8_S8_S8_S8)) \
    M (93, umul, "umul", DOP1 (X8_S8_S8_S8)) \
    M (94, uadd_immediate, "uadd/immediate", DOP1 (X8_S8_S8_C8)) \
    M (95, usub_immediate, "usub/immediate", DOP1 (X8_S8_S8_C8)) \
    M (96, umul_immediate, "umul/immediate", DOP1 (X8_S8_S8_C8)) \
    M (97, ulogand, "ulogand", DOP1 (X8_S8_S8_S8)) \
    M (98, ulogior, "ulogior", DOP1 (X8_S8_S8_S8)) \
    M (99, ulogsub, "ulogsub", DOP1 (X8_S8_S8_S8)) \
    M (100, ulogxor, "ulogxor", DOP1 (X8_S8_S8_S8)) \
    M (101, ursh, "ursh", DOP1 (X8_S8_S8_S8)) \
    M (102, srsh, "srsh", DOP1 (X8_S8_S8_S8)) \
    M (103, ulsh, "ulsh", DOP1 (X8_S8_S8_S8)) \
    M (104, ursh_immediate, "ursh/immediate", DOP1 (X8_S8_S8_C8)) \
    M (105, srsh_immediate, "srsh/immediate", DOP1 (X8_S8_S8_C8)) \
    M (106, ulsh_immediate, "ulsh/immediate", DOP1 (X8_S8_S8_C8)) \
    M (107, fadd, "fadd", DOP1 (X8_S8_S8_S8)) \
    M (108, fsub, "fsub", DOP1 (X8_S8_S8_S8)) \
    M (109, fmul, "fmul", DOP1 (X8_S8_S8_S8)) \
    M (110, fdiv, "fdiv", DOP1 (X8_S8_S8_S8)) \
    M (111, u64_numerically_equal, "u64=?", OP1 (X8_S12_S12)) \
    M (112, u64_less, "u64<?", OP1 (X8_S12_S12)) \
    M (113, s64_less, "s64<?", OP1 (X8_S12_S12)) \
    M (114, s64_imm_numerically_equal, "s64-imm=?", OP1 (X8_S12_Z12)) \
    M (115, u64_imm_less, "u64-imm<?", OP1 (X8_S12_C12)) \
    M (116, imm_u64_less, "imm-u64<?", OP1 (X8_S12_C12)) \
    M (117, s64_imm_less, "s64-imm<?", OP1 (X8_S12_Z12)) \
    M (118, imm_s64_less, "imm-s64<?", OP1 (X8_S12_Z12)) \
    M (119, f64_numerically_equal, "f64=?", OP1 (X8_S12_S12)) \
    M (120, f64_less, "f64<?", OP1 (X8_S12_S12)) \
    M (121, numerically_equal, "=?", OP1 (X8_S12_S12)) \
    M (122, heap_numbers_equal, "heap-numbers-equal?", OP1 (X8_S12_S12)) \
    M (123, less, "<?", OP1 (X8_S12_S12)) \
    M (124, immediate_tag_equals, "immediate-tag=?", OP2 (X8_S24, C16_C16)) \
    M (125, heap_tag_equals, "heap-tag=?", OP2 (X8_S24, C16_C16)) \
    M (126, eq, "eq?", OP1 (X8_S12_S12)) \
    M (127, j, "j", OP1 (X8_L24)) \
    M (128, jl, "jl", OP1 (X8_L24)) \
    M (129, je, "je", OP1 (X8_L24)) \
    M (130, jnl, "jnl", OP1 (X8_L24)) \
    M (131, jne, "jne", OP1 (X8_L24)) \
    M (132, jge, "jge", OP1 (X8_L24)) \
    M (133, jnge, "jnge", OP1 (X8_L24)) \
    M (134, u8_ref, "u8-ref", DOP1 (X8_S8_S8_S8)) \
    M (135, u16_ref, "u16-ref", DOP1 (X8_S8_S8_S8)) \
    M (136, u32_ref, "u32-ref", DOP1 (X8_S8_S8_S8)) \
    M (137, u64_ref, "u64-ref", DOP1 (X8_S8_S8_S8)) \
    M (138, u8_set, "u8-set!", OP1 (X8_S8_S8_S8)) \
    M (139, u16_set, "u16-set!", OP1 (X8_S8_S8_S8)) \
    M (140, u32_set, "u32-set!", OP1 (X8_S8_S8_S8)) \
    M (141, u64_set, "u64-set!", OP1 (X8_S8_S8_S8)) \
    M (142, s8_ref, "s8-ref", DOP1 (X8_S8_S8_S8)) \
    M (143, s16_ref, "s16-ref", DOP1 (X8_S8_S8_S8)) \
    M (144, s32_ref, "s32-ref", DOP1 (X8_S8_S8_S8)) \
    M (145, s64_ref, "s64-ref", DOP1 (X8_S8_S8_S8)) \
    M (146, s8_set, "s8-set!", OP1 (X8_S8_S8_S8)) \
    M (147, s16_set, "s16-set!", OP1 (X8_S8_S8_S8)) \
    M (148, s32_set, "s32-set!", OP1 (X8_S8_S8_S8)) \
    M (149, s64_set, "s64-set!", OP1 (X8_S8_S8_S8)) \
    M (150, f32_ref, "f32-ref", DOP1 (X8_S8_S8_S8)) \
    M (151, f64_ref, "f64-ref", DOP1 (X8_S8_S8_S8)) \
    M (152, f32_set, "f32-set!", OP1 (X8_S8_S8_S8)) \
    M (153, f64_set, "f64-set!", OP1 (X8_S8_S8_S8)) \
    M (154, bind_optionals, "bind-optionals", DOP1 (X8_F24)) \
    M (155, call_f64_from_f64, "call-f64<-f64", DOP2 (X8_S12_S12, C32)) \
    M (156, call_f64_from_f64_f64, "call-f64<-f64-f64", DOP2 (X8_S8_S8_S8, C32)) \
    M (157, allocate_pointerless_words, "allocate-pointerless-words", DOP1 (X8_S12_S12)) \
    M (158, allocate_pointerless_words_immediate, "allocate-pointerless-words/immediate", DOP1 (X8_S12_C12)) \
    M (159, s64_to_f64, "s64->f64", DOP1 (X8_S12_S12)) \
    M (160, call_scm_scm, "call-scm-scm", OP2 (X8_S12_S12, C32)) \
    M (161, call_scm_scm_scm, "call-scm-scm-scm", OP2 (X8_S8_S8_S8, C32)) \
    M (162, call_scm_uimm_scm, "call-scm-uimm-scm", OP2 (X8_S8_C8_S8, C32)) \
    M (163, jtable, "jtable", OP2 (X8_S24, V32_X8_L24)) \
    M (164, make_immediate, "make-immediate", DOP1 (X8_S8_ZI16)) \
    M (165, eq_immediate, "eq-immediate?", OP1 (X8_S8_ZI16)) \
    M (166, call_scm_from_scmn_scmn, "call-scm<-scmn-scmn", DOP4 (X8_S24, N32, N32, C32)) \
    M (167, unused_167, NULL, NOP) \
    M (168, unused_168, NULL, NOP) \
    M (169, unused_169, NULL, NOP) \
    M (170, unused_170, NULL, NOP) \
    M (171, unused_171, NULL, NOP) \
    M (172, unused_172, NULL, NOP) \
    M (173, unused_173, NULL, NOP) \
    M (174, unused_174, NULL, NOP) \
    M (175, unused_175, NULL, NOP) \
    M (176, unused_176, NULL, NOP) \
    M (177, unused_177, NULL, NOP) \
    M (178, unused_178, NULL, NOP) \
    M (179, unused_179, NULL, NOP) \
    M (180, unused_180, NULL, NOP) \
    M (181, unused_181, NULL, NOP) \
    M (182, unused_182, NULL, NOP) \
    M (183, unused_183, NULL, NOP) \
    M (184, unused_184, NULL, NOP) \
    M (185, unused_185, NULL, NOP) \
    M (186, unused_186, NULL, NOP) \
    M (187, unused_187, NULL, NOP) \
    M (188, unused_188, NULL, NOP) \
    M (189, unused_189, NULL, NOP) \
    M (190, unused_190, NULL, NOP) \
    M (191, unused_191, NULL, NOP) \
    M (192, unused_192, NULL, NOP) \
    M (193, unused_193, NULL, NOP) \
    M (194, unused_194, NULL, NOP) \
    M (195, unused_195, NULL, NOP) \
    M (196, unused_196, NULL, NOP) \
    M (197, unused_197, NULL, NOP) \
    M (198, unused_198, NULL, NOP) \
    M (199, unused_199, NULL, NOP) \
    M (200, unused_200, NULL, NOP) \
    M (201, unused_201, NULL, NOP) \
    M (202, unused_202, NULL, NOP) \
    M (203, unused_203, NULL, NOP) \
    M (204, unused_204, NULL, NOP) \
    M (205, unused_205, NULL, NOP) \
    M (206, unused_206, NULL, NOP) \
    M (207, unused_207, NULL, NOP) \
    M (208, unused_208, NULL, NOP) \
    M (209, unused_209, NULL, NOP) \
    M (210, unused_210, NULL, NOP) \
    M (211, unused_211, NULL, NOP) \
    M (212, unused_212, NULL, NOP) \
    M (213, unused_213, NULL, NOP) \
    M (214, unused_214, NULL, NOP) \
    M (215, unused_215, NULL, NOP) \
    M (216, unused_216, NULL, NOP) \
    M (217, unused_217, NULL, NOP) \
    M (218, unused_218, NULL, NOP) \
    M (219, unused_219, NULL, NOP) \
    M (220, unused_220, NULL, NOP) \
    M (221, unused_221, NULL, NOP) \
    M (222, unused_222, NULL, NOP) \
    M (223, unused_223, NULL, NOP) \
    M (224, unused_224, NULL, NOP) \
    M (225, unused_225, NULL, NOP) \
    M (226, unused_226, NULL, NOP) \
    M (227, unused_227, NULL, NOP) \
    M (228, unused_228, NULL, NOP) \
    M (229, unused_229, NULL, NOP) \
    M (230, unused_230, NULL, NOP) \
    M (231, unused_231, NULL, NOP) \
    M (232, unused_232, NULL, NOP) \
    M (233, unused_233, NULL, NOP) \
    M (234, unused_234, NULL, NOP) \
    M (235, unused_235, NULL, NOP) \
    M (236, unused_236, NULL, NOP) \
    M (237, unused_237, NULL, NOP) \
    M (238, unused_238, NULL, NOP) \
    M (239, unused_239, NULL, NOP) \
    M (240, unused_240, NULL, NOP) \
    M (241, unused_241, NULL, NOP) \
    M (242, unused_242, NULL, NOP) \
    M (243, unused_243, NULL, NOP) \
    M (244, unused_244, NULL, NOP) \
    M (245, unused_245, NULL, NOP) \
    M (246, unused_246, NULL, NOP) \
    M (247, unused_247, NULL, NOP) \
    M (248, unused_248, NULL, NOP) \
    M (249, unused_249, NULL, NOP) \
    M (250, unused_250, NULL, NOP) \
    M (251, unused_251, NULL, NOP) \
    M (252, unused_252, NULL, NOP) \
    M (253, unused_253, NULL, NOP) \
    M (254, unused_254, NULL, NOP) \
    M (255, unused_255, NULL, NOP) \

