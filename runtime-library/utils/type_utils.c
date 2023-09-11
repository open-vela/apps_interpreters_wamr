/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "wasm.h"
#include "type_utils.h"
#include "quickjs.h"
#include "dyntype.h"
#include "wamr_utils.h"

#define OFFSET_OF_COUNT 4
#define OFFSET_OF_META_FIELDS 8
#define SIZEOF_META_FIELD 12

#define META_FLAG_MASK 0x0000000F
#define META_INDEX_MASK 0xFFFFFFF0

#define OFFSET_OF_FIELD_FLAG_AND_INDEX 4
#define OFFSET_OF_FIELD_TYPE 8

/** start type id of custom type */
#define CUSTOM_TYPE_BEGIN 1052
/*
    utilities for closure object

    * closure struct (WasmGC struct)
    +----------+      +---------------------------+
    | 0:context|----->|           context         |
    +----------+      +---------------------------+
    |  1:func  |      |            func           |
    +----------+      +---------------------------+
*/
bool
is_ts_closure_type(wasm_module_t wasm_module, wasm_defined_type_t type)
{
    bool is_struct_type;
    wasm_struct_type_t struct_type;
    uint32 field_count;
    bool mut;
    wasm_ref_type_t field_type;
    uint32 field_type_idx = 0;
    wasm_defined_type_t field_defined_type;

    is_struct_type = wasm_defined_type_is_struct_type(type);
    if (!is_struct_type) {
        return false;
    }

    struct_type = (wasm_struct_type_t)type;
    field_count = wasm_struct_type_get_field_count(struct_type);

    if (field_count != 2) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 0, &mut);
    field_type_idx = field_type.heap_type;
    field_defined_type = wasm_get_defined_type(wasm_module, field_type_idx);
    if (!wasm_defined_type_is_struct_type(field_defined_type)) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 1, &mut);
    field_type_idx = field_type.heap_type;
    field_defined_type = wasm_get_defined_type(wasm_module, field_type_idx);
    if (!wasm_defined_type_is_func_type(field_defined_type)) {
        return false;
    }

    return true;
}

/*
    utilities for array object

    * array struct (WasmGC struct)
    +----------+      +---------------------------+
    |  0:data  |----->|  content (WasmGC array)   |
    +----------+      +---------------------------+
    |  1:size  |      ^                           ^
    +----------+      |<-------  capacity  ------>|
*/
bool
is_ts_array_type(wasm_module_t wasm_module, wasm_defined_type_t type)
{
    bool is_struct_type;
    wasm_struct_type_t struct_type;
    uint32 field_count;
    bool mut;
    wasm_ref_type_t field_type;
    uint32 array_type_idx = 0;
    wasm_defined_type_t array_type;

    is_struct_type = wasm_defined_type_is_struct_type(type);
    if (!is_struct_type) {
        return false;
    }

    struct_type = (wasm_struct_type_t)type;
    field_count = wasm_struct_type_get_field_count(struct_type);

    if (field_count != 2) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 1, &mut);
    if (field_type.value_type != VALUE_TYPE_I32 || !mut) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 0, &mut);
    array_type_idx = field_type.heap_type;
    array_type = wasm_get_defined_type(wasm_module, array_type_idx);
    if (!mut || !wasm_defined_type_is_array_type(array_type)) {
        return false;
    }

    return true;
}

int
get_array_length(wasm_struct_obj_t obj)
{
    wasm_value_t wasm_array_len = { 0 };
    bh_assert(wasm_obj_is_struct_obj((wasm_obj_t)obj));

    wasm_struct_obj_get_field(obj, 1, false, &wasm_array_len);
    return wasm_array_len.i32;
}

wasm_array_obj_t
get_array_ref(wasm_struct_obj_t obj)
{
    wasm_value_t wasm_array = { 0 };
    bh_assert(wasm_obj_is_struct_obj((wasm_obj_t)obj));

    wasm_struct_obj_get_field(obj, 0, false, &wasm_array);
    return (wasm_array_obj_t)wasm_array.gc_obj;
}

int
get_array_capacity(wasm_struct_obj_t obj)
{
    wasm_array_obj_t array_ref = get_array_ref(obj);

    return wasm_array_obj_length(array_ref);
}

