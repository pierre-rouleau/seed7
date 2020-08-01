/********************************************************************/
/*                                                                  */
/*  hi   Interpreter for Seed7 programs.                            */
/*  Copyright (C) 1990 - 2007  Thomas Mertes                        */
/*                                                                  */
/*  This program is free software; you can redistribute it and/or   */
/*  modify it under the terms of the GNU General Public License as  */
/*  published by the Free Software Foundation; either version 2 of  */
/*  the License, or (at your option) any later version.             */
/*                                                                  */
/*  This program is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   */
/*  GNU General Public License for more details.                    */
/*                                                                  */
/*  You should have received a copy of the GNU General Public       */
/*  License along with this program; if not, write to the           */
/*  Free Software Foundation, Inc., 51 Franklin Street,             */
/*  Fifth Floor, Boston, MA  02110-1301, USA.                       */
/*                                                                  */
/*  Module: Main                                                    */
/*  File: seed7/src/primitiv.c                                      */
/*  Changes: 1992, 1993, 1994, 2004  Thomas Mertes                  */
/*  Content: Table definitions for all primitive actions.           */
/*                                                                  */
/********************************************************************/

#include "version.h"

#include "stdio.h"

#include "common.h"
#include "data.h"
#include "actutl.h"
#include "actlib.h"
#include "arrlib.h"
#include "biglib.h"
#include "blnlib.h"
#include "bstlib.h"
#include "chrlib.h"
#include "clslib.h"
#include "cmdlib.h"
#include "dcllib.h"
#include "drwlib.h"
#include "enulib.h"
#include "fillib.h"
#include "fltlib.h"
#include "hshlib.h"
#include "intlib.h"
#include "kbdlib.h"
#include "lstlib.h"
#include "prclib.h"
#include "prglib.h"
#include "reflib.h"
#include "rfllib.h"
#include "scrlib.h"
#include "sctlib.h"
#include "setlib.h"
#include "soclib.h"
#include "strlib.h"
#include "timlib.h"
#include "typlib.h"
#include "ut8lib.h"

#undef EXTERN
#define EXTERN
#include "primitiv.h"


