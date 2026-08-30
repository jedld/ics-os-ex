/* Type information for config/i386/i386.c.
   Copyright (C) 2004, 2007, 2009 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* This file is machine generated.  Do not edit.  */

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_i386_h[] = {
  {
    &ix86_builtins[0],
    1 * ((int) IX86_BUILTIN_MAX),
    sizeof (ix86_builtins[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ix86_builtin_func_type_tab[0],
    1 * ((int) IX86_BT_LAST_ALIAS + 1),
    sizeof (ix86_builtin_func_type_tab[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ix86_builtin_type_tab[0],
    1 * ((int) IX86_BT_LAST_CPTR + 1),
    sizeof (ix86_builtin_type_tab[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ix86_tls_module_base_symbol,
    1,
    sizeof (ix86_tls_module_base_symbol),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &ix86_tls_symbol,
    1,
    sizeof (ix86_tls_symbol),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &split_stack_fn_large,
    1,
    sizeof (split_stack_fn_large),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &split_stack_fn,
    1,
    sizeof (split_stack_fn),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &queued_cfa_restores,
    1,
    sizeof (queued_cfa_restores),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &ix86_previous_fndecl,
    1,
    sizeof (ix86_previous_fndecl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ms_va_list_type_node,
    1,
    sizeof (ms_va_list_type_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &sysv_va_list_type_node,
    1,
    sizeof (sysv_va_list_type_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_cache_tab gt_ggc_rc_gt_i386_h[] = {
  {
    &dllimport_map,
    1,
    sizeof (dllimport_map),
    &gt_ggc_mx_tree_map,
    &gt_pch_nx_tree_map,
    &tree_map_marked_p
  },
  LAST_GGC_CACHE_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rc_gt_i386_h[] = {
  {
    &dllimport_map,
    1,
    sizeof (dllimport_map),
    &gt_ggc_m_P8tree_map4htab,
    &gt_pch_n_P8tree_map4htab
  },
  LAST_GGC_ROOT_TAB
};