uint32_t
get_array_element_size(wasm_array_obj_t obj)
{
    wasm_array_type_t arr_type =
        (wasm_array_type_t)wasm_obj_get_defined_type((wasm_obj_t)obj);
    return wasm_value_type_size(arr_type->elem_type);
}

int32_t
get_array_type_by_element(wasm_module_t wasm_module,
                          wasm_ref_type_t *element_ref_type, bool is_mutable,
                          wasm_array_type_t *p_array_type)
{
    uint32 i, type_count;

    type_count = wasm_get_defined_type_count(wasm_module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(wasm_module, i);
        if (wasm_defined_type_is_array_type(type)) {
            bool mutable;
            wasm_ref_type_t arr_elem_ref_type = wasm_array_type_get_elem_type(
                (wasm_array_type_t)type, &mutable);
            if (wasm_ref_type_equal(&arr_elem_ref_type, element_ref_type,
                                    wasm_module)
                && (mutable == is_mutable)) {
                if (p_array_type) {
                    *p_array_type = (wasm_array_type_t)type;
                }
                return i;
            }
        }
    }

    if (p_array_type) {
        *p_array_type = NULL;
    }
    return -1;
}

int32_t
get_array_struct_type(wasm_module_t wasm_module, int32_t array_type_idx,
                      wasm_struct_type_t *p_struct_type)
{
    uint32 i, type_count;
    wasm_ref_type_t res_arr_ref_type;

    wasm_ref_type_set_type_idx(&res_arr_ref_type, true, array_type_idx);

    type_count = wasm_get_defined_type_count(wasm_module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(wasm_module, i);
        if (wasm_defined_type_is_struct_type(type)
            && (wasm_struct_type_get_field_count((wasm_struct_type_t)type)
                == 2)) {
            bool field1_mutable, field2_mutable;
            wasm_ref_type_t first_field_type = wasm_struct_type_get_field_type(
                (wasm_struct_type_t)type, 0, &field1_mutable);
            wasm_ref_type_t second_field_type = wasm_struct_type_get_field_type(
                (wasm_struct_type_t)type, 1, &field2_mutable);
            if (wasm_ref_type_equal(&first_field_type, &res_arr_ref_type,
                                    wasm_module)
                && second_field_type.value_type == VALUE_TYPE_I32) {
                if (p_struct_type) {
                    *p_struct_type = (wasm_struct_type_t)type;
                }
                return i;
            }
        }
    }

    if (p_struct_type) {
        *p_struct_type = NULL;
    }
    return -1;
}

wasm_struct_obj_t
create_wasm_array_with_string(wasm_exec_env_t exec_env, void **ptr, uint32_t arrlen)
{
    uint32 arr_type_idx, string_type_idx;
    wasm_value_t init = { .gc_obj = NULL }, tmp_val = { 0 },
                 val = { .gc_obj = NULL };
    wasm_array_type_t res_arr_type = NULL;
    wasm_struct_type_t arr_struct_type = NULL;
    wasm_struct_type_t string_struct_type = NULL;
    wasm_ref_type_t arr_ref_type;
    wasm_array_obj_t new_arr;
    wasm_local_obj_ref_t local_ref;
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);

    /* get array type_idx and the element is string */
    string_type_idx = get_string_struct_type(module, &string_struct_type);

    wasm_ref_type_set_type_idx(&arr_ref_type, true, string_type_idx);

    arr_type_idx =
        get_array_type_by_element(module, &arr_ref_type, true, &res_arr_type);
    bh_assert(wasm_defined_type_is_array_type((wasm_defined_type_t)res_arr_type));

    /* get result array struct type */
    get_array_struct_type(module, arr_type_idx, &arr_struct_type);
    bh_assert(
        wasm_defined_type_is_struct_type((wasm_defined_type_t)arr_struct_type));

    if(!ptr || !arrlen ) return NULL;

    /* create new array */
    new_arr = wasm_array_obj_new_with_type(exec_env, res_arr_type, arrlen, &init);
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)new_arr;

    if (!new_arr) {
        wasm_runtime_pop_local_object_ref(exec_env);
        wasm_runtime_set_exception((wasm_module_inst_t)module_inst,
                                   "alloc memory failed");
        return NULL;
    }

    /* create_wasm_string for every element */
    for (int i = 0; i < arrlen; i++) {
        const char *p = (const char *)((void **)ptr)[i];
        wasm_struct_obj_t string_struct = create_wasm_string(exec_env, p);
        val.gc_obj = (wasm_obj_t)string_struct;
        wasm_array_obj_set_elem(new_arr, i, &val);
    }

    wasm_struct_obj_t string_array_struct =
        wasm_struct_obj_new_with_type(exec_env, arr_struct_type);

    if (!string_array_struct) {
        wasm_runtime_set_exception((wasm_module_inst_t)module_inst,
                                   "alloc memory failed");
        return NULL;
    }

    tmp_val.gc_obj = (wasm_obj_t)new_arr;
    wasm_struct_obj_set_field(string_array_struct, 0, &tmp_val);
    tmp_val.u32 = arrlen;
    wasm_struct_obj_set_field(string_array_struct, 1, &tmp_val);

    wasm_runtime_pop_local_object_ref(exec_env);
    return string_array_struct;
}