static primactrecord prim_act_table[] = {
    { "ACT_ILLEGAL",         act_illegal,         },

    { "ACT_CPY",             act_cpy,             },
    { "ACT_CREATE",          act_create,          },
    { "ACT_GEN",             act_gen,             },
    { "ACT_STR",             act_str,             },
    { "ACT_VALUE",           act_value,           },

    { "ARR_APPEND",          arr_append,          },
    { "ARR_ARRLIT",          arr_arrlit,          },
    { "ARR_ARRLIT2",         arr_arrlit2,         },
    { "ARR_BASELIT",         arr_baselit,         },
    { "ARR_BASELIT2",        arr_baselit2,        },
    { "ARR_CAT",             arr_cat,             },
    { "ARR_CONV",            arr_conv,            },
    { "ARR_CPY",             arr_cpy,             },
    { "ARR_CREATE",          arr_create,          },
    { "ARR_DESTR",           arr_destr,           },
    { "ARR_EMPTY",           arr_empty,           },
    { "ARR_EXTEND",          arr_extend,          },
    { "ARR_GEN",             arr_gen,             },
    { "ARR_HEAD",            arr_head,            },
    { "ARR_IDX",             arr_idx,             },
    { "ARR_LNG",             arr_lng,             },
    { "ARR_RANGE",           arr_range,           },
    { "ARR_REMOVE",          arr_remove,          },
    { "ARR_SORT",            arr_sort,            },
    { "ARR_TAIL",            arr_tail,            },
    { "ARR_TIMES",           arr_times,           },

    { "BIG_ABS",             big_abs,             },
    { "BIG_ADD",             big_add,             },
    { "BIG_BIT_LENGTH",      big_bit_length,      },
    { "BIG_CLIT",            big_clit,            },
    { "BIG_CMP",             big_cmp,             },
    { "BIG_CPY",             big_cpy,             },
    { "BIG_CREATE",          big_create,          },
    { "BIG_DECR",            big_decr,            },
    { "BIG_DESTR",           big_destr,           },
    { "BIG_DIV",             big_div,             },
    { "BIG_EQ",              big_eq,              },
    { "BIG_GE",              big_ge,              },
    { "BIG_GROW",            big_grow,            },
    { "BIG_GT",              big_gt,              },
    { "BIG_HASHCODE",        big_hashcode,        },
    { "BIG_ICONV",           big_iconv,           },
    { "BIG_INCR",            big_incr,            },
    { "BIG_IPOW",            big_ipow,            },
    { "BIG_LE",              big_le,              },
    { "BIG_LOG2",            big_log2,            },
    { "BIG_LT",              big_lt,              },
    { "BIG_MCPY",            big_mcpy,            },
    { "BIG_MDIV",            big_mdiv,            },
    { "BIG_MINUS",           big_minus,           },
    { "BIG_MOD",             big_mod,             },
    { "BIG_MULT",            big_mult,            },
    { "BIG_NE",              big_ne,              },
    { "BIG_ODD",             big_odd,             },
    { "BIG_ORD",             big_ord,             },
    { "BIG_PARSE",           big_parse,           },
    { "BIG_PLUS",            big_plus,            },
    { "BIG_PRED",            big_pred,            },
    { "BIG_RAND",            big_rand,            },
    { "BIG_REM",             big_rem,             },
    { "BIG_SBTR",            big_sbtr,            },
    { "BIG_SHRINK",          big_shrink,          },
    { "BIG_STR",             big_str,             },
    { "BIG_SUCC",            big_succ,            },
    { "BIG_VALUE",           big_value,           },

    { "BLN_AND",             bln_and,             },
    { "BLN_CPY",             bln_cpy,             },
    { "BLN_CREATE",          bln_create,          },
    { "BLN_GE",              bln_ge,              },
    { "BLN_GT",              bln_gt,              },
    { "BLN_ICONV",           bln_iconv,           },
    { "BLN_LE",              bln_le,              },
    { "BLN_LT",              bln_lt,              },
    { "BLN_NOT",             bln_not,             },
    { "BLN_OR",              bln_or,              },
    { "BLN_ORD",             bln_ord,             },

    { "BST_CPY",             bst_cpy,             },
    { "BST_CREATE",          bst_create,          },
    { "BST_DESTR",           bst_destr,           },
    { "BST_EMPTY",           bst_empty,           },

    { "CHR_CHR",             chr_chr,             },
    { "CHR_CMP",             chr_cmp,             },
    { "CHR_CONV",            chr_conv,            },
    { "CHR_CPY",             chr_cpy,             },
    { "CHR_CREATE",          chr_create,          },
    { "CHR_DECR",            chr_decr,            },
    { "CHR_EQ",              chr_eq,              },
    { "CHR_GE",              chr_ge,              },
    { "CHR_GT",              chr_gt,              },
    { "CHR_HASHCODE",        chr_hashcode,        },
    { "CHR_ICONV",           chr_iconv,           },
    { "CHR_INCR",            chr_incr,            },
    { "CHR_LE",              chr_le,              },
    { "CHR_LOW",             chr_low,             },
    { "CHR_LT",              chr_lt,              },
    { "CHR_NE",              chr_ne,              },
    { "CHR_ORD",             chr_ord,             },
    { "CHR_PRED",            chr_pred,            },
    { "CHR_STR",             chr_str,             },
    { "CHR_SUCC",            chr_succ,            },
    { "CHR_UP",              chr_up,              },
    { "CHR_VALUE",           chr_value,           },

    { "CLS_CONV2",           cls_conv2,           },
    { "CLS_CPY",             cls_cpy,             },
    { "CLS_CPY2",            cls_cpy2,            },
    { "CLS_CREATE",          cls_create,          },
    { "CLS_CREATE2",         cls_create2,         },
    { "CLS_EQ",              cls_eq,              },
    { "CLS_NE",              cls_ne,              },
    { "CLS_SELECT",          cls_select,          },

    { "CMD_CHDIR",           cmd_chdir,           },
    { "CMD_CONFIG_VALUE",    cmd_config_value,    },
    { "CMD_COPY",            cmd_copy,            },
    { "CMD_FILETYPE",        cmd_filetype,        },
    { "CMD_GETCWD",          cmd_getcwd,          },
    { "CMD_LNG",             cmd_lng,             },
    { "CMD_LS",              cmd_ls,              },
    { "CMD_MKDIR",           cmd_mkdir,           },
    { "CMD_MOVE",            cmd_move,            },
    { "CMD_REMOVE",          cmd_remove,          },
    { "CMD_SH",              cmd_sh,              },
    { "CMD_SYMLINK",         cmd_symlink,         },

    { "DCL_ATTR",            dcl_attr,            },
    { "DCL_CONST",           dcl_const,           },
    { "DCL_ELEMENTS",        dcl_elements,        },
    { "DCL_FWD",             dcl_fwd,             },
    { "DCL_GETFUNC",         dcl_getfunc        },
    { "DCL_GETOBJ",          dcl_getobj,          },
    { "DCL_GLOBAL",          dcl_global,          },
    { "DCL_IN1VAR",          dcl_in1var,          },
    { "DCL_IN2VAR",          dcl_in2var,          },
    { "DCL_INOUT1",          dcl_inout1,          },
    { "DCL_INOUT2",          dcl_inout2,          },
    { "DCL_REF1",            dcl_ref1,            },
    { "DCL_REF2",            dcl_ref2,            },
    { "DCL_SYMB",            dcl_symb,            },
    { "DCL_VAL1",            dcl_val1,            },
    { "DCL_VAL2",            dcl_val2,            },
    { "DCL_VAR",             dcl_var,             },

#ifdef WITH_DRAW
    { "DRW_ARC",             drw_arc,             },
    { "DRW_ARC2",            drw_arc2,            },
    { "DRW_BACKGROUND",      drw_background,      },
    { "DRW_CIRCLE",          drw_circle,          },
    { "DRW_CLEAR",           drw_clear,           },
    { "DRW_COLOR",           drw_color,           },
    { "DRW_COPYAREA",        drw_copyarea,        },
    { "DRW_CPY",             drw_cpy,             },
    { "DRW_CREATE",          drw_create,          },
    { "DRW_DESTR",           drw_destr,           },
    { "DRW_EMPTY",           drw_empty,           },
    { "DRW_EQ",              drw_eq,              },
    { "DRW_FARCCHORD",       drw_farcchord,       },
    { "DRW_FARCPIESLICE",    drw_farcpieslice,    },
    { "DRW_FCIRCLE",         drw_fcircle,         },
    { "DRW_FELLIPSE",        drw_fellipse,        },
    { "DRW_FLUSH",           drw_flush,           },
    { "DRW_GET",             drw_get,             },
    { "DRW_HEIGHT",          drw_height,          },
    { "DRW_IMAGE",           drw_image,           },
    { "DRW_LINE",            drw_line,            },
    { "DRW_NE",              drw_ne,              },
    { "DRW_NEW_PIXMAP",      drw_new_pixmap,      },
    { "DRW_OPEN",            drw_open,            },
    { "DRW_PARC",            drw_parc,            },
    { "DRW_PCIRCLE",         drw_pcircle,         },
    { "DRW_PFARCCHORD",      drw_pfarcchord,      },
    { "DRW_PFARCPIESLICE",   drw_pfarcpieslice,   },
    { "DRW_PFCIRCLE",        drw_pfcircle,        },
    { "DRW_PFELLIPSE",       drw_pfellipse,       },
    { "DRW_PLINE",           drw_pline,           },
    { "DRW_POINT",           drw_point,           },
    { "DRW_PPOINT",          drw_ppoint,          },
    { "DRW_PRECT",           drw_prect,           },
    { "DRW_PUT",             drw_put,             },
    { "DRW_RECT",            drw_rect,            },
    { "DRW_RGBCOL",          drw_rgbcol,          },
    { "DRW_ROT",             drw_rot,             },
    { "DRW_SCALE",           drw_scale,           },
    { "DRW_TEXT",            drw_text,            },
    { "DRW_WIDTH",           drw_width,           },
#endif

    { "ENU_CPY",             enu_cpy,             },
    { "ENU_CREATE",          enu_create,          },
    { "ENU_EQ",              enu_eq,              },
    { "ENU_GENLIT",          enu_genlit,          },
    { "ENU_ICONV2",          enu_iconv2,          },
    { "ENU_NE",              enu_ne,              },
    { "ENU_ORD2",            enu_ord2,            },
    { "ENU_SIZE",            enu_size,            },
    { "ENU_VALUE",           enu_value,           },

    { "FIL_BIG_LNG",         fil_big_lng,         },
    { "FIL_BIG_SEEK",        fil_big_seek,        },
    { "FIL_BIG_TELL",        fil_big_tell,        },
    { "FIL_CLOSE",           fil_close,           },
    { "FIL_CPY",             fil_cpy,             },
    { "FIL_CREATE",          fil_create,          },
    { "FIL_EMPTY",           fil_empty,           },
    { "FIL_EOF",             fil_eof,             },
    { "FIL_EQ",              fil_eq,              },
    { "FIL_ERR",             fil_err,             },
    { "FIL_FLUSH",           fil_flush,           },
    { "FIL_GETC",            fil_getc,            },
    { "FIL_GETS",            fil_gets,            },
    { "FIL_IN",              fil_in,              },
    { "FIL_LINE_READ",       fil_line_read,       },
    { "FIL_LIT",             fil_lit,             },
    { "FIL_LNG",             fil_lng,             },
    { "FIL_NE",              fil_ne,              },
    { "FIL_OPEN",            fil_open,            },
    { "FIL_OUT",             fil_out,             },
    { "FIL_POPEN",           fil_popen,           },
    { "FIL_SEEK",            fil_seek,            },
    { "FIL_TELL",            fil_tell,            },
    { "FIL_VALUE",           fil_value,           },
    { "FIL_WORD_READ",       fil_word_read,       },
    { "FIL_WRITE",           fil_write,           },

#ifdef WITH_FLOAT
    { "FLT_A2TAN",           flt_a2tan,           },
    { "FLT_ABS",             flt_abs,             },
    { "FLT_ACOS",            flt_acos,            },
    { "FLT_ADD",             flt_add,             },
    { "FLT_ASIN",            flt_asin,            },
    { "FLT_ATAN",            flt_atan,            },
    { "FLT_CEIL",            flt_ceil,            },
    { "FLT_CMP",             flt_cmp,             },
    { "FLT_COS",             flt_cos,             },
    { "FLT_CPY",             flt_cpy,             },
    { "FLT_CREATE",          flt_create,          },
    { "FLT_DGTS",            flt_dgts,            },
    { "FLT_DIV",             flt_div,             },
    { "FLT_EQ",              flt_eq,              },
    { "FLT_EXP",             flt_exp,             },
    { "FLT_FLOOR",           flt_floor,           },
    { "FLT_GE",              flt_ge,              },
    { "FLT_GROW",            flt_grow,            },
    { "FLT_GT",              flt_gt,              },
    { "FLT_HASHCODE",        flt_hashcode,        },
    { "FLT_HCOS",            flt_hcos,            },
    { "FLT_HSIN",            flt_hsin,            },
    { "FLT_HTAN",            flt_htan,            },
    { "FLT_IFLT",            flt_iflt,            },
    { "FLT_IPOW",            flt_ipow,            },
    { "FLT_ISNAN",           flt_isnan,           },
    { "FLT_LE",              flt_le,              },
    { "FLT_LOG",             flt_log,             },
    { "FLT_LOG10",           flt_log10,           },
    { "FLT_LT",              flt_lt,              },
    { "FLT_MCPY",            flt_mcpy,            },
    { "FLT_MINUS",           flt_minus,           },
    { "FLT_MULT",            flt_mult,            },
    { "FLT_NE",              flt_ne,              },
    { "FLT_PARSE",           flt_parse,           },
    { "FLT_PLUS",            flt_plus,            },
    { "FLT_POW",             flt_pow,             },
    { "FLT_RAND",            flt_rand,            },
    { "FLT_ROUND",           flt_round,           },
    { "FLT_SBTR",            flt_sbtr,            },
    { "FLT_SHRINK",          flt_shrink,          },
    { "FLT_SIN",             flt_sin,             },
    { "FLT_SQRT",            flt_sqrt,            },
    { "FLT_STR",             flt_str,             },
    { "FLT_TAN",             flt_tan,             },
    { "FLT_TRUNC",           flt_trunc,           },
    { "FLT_VALUE",           flt_value,           },
#endif

#ifdef WITH_DRAW
    { "GKB_BUSY_GETC",       gkb_busy_getc,       },
    { "GKB_GETC",            gkb_getc           },
    { "GKB_GETS",            gkb_gets           },
    { "GKB_KEYPRESSED",      gkb_keypressed     },
    { "GKB_LINE_READ",       gkb_line_read      },
    { "GKB_RAW_GETC",        gkb_raw_getc       },
    { "GKB_WORD_READ",       gkb_word_read      },
    { "GKB_XPOS",            gkb_xpos           },
    { "GKB_YPOS",            gkb_ypos           },
#endif

    { "HSH_CONTAINS",        hsh_contains,        },
    { "HSH_CPY",             hsh_cpy,             },
    { "HSH_CREATE",          hsh_create,          },
    { "HSH_DESTR",           hsh_destr,           },
    { "HSH_EMPTY",           hsh_empty,           },
    { "HSH_EXCL",            hsh_excl,            },
    { "HSH_FOR",             hsh_for,             },
    { "HSH_FOR_DATA_KEY",    hsh_for_data_key,    },
    { "HSH_FOR_KEY",         hsh_for_key,         },
    { "HSH_IDX",             hsh_idx,             },
    { "HSH_IDX2",            hsh_idx2,            },
    { "HSH_INCL",            hsh_incl,            },
    { "HSH_KEYS",            hsh_keys,            },
    { "HSH_LNG",             hsh_lng,             },
    { "HSH_REFIDX",          hsh_refidx,          },
    { "HSH_VALUES",          hsh_values,          },

    { "INT_ABS",             int_abs,             },
    { "INT_ADD",             int_add,             },
    { "INT_BINOM",           int_binom,           },
    { "INT_CMP",             int_cmp,             },
    { "INT_CONV",            int_conv,            },
    { "INT_CPY",             int_cpy,             },
    { "INT_CREATE",          int_create,          },
    { "INT_DECR",            int_decr,            },
    { "INT_DIV",             int_div,             },
    { "INT_EQ",              int_eq,              },
    { "INT_FACT",            int_fact,            },
    { "INT_GE",              int_ge,              },
    { "INT_GROW",            int_grow,            },
    { "INT_GT",              int_gt,              },
    { "INT_HASHCODE",        int_hashcode,        },
    { "INT_INCR",            int_incr,            },
    { "INT_LE",              int_le,              },
    { "INT_LOG2",            int_log2,            },
    { "INT_LT",              int_lt,              },
    { "INT_MCPY",            int_mcpy,            },
    { "INT_MDIV",            int_mdiv,            },
    { "INT_MINUS",           int_minus,           },
    { "INT_MOD",             int_mod,             },
    { "INT_MULT",            int_mult,            },
    { "INT_NE",              int_ne,              },
    { "INT_ODD",             int_odd,             },
    { "INT_ORD",             int_ord,             },
    { "INT_PARSE",           int_parse,           },
    { "INT_PLUS",            int_plus,            },
    { "INT_POW",             int_pow,             },
    { "INT_PRED",            int_pred,            },
    { "INT_RAND",            int_rand,            },
    { "INT_REM",             int_rem,             },
    { "INT_SBTR",            int_sbtr,            },
    { "INT_SHRINK",          int_shrink,          },
    { "INT_SQRT",            int_sqrt,            },
    { "INT_STR",             int_str,             },
    { "INT_SUCC",            int_succ,            },
    { "INT_VALUE",           int_value,           },

    { "KBD_BUSY_GETC",       kbd_busy_getc,       },
    { "KBD_GETC",            kbd_getc,            },
    { "KBD_GETS",            kbd_gets,            },
    { "KBD_KEYPRESSED",      kbd_keypressed,      },
    { "KBD_LINE_READ",       kbd_line_read,       },
    { "KBD_RAW_GETC",        kbd_raw_getc,        },
    { "KBD_WORD_READ",       kbd_word_read,       },

    { "LST_CAT",             lst_cat,             },
    { "LST_CPY",             lst_cpy,             },
    { "LST_CREATE",          lst_create,          },
    { "LST_DESTR",           lst_destr,           },
    { "LST_ELEM",            lst_elem,            },
    { "LST_EMPTY",           lst_empty,           },
    { "LST_EXCL",            lst_excl,            },
    { "LST_HEAD",            lst_head,            },
    { "LST_IDX",             lst_idx,             },
    { "LST_INCL",            lst_incl,            },
    { "LST_LNG",             lst_lng,             },
    { "LST_RANGE",           lst_range,           },
    { "LST_TAIL",            lst_tail,            },

    { "PRC_ARGS",            prc_args,            },
    { "PRC_BEGIN",           prc_begin,           },
    { "PRC_BLOCK",           prc_block,           },
    { "PRC_BLOCK_DEF",       prc_block_def,       },
    { "PRC_CASE",            prc_case,            },
    { "PRC_CASE_DEF",        prc_case_def,        },
    { "PRC_CPY",             prc_cpy,             },
    { "PRC_CREATE",          prc_create,          },
    { "PRC_DECLS",           prc_decls,           },
    { "PRC_DYNAMIC",         prc_dynamic,         },
    { "PRC_EXIT",            prc_exit,            },
    { "PRC_FOR_DOWNTO",      prc_for_downto,      },
    { "PRC_FOR_TO",          prc_for_to,          },
    { "PRC_GETENV",          prc_getenv,          },
    { "PRC_HEAPSTAT",        prc_heapstat,        },
    { "PRC_HSIZE",           prc_hsize,           },
    { "PRC_IF",              prc_if,              },
    { "PRC_IF_ELSIF",        prc_if_elsif,        },
    { "PRC_INCLUDE",         prc_include,         },
    { "PRC_LOCAL",           prc_local,           },
    { "PRC_NOOP",            prc_noop,            },
    { "PRC_PRINT",           prc_print,           },
    { "PRC_RAISE",           prc_raise,           },
    { "PRC_REPEAT",          prc_repeat,          },
    { "PRC_RES_BEGIN",       prc_res_begin,       },
    { "PRC_RES_LOCAL",       prc_res_local,       },
    { "PRC_RETURN",          prc_return,          },
    { "PRC_RETURN2",         prc_return2,         },
    { "PRC_SETTRACE",        prc_settrace,        },
    { "PRC_TRACE",           prc_trace,           },
    { "PRC_VARFUNC",         prc_varfunc,         },
    { "PRC_VARFUNC2",        prc_varfunc2,        },
    { "PRC_WHILE",           prc_while,           },

    { "PRG_ANALYZE",         prg_analyze,         },
    { "PRG_CPY",             prg_cpy,             },
    { "PRG_CREATE",          prg_create,          },
    { "PRG_DECL_OBJECTS",    prg_decl_objects,    },
    { "PRG_DESTR",           prg_destr,           },
    { "PRG_EMPTY",           prg_empty,           },
    { "PRG_EQ",              prg_eq,              },
    { "PRG_ERROR_COUNT",     prg_error_count,     },
    { "PRG_EVAL",            prg_eval,            },
    { "PRG_EXEC",            prg_exec,            },
    { "PRG_FIND",            prg_find,            },
    { "PRG_MATCH",           prg_match,           },
    { "PRG_NAME",            prg_name,            },
    { "PRG_NE",              prg_ne,              },
    { "PRG_PROG",            prg_prog,            },
    { "PRG_SYOBJECT",        prg_syobject,        },
    { "PRG_SYSVAR",          prg_sysvar,          },
    { "PRG_S_ANALYZE",       prg_s_analyze,       },
    { "PRG_VALUE",           prg_value,           },

#ifdef WITH_REFERENCE
    { "REF_ADDR",            ref_addr,            },
    { "REF_ALLOC",           ref_alloc,           },
    { "REF_ARRMINPOS",       ref_arrminpos,       },
    { "REF_ARRTOLIST",       ref_arrtolist,       },
    { "REF_BODY",            ref_body,            },
    { "REF_BUILD",           ref_build,           },
    { "REF_CATEGORY",        ref_category,        },
    { "REF_CMP",             ref_cmp,             },
    { "REF_CONTENT",         ref_content,         },
    { "REF_CONV",            ref_conv,            },
    { "REF_CPY",             ref_cpy,             },
    { "REF_CREATE",          ref_create,          },
    { "REF_DEREF",           ref_deref,           },
    { "REF_EQ",              ref_eq,              },
    { "REF_FIND",            ref_find,            },
    { "REF_HASHCODE",        ref_hashcode,        },
    { "REF_ISSYMB",          ref_issymb,          },
    { "REF_ISVAR",           ref_isvar,           },
    { "REF_ITFTOSCT",        ref_itftosct,        },
    { "REF_LOCAL_CONSTS",    ref_local_consts,    },
    { "REF_LOCAL_VARS",      ref_local_vars,      },
    { "REF_LOCINI",          ref_locini,          },
    { "REF_MKREF",           ref_mkref,           },
    { "REF_NAME",            ref_name,            },
    { "REF_NE",              ref_ne,              },
    { "REF_NIL",             ref_nil,             },
    { "REF_NUM",             ref_num,             },
    { "REF_PARAMS",          ref_params,          },
    { "REF_PROG",            ref_prog,            },
    { "REF_RESINI",          ref_resini,          },
    { "REF_RESULT",          ref_result,          },
    { "REF_SCAN",            ref_scan,            },
    { "REF_SCTTOLIST",       ref_scttolist,       },
    { "REF_SELECT",          ref_select,          },
    { "REF_SETCATEGORY",     ref_setcategory,     },
    { "REF_SETPARAMS",       ref_setparams,       },
    { "REF_SETTYPE",         ref_settype,         },
    { "REF_STR",             ref_str,             },
    { "REF_SYMB",            ref_symb,            },
    { "REF_TRACE",           ref_trace,           },
    { "REF_TYPE",            ref_type,            },
    { "REF_VALUE",           ref_value,           },

    { "RFL_APPEND",          rfl_append,          },
    { "RFL_CAT",             rfl_cat,             },
    { "RFL_CPY",             rfl_cpy,             },
    { "RFL_CREATE",          rfl_create,          },
    { "RFL_DESTR",           rfl_destr,           },
    { "RFL_ELEM",            rfl_elem,            },
    { "RFL_ELEMCPY",         rfl_elemcpy,         },
    { "RFL_EMPTY",           rfl_empty,           },
    { "RFL_EQ",              rfl_eq,              },
    { "RFL_EXCL",            rfl_excl,            },
    { "RFL_EXPR",            rfl_expr,            },
    { "RFL_FOR",             rfl_for,             },
    { "RFL_HEAD",            rfl_head,            },
    { "RFL_IDX",             rfl_idx,             },
    { "RFL_INCL",            rfl_incl,            },
    { "RFL_LNG",             rfl_lng,             },
    { "RFL_MKLIST",          rfl_mklist,          },
    { "RFL_NE",              rfl_ne,              },
    { "RFL_POS",             rfl_pos,             },
    { "RFL_RANGE",           rfl_range,           },
    { "RFL_SETVALUE",        rfl_setvalue,        },
    { "RFL_TAIL",            rfl_tail,            },
    { "RFL_TRACE",           rfl_trace,           },
    { "RFL_VALUE",           rfl_value,           },
#endif

    { "SCR_CLEAR",           scr_clear,           },
    { "SCR_CURSOR",          scr_cursor,          },
    { "SCR_FLUSH",           scr_flush,           },
    { "SCR_HEIGHT",          scr_height,          },
    { "SCR_H_SCL",           scr_h_scl,           },
    { "SCR_OPEN",            scr_open,            },
    { "SCR_SETPOS",          scr_setpos,          },
    { "SCR_V_SCL",           scr_v_scl,           },
    { "SCR_WIDTH",           scr_width,           },
    { "SCR_WRITE",           scr_write,           },

    { "SCT_ALLOC",           sct_alloc,           },
    { "SCT_CAT",             sct_cat,             },
    { "SCT_CONV",            sct_conv,            },
    { "SCT_CPY",             sct_cpy,             },
    { "SCT_CREATE",          sct_create,          },
    { "SCT_DESTR",           sct_destr,           },
    { "SCT_ELEM",            sct_elem,            },
    { "SCT_EMPTY",           sct_empty,           },
    { "SCT_INCL",            sct_incl,            },
    { "SCT_LNG",             sct_lng,             },
    { "SCT_REFIDX",          sct_refidx,          },
    { "SCT_SELECT",          sct_select,          },

    { "SET_ARRLIT",          set_arrlit,          },
    { "SET_BASELIT",         set_baselit,         },
    { "SET_CARD",            set_card,            },
    { "SET_CMP",             set_cmp,             },
    { "SET_CONV",            set_conv,            },
    { "SET_CPY",             set_cpy,             },
    { "SET_CREATE",          set_create,          },
    { "SET_DESTR",           set_destr,           },
    { "SET_DIFF",            set_diff,            },
    { "SET_ELEM",            set_elem,            },
    { "SET_EMPTY",           set_empty,           },
    { "SET_EQ",              set_eq,              },
    { "SET_EXCL",            set_excl,            },
    { "SET_GE",              set_ge,              },
    { "SET_GT",              set_gt,              },
    { "SET_HASHCODE",        set_hashcode,        },
    { "SET_INCL",            set_incl,            },
    { "SET_INTERSECT",       set_intersect,       },
    { "SET_LE",              set_le,              },
    { "SET_LT",              set_lt,              },
    { "SET_MAX",             set_max,             },
    { "SET_MIN",             set_min,             },
    { "SET_NE",              set_ne,              },
    { "SET_NOT_ELEM",        set_not_elem,        },
    { "SET_RAND",            set_rand,            },
    { "SET_SYMDIFF",         set_symdiff,         },
    { "SET_UNION",           set_union,           },
    { "SET_VALUE",           set_value,           },

    { "SOC_ACCEPT",          soc_accept,          },
    { "SOC_BIND",            soc_bind,            },
    { "SOC_CLOSE",           soc_close,           },
    { "SOC_CONNECT",         soc_connect,         },
    { "SOC_CPY",             soc_cpy,             },
    { "SOC_CREATE",          soc_create,          },
    { "SOC_EMPTY",           soc_empty,           },
    { "SOC_EQ",              soc_eq,              },
    { "SOC_GETC",            soc_getc,            },
    { "SOC_GETS",            soc_gets,            },
    { "SOC_INET_ADDR",       soc_inet_addr,       },
    { "SOC_INET_LOCAL_ADDR", soc_inet_local_addr, },
    { "SOC_INET_SERV_ADDR",  soc_inet_serv_addr,  },
    { "SOC_LINE_READ",       soc_line_read,       },
    { "SOC_LISTEN",          soc_listen,          },
    { "SOC_NE",              soc_ne,              },
    { "SOC_RECV",            soc_recv,            },
    { "SOC_RECVFROM",        soc_recvfrom,        },
    { "SOC_SEND",            soc_send,            },
    { "SOC_SENDTO",          soc_sendto,          },
    { "SOC_SOCKET",          soc_socket,          },
    { "SOC_WORD_READ",       soc_word_read,       },
    { "SOC_WRITE",           soc_write,           },

    { "STR_APPEND",          str_append,          },
    { "STR_CAT",             str_cat,             },
    { "STR_CHSPLIT",         str_chsplit,         },
    { "STR_CLIT",            str_clit,            },
    { "STR_CMP",             str_cmp,             },
    { "STR_CNT",             act_illegal,         },
    { "STR_CPY",             str_cpy,             },
    { "STR_CREATE",          str_create,          },
    { "STR_DESTR",           str_destr,           },
    { "STR_ELEMCPY",         str_elemcpy,         },
    { "STR_EQ",              str_eq,              },
    { "STR_GE",              str_ge,              },
    { "STR_GT",              str_gt,              },
    { "STR_HASHCODE",        str_hashcode,        },
    { "STR_HEAD",            str_head,            },
    { "STR_IDX",             str_idx,             },
    { "STR_IPOS",            str_ipos,            },
    { "STR_LE",              str_le,              },
    { "STR_LIT",             str_lit,             },
    { "STR_LNG",             str_lng,             },
    { "STR_LOW",             str_low,             },
    { "STR_LPAD",            str_lpad,            },
    { "STR_LT",              str_lt,              },
    { "STR_MULT",            str_mult,            },
    { "STR_NE",              str_ne,              },
    { "STR_POS",             str_pos,             },
    { "STR_RANGE",           str_range,           },
    { "STR_REPL",            str_repl,            },
    { "STR_RPAD",            str_rpad,            },
    { "STR_RPOS",            str_rpos,            },
    { "STR_SPLIT",           str_split,           },
    { "STR_STR",             str_str,             },
    { "STR_SUBSTR",          str_substr,          },
    { "STR_TAIL",            str_tail,            },
    { "STR_TRIM",            str_trim,            },
    { "STR_UP",              str_up,              },
    { "STR_VALUE",           str_value,           },

    { "TIM_AWAIT",           tim_await,           },
    { "TIM_NOW",             tim_now,             },


    { "TYP_ADDINTERFACE",    typ_addinterface,    },
    { "TYP_CMP",             typ_cmp,             },
    { "TYP_CPY",             typ_cpy,             },
    { "TYP_CREATE",          typ_create,          },
    { "TYP_DESTR",           typ_destr,           },
    { "TYP_EQ",              typ_eq,              },
    { "TYP_FUNC",            typ_func,            },
    { "TYP_GENSUB",          typ_gensub,          },
    { "TYP_GENTYPE",         typ_gentype,         },
    { "TYP_HASHCODE",        typ_hashcode,        },
    { "TYP_ISDECLARED",      typ_isdeclared,      },
    { "TYP_ISDERIVED",       typ_isderived,       },
    { "TYP_ISFORWARD",       typ_isforward,       },
    { "TYP_ISFUNC",          typ_isfunc,          },
    { "TYP_ISVARFUNC",       typ_isvarfunc,       },
    { "TYP_MATCHOBJ",        typ_matchobj,        },
    { "TYP_META",            typ_meta,            },
    { "TYP_NE",              typ_ne,              },
    { "TYP_NUM",             typ_num,             },
    { "TYP_RESULT",          typ_result,          },
    { "TYP_STR",             typ_str,             },
    { "TYP_VALUE",           typ_value,           },
    { "TYP_VARCONV",         typ_varconv,         },
    { "TYP_VARFUNC",         typ_varfunc,         },

    { "UT8_GETC",            ut8_getc,            },
    { "UT8_GETS",            ut8_gets,            },
    { "UT8_LINE_READ",       ut8_line_read,       },
    { "UT8_SEEK",            ut8_seek,            },
    { "UT8_WORD_READ",       ut8_word_read,       },
    { "UT8_WRITE",           ut8_write,           }
  };



#ifdef ANSI_C

void init_primitiv (void)
#else

void init_primitiv ()
#endif

  { /* init_primitiv */
    act_table.size = sizeof(prim_act_table) / sizeof(primactrecord);
    act_table.primitive = prim_act_table;
  } /* init_primitiv */
