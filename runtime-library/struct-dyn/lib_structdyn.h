/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gc_export.h"

int
struct_get_dyn_i32(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index);

long long
struct_get_dyn_i64(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index);

float
struct_get_dyn_f32(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index);

double
struct_get_dyn_f64(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index);

void *
struct_get_dyn_anyref(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj,
                      int index);

void *
struct_get_dyn_funcref(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj,
                       int index);

void
struct_set_dyn_i32(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index,
                   int value);

void
struct_set_dyn_i64(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index,
                   long long value);

void
struct_set_dyn_f32(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index,
                   float value);

void
struct_set_dyn_f64(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj, int index,
                   double value);

void
struct_set_dyn_anyref(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj,
                      int index, void *value);

void
struct_set_dyn_funcref(wasm_exec_env_t exec_env, wasm_anyref_obj_t obj,
                       int index, void *value);