/*
    utilities for string type

    * string struct (WasmGC struct)
    +----------+
    |  0:flag  |
    +----------+      +---------------------------+
    |  1:data  |----->| content (WasmGC array) |\0|
    +----------+      +---------------------------+
                      ^                        ^
                      |<------  length  ------>|
*/
static bool
is_i8_array(wasm_module_t wasm_module, bool is_mutable,
            wasm_ref_type_t ref_type)
{
    if (ref_type.heap_type >= 0) {
        uint32 type_idx = ref_type.heap_type;
        wasm_defined_type_t type = wasm_get_defined_type(wasm_module, type_idx);

        if (wasm_defined_type_is_array_type(type)) {
            bool mut;
            wasm_ref_type_t ref_element =
                wasm_array_type_get_elem_type((wasm_array_type_t)type, &mut);
            if (ref_element.value_type == VALUE_TYPE_I8 && mut == is_mutable) {
                return true;
            }
        }
    }

    return false;
}

int32_t
get_string_array_type(wasm_module_t wasm_module,
                      wasm_array_type_t *p_array_type_t)
{
    uint32 i, type_count;
    bool is_mutable = true;

    type_count = wasm_get_defined_type_count(wasm_module);
    for (i = 0; i < type_count; i++) {
        wasm_defined_type_t type = wasm_get_defined_type(wasm_module, i);

        if (wasm_defined_type_is_array_type(type)) {
            bool mutable;
            wasm_ref_type_t arr_elem_ref_type = wasm_array_type_get_elem_type(
                (wasm_array_type_t)type, &mutable);

            if (arr_elem_ref_type.value_type == VALUE_TYPE_I8
                && mutable == is_mutable) {
                if (p_array_type_t) {
                    *p_array_type_t = (wasm_array_type_t)type;
                }
                return i;
            }
        }
    }

    if (p_array_type_t) {
        *p_array_type_t = NULL;
    }

    return -1;
}

int32_t
get_string_struct_type(wasm_module_t wasm_module,
                       wasm_struct_type_t *p_struct_type)
{
    uint32 i, type_count;
    wasm_defined_type_t type;

    type_count = wasm_get_defined_type_count(wasm_module);
    for (i = 0; i < type_count; i++) {
        type = wasm_get_defined_type(wasm_module, i);
        if (!is_ts_string_type(wasm_module, type)) {
            continue;
        }
        if (p_struct_type) {
            *p_struct_type = (wasm_struct_type_t)type;
        }
        return i;
    }
    if (p_struct_type) {
        *p_struct_type = NULL;
    }
    return -1;
}

bool
is_ts_string_type(wasm_module_t wasm_module, wasm_defined_type_t type)
{
    bool is_struct_type;
    wasm_struct_type_t struct_type;
    uint32 field_count;
    bool mut;
    wasm_ref_type_t field_type;

    is_struct_type = wasm_defined_type_is_struct_type(type);
    if (!is_struct_type) {
        return false;
    }

    struct_type = (wasm_struct_type_t)type;
    field_count = wasm_struct_type_get_field_count(struct_type);

    if (field_count != 2) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 0, &mut);
    if (field_type.value_type != VALUE_TYPE_I32 || !mut) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 1, &mut);
    if (!mut || !is_i8_array(wasm_module, true, field_type)) {
        return false;
    }

    return true;
}

