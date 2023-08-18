/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gc_export.h"

/* whether the type is struct(struct_func) */
bool
is_ts_closure_type(wasm_module_t wasm_module, wasm_defined_type_t type);

/* whether the type is struct(array_i32) */
bool
is_ts_array_type(wasm_module_t wasm_module, wasm_defined_type_t type);

/* Helper to get common used fields */
int
get_array_length(wasm_struct_obj_t obj);

wasm_array_obj_t
get_array_ref(wasm_struct_obj_t obj);

int
get_array_capacity(wasm_struct_obj_t obj);

uint32_t
get_array_element_size(wasm_array_obj_t obj);

/* Type reflection */
int32_t
get_array_type_by_element(wasm_module_t wasm_module,
                          wasm_ref_type_t *element_ref_type, bool is_mutable,
                          wasm_array_type_t *p_array_type);

int32_t
get_array_struct_type(wasm_module_t wasm_module, int32_t array_type_idx,
                      wasm_struct_type_t *p_struct_type);

/* get string struct type*/
int32_t
get_string_struct_type(wasm_module_t wasm_module,
                       wasm_struct_type_t *p_struct_type);

/* get string array type*/
int32_t
get_string_array_type(wasm_module_t wasm_module,
                      wasm_array_type_t *p_array_type_t);

bool
is_ts_string_type(wasm_module_t wasm_module, wasm_defined_type_t type);

/* create wasm string from c string*/
wasm_struct_obj_t
create_wasm_string(wasm_exec_env_t exec_env, const char *value);

/* whether the type is struct(i32_i32_anyref) */
bool
is_infc(wasm_obj_t obj);

/* try to get object that an interface point to */
void *
get_infc_obj(wasm_exec_env_t exec_env, wasm_obj_t obj);

/* combine elements of an array to an string */
void *
array_to_string(wasm_exec_env_t exec_env, void *ctx, void *obj,
                void *separator);