wasm_struct_obj_t create_wasm_string(wasm_exec_env_t exec_env, const char *value)
{
    wasm_struct_type_t string_struct_type = NULL;
    wasm_array_type_t string_array_type = NULL;
    wasm_local_obj_ref_t local_ref = { 0 };
    wasm_value_t val = { 0 };
    wasm_struct_obj_t new_string_struct = NULL;
    wasm_array_obj_t new_arr;
    int len = 0;
    char *p, *p_end;
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);

    /* get string len */
    len = strlen(value);

    /* get struct_string_type */
    get_string_struct_type(module, &string_struct_type);
    bh_assert(string_struct_type != NULL);
    bh_assert(wasm_defined_type_is_struct_type(
        (wasm_defined_type_t)string_struct_type));

    /* wrap with string struct */
    new_string_struct =
        wasm_struct_obj_new_with_type(exec_env, string_struct_type);
    if (!new_string_struct) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "alloc memory failed");
        return NULL;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)new_string_struct;

    val.i32 = 0;
    get_string_array_type(module, &string_array_type);
    new_arr = wasm_array_obj_new_with_type(exec_env, string_array_type, len,
                                           &val);
    if (!new_arr) {
        wasm_runtime_pop_local_object_ref(exec_env);
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        return NULL;
    }

    p = (char *)wasm_array_obj_first_elem_addr(new_arr);
    p_end = p + len;
    bh_assert(p);
    bh_assert(p_end);

    bh_memcpy_s(p, len, value, len);
    p += len;
    bh_assert(p == p_end);

    val.gc_obj = (wasm_obj_t)new_arr;
    wasm_struct_obj_set_field(new_string_struct, 1, &val);

    wasm_runtime_pop_local_object_ref(exec_env);

    (void)p_end;
    return new_string_struct;
}

bool
is_infc(wasm_obj_t obj) {
    wasm_struct_type_t struct_type;

    if (!obj || !wasm_obj_is_struct_obj(obj)) {
        return false;
    }
    struct_type = (wasm_struct_type_t)wasm_obj_get_defined_type(obj);

    uint32_t fields_count;
    bool mut;
    wasm_ref_type_t field_type;

    fields_count = wasm_struct_type_get_field_count(struct_type);
    if (fields_count != 4) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 0, &mut);
    if (field_type.value_type != VALUE_TYPE_I32 || mut) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 1, &mut);
    if (field_type.value_type != VALUE_TYPE_I32 || mut) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 2, &mut);
    if (field_type.value_type != VALUE_TYPE_I32 || mut) {
        return false;
    }
    field_type = wasm_struct_type_get_field_type(struct_type, 3, &mut);
    if (field_type.value_type != VALUE_TYPE_ANYREF || !mut) {
        return false;
    }

    return true;
}

void *
get_infc_obj(wasm_exec_env_t exec_env, wasm_obj_t obj) {
    wasm_value_t res = { 0 };
    wasm_struct_obj_t struct_obj;

    if (!is_infc(obj)) {
        return NULL;
    }
    struct_obj = (wasm_struct_obj_t)obj;
    wasm_struct_obj_get_field(struct_obj, 3, false, &res);

    return res.gc_obj;
}

void *
array_to_string(wasm_exec_env_t exec_env, void *ctx, void *obj,
                void *separator) {
    uint32 len, i, result_len, sep_len;
    uint32 *string_lengths;
    wasm_value_t value = { 0 }, field1 = { 0 };
    wasm_array_obj_t new_arr, arr_ref = get_array_ref(obj);
    wasm_struct_type_t string_struct_type = NULL;
    wasm_struct_obj_t new_string_struct = NULL;
    wasm_array_type_t string_array_type = NULL;
    wasm_local_obj_ref_t local_ref = { 0 };
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    char **string_addrs = NULL, *p, *p_end;
    char *sep = NULL;
    wasm_defined_type_t value_defined_type;

    len = get_array_length(obj);

    string_lengths = wasm_runtime_malloc(len * sizeof(uint32));
    if (!string_lengths) {
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        return NULL;
    }

    string_addrs = wasm_runtime_malloc(len * sizeof(char *));
    if (!string_addrs) {
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        goto fail;
    }

    /* get separator */
    if (separator) {
        dyn_value_t js_sep = (dyn_value_t) wasm_anyref_obj_get_value((wasm_anyref_obj_t)separator);
        if (!dyntype_is_undefined(ctx, js_sep)) {
            JSValue *js_value = (JSValue *)wasm_anyref_obj_get_value(separator);
            dyntype_to_cstring(dyntype_get_context(), js_value, &sep);
        }
    }

    for (i = 0; i < len; i++) {
        wasm_array_obj_get_elem(arr_ref, i, 0, &value);
        if (!value.gc_obj) {
            string_lengths[i] = 0;
            string_addrs[i] = "";
            continue;
        }
        wasm_struct_obj_get_field((wasm_struct_obj_t)value.gc_obj, 1, false,
                                  &field1);
        value_defined_type = wasm_obj_get_defined_type((wasm_obj_t)value.gc_obj);
        if (is_ts_string_type(module, value_defined_type)) {
            wasm_array_obj_t str_array = (wasm_array_obj_t)field1.gc_obj;
            string_lengths[i] = wasm_array_obj_length(str_array);
            string_addrs[i] = wasm_array_obj_first_elem_addr(str_array);
        }
        else {
            wasm_runtime_set_exception(
                wasm_runtime_get_module_inst(exec_env),
                "array join for non-string type not implemented");
            goto fail;
        }
    }

    result_len = 0;
    /* If there is no separator, it will be separated by ',' by default */
    sep_len = sep ? strlen(sep) : strlen(",");
    for (i = 0; i < len; i++) {
        result_len += string_lengths[i] + sep_len;
    }
    if (len >= 1) {
        /* Remove separator after last character */
        result_len -= sep_len;
    }
    /* Create new array for holding string contents */
    value.i32 = 0;
    get_string_array_type(module, &string_array_type);
    new_arr = wasm_array_obj_new_with_type(exec_env, string_array_type,
                                           result_len, &value);
    if (!new_arr) {
        wasm_runtime_set_exception(module_inst, "alloc memory failed");
        goto fail;
    }

    /* Push object to local ref to avoid being freed at next allocation */
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (wasm_obj_t)new_arr;

    p = (char *)wasm_array_obj_first_elem_addr(new_arr);
    p_end = p + result_len;
    bh_assert(p);
    bh_assert(p_end);

    for (i = 0; i < len; i++) {
        uint32 cur_string_len = string_lengths[i];
        bh_memcpy_s(p, p_end - p, string_addrs[i], cur_string_len);
        p += cur_string_len;
        if (i < len - 1) {
            bh_memcpy_s(p, p_end - p, sep ? sep : ",", sep_len);
            p += sep_len;
        }
    }
    bh_assert(p == p_end);

    /* get struct_string_type */
    get_string_struct_type(module, &string_struct_type);
    bh_assert(string_struct_type != NULL);
    bh_assert(wasm_defined_type_is_struct_type(
        (wasm_defined_type_t)string_struct_type));

    /* wrap with string struct */
    new_string_struct =
        wasm_struct_obj_new_with_type(exec_env, string_struct_type);
    if (!new_string_struct) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "alloc memory failed");
        goto fail;
    }

    value.gc_obj = (wasm_obj_t)new_arr;
    wasm_struct_obj_set_field(new_string_struct, 1, &value);

fail:
    if (string_lengths) {
        wasm_runtime_free(string_lengths);
    }

    if (string_addrs) {
        wasm_runtime_free(string_addrs);
    }

    if (local_ref.val) {
        wasm_runtime_pop_local_object_ref(exec_env);
    }

    if (sep) {
        dyntype_free_cstring(dyntype_get_context(), sep);
    }

    return new_string_struct;
}

void
get_dyn_obj_info(void *dyn_ctx, void *dyn_obj, uint32 *obj_info)
{
    bool is_ext_ref;
    void *ext_ref;
    int tag = 0;

    is_ext_ref = dyntype_is_extref((dyn_value_t)dyn_ctx, (dyn_value_t)dyn_obj);
    /* if !is_ext_ref, the obj is a pure dynamic obj */
    if (!is_ext_ref) {
        obj_info[0] = DynObject;
    }
    else {
        /* else, get ext_ref from table */
        tag = dyntype_to_extref((dyn_value_t)dyn_ctx, (dyn_value_t)dyn_obj,
                                &ext_ref);
        if (tag == ExtObj) {
            obj_info[0] = DynExtRefObj;
        }
        else if (tag == ExtInfc) {
            obj_info[0] = DynExtRefInfc;
        }
        else if (tag == ExtArray) {
            obj_info[0] = DynExtRefArray;
        }
        else if (tag == ExtFunc) {
            obj_info[0] = DynExtRefFunc;
        }
        /* store table index */
        obj_info[1] = (uint32_t)(uintptr_t)ext_ref;
    }
}

bool
get_static_array_info(wasm_exec_env_t exec_env, void *dyn_ctx, void *dyn_obj,
                      int index, uint64 *arr_info)
{
    void *static_arr_struct = NULL;
    wasm_defined_type_t static_arr_arr_type = { 0 };
    bool mutable = false;
    wasm_array_obj_t arr_ref = { 0 };
    uint64 arr_len = 0;
    wasm_ref_type_t arr_elem_ref_type = { 0 };
    wasm_value_t elem_res = { 0 };
    uint32 obj_info[2] = { 0 };

    get_dyn_obj_info(dyn_ctx, dyn_obj, obj_info);
    if (obj_info[0] == DynExtRefArray) {
        static_arr_struct = (wasm_struct_obj_t)wamr_utils_get_table_element(
            exec_env, obj_info[1]);
        arr_ref = get_array_ref(static_arr_struct);
        arr_len = get_array_length(static_arr_struct);
        static_arr_arr_type = wasm_obj_get_defined_type((wasm_obj_t)arr_ref);
        arr_elem_ref_type = wasm_array_type_get_elem_type(
            (wasm_array_type_t)static_arr_arr_type, &mutable);
        arr_info[0] = (uint64)(intptr_t)&arr_ref;
        arr_info[1] = (uint64)(intptr_t)&arr_elem_ref_type;
        if (index < 0) {
            arr_info[2] = arr_len;
        }
        else {
            bh_assert(arr_len > index);
            wasm_array_obj_get_elem(arr_ref, index, false, &elem_res);
            arr_info[2] = (uint64)(intptr_t)&elem_res;
        }
        return true;
    }
    else {
        return false;
    }
}

int
get_prop_index_of_struct(wasm_exec_env_t exec_env, const char *prop,
                         wasm_obj_t *wasm_obj, wasm_ref_type_t *field_type)
{
    wasm_module_inst_t module_inst;
    bool is_mut;
    wasm_function_inst_t func;
    wasm_struct_obj_t wasm_struct_obj;
    WASMValue vtable_value = { 0 };
    WASMValue meta = { 0 };
    uint32 argc = 3, argv[3] = { 0 }, offset;
    wasm_struct_type_t struct_type;

    module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_struct_obj = (wasm_struct_obj_t)(*wasm_obj);
    wasm_struct_obj_get_field(wasm_struct_obj, 0, false, &vtable_value);
    wasm_struct_obj_get_field((wasm_struct_obj_t)vtable_value.gc_obj, 0, false, &meta);
    func = wasm_runtime_lookup_function(module_inst, "find_index", NULL);
    bh_assert(func);

    argv[0] = meta.i32;
    offset = wasm_runtime_addr_native_to_app(module_inst, (void *)prop);
    argv[1] = offset;
    argv[2] = 0;

    wasm_runtime_call_wasm(exec_env, func, argc, argv);
    if (argv[0] == -1) {
        return -1;
    }
    struct_type = (wasm_struct_type_t)wasm_obj_get_defined_type(*wasm_obj);
    *field_type = wasm_struct_type_get_field_type(struct_type, argv[0], &is_mut);

    return argv[0];
}

/**********Utils for search field value of object through meta information*************/
static inline int32
get_meta_fields_count(void *meta) {
    return *(int32 *)(meta + OFFSET_OF_COUNT);
}

static inline void *
get_meta_field_by_index(void *meta, int32 index) {
    return (meta + OFFSET_OF_META_FIELDS + index * SIZEOF_META_FIELD);
}

static inline enum field_flag
get_meta_field_flag(void *meta_field) {
    int flag = *((int32 *)(meta_field + OFFSET_OF_FIELD_FLAG_AND_INDEX))
                & META_FLAG_MASK;

    return (enum field_flag)flag;
}

static inline int32
get_meta_field_index(void *meta_field) {
    return (*((int32 *)(meta_field + OFFSET_OF_FIELD_FLAG_AND_INDEX))
            & META_INDEX_MASK) >> 4;
}

static inline int32
get_meta_field_type(void *meta_field) {
    return *((int32 *)(meta_field + OFFSET_OF_FIELD_TYPE));
}

static inline int32
get_meta_field_name(void *meta_field) {
    return *((int32 *)meta_field);
}

static int32
get_object_field_index_by_mata(wasm_exec_env_t exec_env,
                               void *meta,
                               const char *field_name,
                               enum field_flag flag,
                               ts_value_type_t *field_type) {
    int32 count;
    void *meta_field;
    enum field_flag meta_field_flag;
    int32 meta_field_name_offset;
    const char *meta_field_name;
    int32 meta_field_index;
    int32 field_type_id;

    count = get_meta_fields_count(meta);
    meta_field_index = -1;

    for (int index = 0; index < count; index++) {
        meta_field = get_meta_field_by_index(meta, index);
        meta_field_flag = get_meta_field_flag(meta_field);
        meta_field_name_offset = get_meta_field_name(meta_field);
        meta_field_name = wasm_runtime_addr_app_to_native(wasm_runtime_get_module_inst(exec_env),
                                                          meta_field_name_offset);

        if (meta_field_flag == flag
            && strcmp(field_name, meta_field_name) == 0) {
            meta_field_index = get_meta_field_index(meta_field);
            if (field_type) {
                field_type_id = get_meta_field_type(meta_field);
                if (field_type_id >= CUSTOM_TYPE_BEGIN) {
                    *field_type = TS_OBJECT;
                } else {
                    *field_type = (ts_value_type_t)field_type_id;
                }
            }
            break;
        }
    }

    return meta_field_index;
}

int
get_object_field(wasm_exec_env_t exec_env,
                 wasm_obj_t obj,
                 const char *field_name,
                 enum field_flag flag,
                 ts_value_t *field_value) {
    wasm_obj_t wasm_obj;
    wasm_struct_obj_t struct_obj;
    WASMValue vtable_value = { 0 };
    wasm_struct_obj_t vtable_struct;
    wasm_value_t value = { 0 };
    wasm_value_t meta = { 0 };
    void *meta_addr;
    int32 field_index;

    wasm_obj = obj;
    if (is_infc(obj)) {
        wasm_obj = (wasm_obj_t)get_infc_obj(exec_env, obj);
    }

    /* get meta addr */
    struct_obj = (wasm_struct_obj_t)wasm_obj;
    wasm_struct_obj_get_field(struct_obj, 0, false, &vtable_value);
    vtable_struct = (wasm_struct_obj_t)vtable_value.gc_obj;
    wasm_struct_obj_get_field(vtable_struct, 0, false, &meta);
    meta_addr = wasm_runtime_addr_app_to_native(wasm_runtime_get_module_inst(exec_env), meta.i32);

    /* get field index */
    field_index = get_object_field_index_by_mata(exec_env, meta_addr, field_name, flag, &field_value->type);
    if (field_index == -1) {
        return -1;
    }

    if (flag == FIELD) {
        wasm_struct_obj_get_field(struct_obj, field_index, false, &value);
    } else {
        wasm_struct_obj_get_field(vtable_struct, field_index, false, &value);
    }

    if (field_value->type == TS_BOOLEAN
        || field_value->type == TS_INT) {
        field_value->of.i32 = value.i32;
    } else if (field_value->type == TS_NUMBER) {
        field_value->of.f64 = value.f64;
    } else {
        field_value->of.ref = value.gc_obj;
    }

    return 0;
}
