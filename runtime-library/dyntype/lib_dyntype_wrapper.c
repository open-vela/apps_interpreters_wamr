/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include "gc_export.h"
#include "bh_platform.h"
#include "type_utils.h"
#include "wamr_utils.h"
#include "wasm.h"

/* Convert host pointer to anyref */
#define RETURN_BOX_ANYREF(ptr)                     \
    do {                                           \
        return wasm_anyref_obj_new(exec_env, ptr); \
    } while (0)

#define BOX_ANYREF(ptr) wasm_anyref_obj_new(exec_env, ptr)

/* Convert anyref to host pointer */
#define UNBOX_ANYREF(anyref) \
    (dyn_value_t) wasm_anyref_obj_get_value((wasm_anyref_obj_t)anyref)

/****************** Context access *****************/
void *
dyntype_get_context_wrapper(wasm_exec_env_t exec_env)
{
    dyn_ctx_t ctx = dyntype_get_context();
    RETURN_BOX_ANYREF(ctx);
}

/******************* Field access *******************/
dyn_value_t
dyntype_new_number_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           double value)
{
    RETURN_BOX_ANYREF(dyntype_new_number(UNBOX_ANYREF(ctx), value));
}

dyn_value_t
dyntype_new_boolean_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                            bool value)
{
    RETURN_BOX_ANYREF(dyntype_new_boolean(UNBOX_ANYREF(ctx), value));
}

dyn_value_t
dyntype_new_string_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           wasm_struct_obj_t str_obj)
{
    WASMValue arr_obj = { 0 };
    uint32_t arr_len = 0;
    const char *str = "";
    wasm_struct_obj_get_field(str_obj, 1, false, &arr_obj);
    arr_len = wasm_array_obj_length((wasm_array_obj_t)arr_obj.gc_obj);

    if (arr_len != 0) {
        str = (char *)wasm_array_obj_first_elem_addr(
            (wasm_array_obj_t)arr_obj.gc_obj);
    }

    RETURN_BOX_ANYREF(dyntype_new_string_with_length(UNBOX_ANYREF(ctx), str, arr_len));
}

dyn_value_t
dyntype_new_undefined_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx)
{
    RETURN_BOX_ANYREF(dyntype_new_undefined(UNBOX_ANYREF(ctx)));
}

dyn_value_t
dyntype_new_null_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx)
{
    RETURN_BOX_ANYREF(dyntype_new_null(UNBOX_ANYREF(ctx)));
}

dyn_value_t
dyntype_new_object_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx)
{
    RETURN_BOX_ANYREF(dyntype_new_object(UNBOX_ANYREF(ctx)));
}

dyn_value_t
dyntype_new_array_with_length_wrapper(wasm_exec_env_t exec_env,
                                      wasm_anyref_obj_t ctx, int len)
{
    RETURN_BOX_ANYREF(dyntype_new_array_with_length(UNBOX_ANYREF(ctx), len));
}

dyn_value_t
dyntype_new_array_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx)
{
    RETURN_BOX_ANYREF(dyntype_new_array(UNBOX_ANYREF(ctx)));
}

void
dyntype_add_elem_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t obj, wasm_anyref_obj_t elem)
{
}

void
dyntype_set_elem_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t obj, int index,
                         wasm_anyref_obj_t elem)
{
    dyntype_set_elem(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), index,
                     UNBOX_ANYREF(elem));
}

dyn_value_t
dyntype_get_elem_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t obj, int index)
{
    RETURN_BOX_ANYREF(dyntype_get_elem(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), index));
}

dyn_value_t
dyntype_new_extref_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           void *ptr, external_ref_tag tag)
{
    RETURN_BOX_ANYREF(dyntype_new_extref(UNBOX_ANYREF(ctx), ptr, tag, (void *)exec_env));
}

int
dyntype_set_property_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                             wasm_anyref_obj_t obj, const char *prop,
                             wasm_anyref_obj_t value)
{
    return dyntype_set_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop,
                                UNBOX_ANYREF(value));
}

int
dyntype_define_property_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                                wasm_anyref_obj_t obj, const char *prop,
                                wasm_anyref_obj_t desc)
{
    return dyntype_define_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop,
                                   UNBOX_ANYREF(desc));
}

dyn_value_t
dyntype_get_property_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                             wasm_anyref_obj_t obj, const char *prop)
{
    RETURN_BOX_ANYREF(
        dyntype_get_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop));
}

int
dyntype_has_property_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                             wasm_anyref_obj_t obj, const char *prop)
{
    return dyntype_has_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop);
}

int
dyntype_delete_property_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                                wasm_anyref_obj_t obj, const char *prop)
{
    return dyntype_delete_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop);
}

/******************* Runtime type checking *******************/
int
dyntype_is_undefined_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                             wasm_anyref_obj_t obj)
{
    return dyntype_is_undefined(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_is_null_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t obj)
{
    return dyntype_is_null(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_is_bool_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t obj)
{
    return dyntype_is_bool(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_to_bool_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t obj)
{
    bool value = 0, ret;

    ret = dyntype_to_bool(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to bool");
    }

    return value;
}

int
dyntype_is_number_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    return dyntype_is_number(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

double
dyntype_to_number_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    double value = 0;
    bool ret;

    ret = dyntype_to_number(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to number");
    }

    return value;
}

int
dyntype_is_string_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    return dyntype_is_string(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

char *
dyntype_to_cstring_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           wasm_anyref_obj_t obj)
{
    wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                               "libdyntype: string not supported");

    return NULL;
}

void *
dyntype_to_string_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    char *value = NULL;
    int ret;
    wasm_struct_obj_t new_string_struct = NULL;

    ret = dyntype_to_cstring(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), &value);
    if (ret != DYNTYPE_SUCCESS) {
        if (value) {
            dyntype_free_cstring(UNBOX_ANYREF(ctx), value);
        }
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to cstring");
        return NULL;
    }

    new_string_struct = create_wasm_string(exec_env, value);

    return (void *)new_string_struct;
}

void
dyntype_free_cstring_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                             char *str)
{
    return dyntype_free_cstring(UNBOX_ANYREF(ctx), str);
}

int
dyntype_is_object_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    return dyntype_is_object(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_is_array_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t obj)
{
    return dyntype_is_array(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_is_extref_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    return dyntype_is_extref(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

void *
dyntype_to_extref_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          wasm_anyref_obj_t obj)
{
    void *value = NULL;
    int ret;

    ret = dyntype_to_extref(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), &value);
    if (ret < ExtObj || ret > ExtArray) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "libdyntype: failed to convert to extref");
    }

    return value;
}

int
dyntype_is_falsy_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t value)
{
    return dyntype_is_falsy(UNBOX_ANYREF(ctx), UNBOX_ANYREF(value));
}

void *
dyntype_toString_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         wasm_anyref_obj_t value)
{
    char *str;
    dyn_type_t type;
    wasm_struct_obj_t res;
    void *table_elem;
    int32_t table_index;
    dyn_value_t dyn_ctx, dyn_value;
    char *tmp_value = NULL;

    dyn_ctx = UNBOX_ANYREF(ctx);
    dyn_value = UNBOX_ANYREF(value);

    if (dyntype_is_extref(dyn_ctx, dyn_value)) {
        type = dyntype_typeof(dyn_ctx, dyn_value);
        if (type != DynExtRefArray) {
            tmp_value = "[object Object]";
            if (type == DynExtRefFunc) {
                tmp_value = "[wasm Function]";
            }
            res = create_wasm_string(exec_env, tmp_value);
        } else {
            dyntype_to_extref(dyn_ctx, dyn_value, &table_elem);
            table_index = (int32_t)(intptr_t)table_elem;
            table_elem = wamr_utils_get_table_element(exec_env, table_index);
            res = array_to_string(exec_env, dyn_ctx, table_elem, NULL);
        }
    } else {
        dyntype_to_cstring(dyn_ctx, dyn_value, &str);
        if (str == NULL) {
            return NULL;
        }
        res = create_wasm_string(exec_env, str);
        dyntype_free_cstring(dyn_ctx, str);
    }

    return res;
}

/******************* Type equivalence *******************/
/* for typeof keyword*/
void *
dyntype_typeof_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                       wasm_anyref_obj_t obj)
{
    dyn_type_t dyn_type;
    char* value;
    wasm_struct_obj_t res;

    dyn_type = dyntype_typeof(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
    switch (dyn_type) {
        case DynUndefined:
            value = "undefined";
            break;
        case DynBoolean:
            value = "boolean";
            break;
        case DynNumber:
            value = "number";
            break;
        case DynString:
            value = "string";
            break;
        case DynFunction:
        case DynExtRefFunc:
            value = "function";
            break;
        case DynNull:
        case DynObject:
        case DynExtRefObj:
        case DynExtRefInfc:
        case DynExtRefArray:
            value = "object";
            break;
        default:
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                    "libdyntype: typeof getting unknown type");
            value = "unknown";
    }
    res = create_wasm_string(exec_env, value);

    return (void*)res;
}

/* for internal use, no need to create a wasm string*/
dyn_type_t
dyntype_typeof1_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t obj)
{
    return dyntype_typeof(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_type_eq_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t lhs, wasm_anyref_obj_t rhs)
{
    return dyntype_type_eq(UNBOX_ANYREF(ctx), UNBOX_ANYREF(lhs),
                           UNBOX_ANYREF(rhs));
}

int
dyntype_cmp_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                    wasm_anyref_obj_t lhs, wasm_anyref_obj_t rhs,
                    cmp_operator operator_kind)
{
    int res = 0;
    dyn_type_t type_l, type_r;
    bool l_is_null = false, r_is_null = false;
    void *lhs_ref, *rhs_ref;
    int32_t lhs_idx, rhs_idx;

    type_l = dyntype_typeof(UNBOX_ANYREF(ctx), UNBOX_ANYREF(lhs));
    type_r = dyntype_typeof(UNBOX_ANYREF(ctx), UNBOX_ANYREF(rhs));

    if (type_l == type_r) {
        res = dyntype_cmp(UNBOX_ANYREF(ctx), UNBOX_ANYREF(lhs), UNBOX_ANYREF(rhs),
                        operator_kind);
    }
    if (res) {
        return res;
    }
    if (dyntype_is_null(UNBOX_ANYREF(ctx), UNBOX_ANYREF(lhs))) {
        l_is_null = true;
    }
    if (dyntype_is_null(UNBOX_ANYREF(ctx), UNBOX_ANYREF(rhs))) {
        r_is_null = true;
    }
    // iff undefined
    if (type_l != type_r && (type_l == DynUndefined || type_r == DynUndefined)) {
        if (operator_kind == ExclamationEqualsToken
            || operator_kind == ExclamationEqualsEqualsToken) {
            res = !res;
        }
        return res;
    }
    // iff null
    if ((!l_is_null && (type_l < DynExtRefObj || type_l > DynExtRefArray))
        || (!r_is_null && (type_r < DynExtRefObj || type_r > DynExtRefArray))) {
        if (type_l != type_r && (operator_kind == ExclamationEqualsToken
            || operator_kind == ExclamationEqualsEqualsToken)) {
            res = !res;
        }
        return res;
    }

    if (!l_is_null) {
        dyntype_to_extref(UNBOX_ANYREF(ctx), UNBOX_ANYREF(lhs), &lhs_ref);
        lhs_idx = (int32_t)(intptr_t)lhs_ref;
        lhs_ref = wamr_utils_get_table_element(exec_env, lhs_idx);
        if (is_infc(lhs_ref)) {
            lhs_ref = get_infc_obj(exec_env, lhs_ref);
        }
    } else {
        lhs_ref = NULL;
    }
    if (!r_is_null) {
        dyntype_to_extref(UNBOX_ANYREF(ctx), UNBOX_ANYREF(rhs), &rhs_ref);
        rhs_idx = (int32_t)(intptr_t)rhs_ref;
        rhs_ref = wamr_utils_get_table_element(exec_env, rhs_idx);
        if (is_infc(rhs_ref)) {
            rhs_ref = get_infc_obj(exec_env, rhs_ref);
        }
    } else {
        rhs_ref = NULL;
    }
    res = lhs_ref == rhs_ref;

    if (operator_kind == ExclamationEqualsToken || operator_kind == ExclamationEqualsEqualsToken) {
        res = !res;
    }

    return res;
}

/******************* Subtyping *******************/
dyn_value_t
dyntype_new_object_with_proto_wrapper(wasm_exec_env_t exec_env,
                                      wasm_anyref_obj_t ctx,
                                      const wasm_anyref_obj_t proto_obj)
{
    RETURN_BOX_ANYREF(dyntype_new_object_with_proto(UNBOX_ANYREF(ctx),
                                             UNBOX_ANYREF(proto_obj)));
}

int
dyntype_set_prototype_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                              wasm_anyref_obj_t obj,
                              wasm_anyref_obj_t proto_obj)
{
    return dyntype_set_prototype(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj),
                                 UNBOX_ANYREF(proto_obj));
}

const dyn_value_t
dyntype_get_prototype_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                              wasm_anyref_obj_t obj)
{
    RETURN_BOX_ANYREF(dyntype_get_prototype(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj)));
}

dyn_value_t
dyntype_get_own_property_wrapper(wasm_exec_env_t exec_env,
                                 wasm_anyref_obj_t ctx, wasm_anyref_obj_t obj,
                                 const char *prop)
{
    RETURN_BOX_ANYREF(
        dyntype_get_own_property(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), prop));
}

int
dyntype_instanceof_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           const wasm_anyref_obj_t src_obj,
                           const wasm_anyref_obj_t dst_obj)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    dyn_type_t obj_type;
    dyn_ctx_t dyn_ctx;
    dyn_value_t dyn_src;
    void *table_elem;
    int32_t table_idx;
    wasm_obj_t obj;
    wasm_obj_t inst_obj;
    wasm_defined_type_t inst_type;

    dyn_ctx = UNBOX_ANYREF(ctx);
    dyn_src = UNBOX_ANYREF(src_obj);
    obj_type = dyntype_typeof(dyn_ctx, dyn_src);

    // if src is not an extref object, return false
    if (obj_type < DynExtRefObj) {
        return 0;
    }
    dyntype_to_extref(dyn_ctx, dyn_src, &table_elem);
    table_idx = (int32_t)(intptr_t)table_elem;
    table_elem = wamr_utils_get_table_element(exec_env, table_idx);
    if (is_infc(table_elem)) {
        table_elem = get_infc_obj(exec_env, table_elem);
    }

    obj = (wasm_obj_t)table_elem;
    inst_obj = (wasm_obj_t)dst_obj;
    if (!wasm_obj_is_struct_obj(inst_obj)) {
        return 0;
    }
    inst_type = wasm_obj_get_defined_type(inst_obj);

    return wasm_obj_is_instance_of_defined_type(obj, inst_type, module);
}

/******************* Dumping *******************/
void
dyntype_dump_value_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           wasm_anyref_obj_t obj)
{
    return dyntype_dump_value(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

int
dyntype_dump_value_buffer_wrapper(wasm_exec_env_t exec_env,
                                  wasm_anyref_obj_t ctx, wasm_anyref_obj_t obj,
                                  void *buffer, int len)
{
    return dyntype_dump_value_buffer(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj),
                                     buffer, len);
}

/******************* Garbage collection *******************/

void
dyntype_hold_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                     wasm_anyref_obj_t obj)
{
    return dyntype_hold(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

void
dyntype_release_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                        wasm_anyref_obj_t obj)
{
    return dyntype_release(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj));
}

void
dyntype_collect_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx)
{
    return dyntype_collect(UNBOX_ANYREF(ctx));
}

wasm_anyref_obj_t
dyntype_invoke_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                       const char *name, wasm_anyref_obj_t this_obj,
                       wasm_struct_obj_t args_array)
{
    dyn_value_t this_val = UNBOX_ANYREF(this_obj);
    dyn_value_t *argv = NULL;
    dyn_value_t ret = NULL;
    wasm_array_obj_t arr_ref = { 0 };
    wasm_value_t tmp;
    int argc = 0;
    int i = 0;

    arr_ref = get_array_ref(args_array);
    argc = get_array_length(args_array);
    if (argc) {
        argv = wasm_runtime_malloc(sizeof(dyn_value_t) * argc);
        if (!argv) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
    }

    for (i = 0; i < argc; i++) {
        wasm_array_obj_get_elem(arr_ref, i, false, &tmp);
        argv[i] = (dyn_value_t)UNBOX_ANYREF(tmp.gc_obj);
    }

    ret = dyntype_invoke(UNBOX_ANYREF(ctx), name, this_val, argc, argv);

    if (!ret) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "dyntype invoke failed");
        return NULL;
    }
    if (argv) {
        wasm_runtime_free(argv);
    }

    RETURN_BOX_ANYREF(ret);
}

wasm_anyref_obj_t
dyntype_get_global_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           const char *name)
{
    RETURN_BOX_ANYREF(dyntype_get_global(UNBOX_ANYREF(ctx), name));
}

wasm_anyref_obj_t
dyntype_new_object_with_class_wrapper(wasm_exec_env_t exec_env,
                                      wasm_anyref_obj_t ctx, const char *name,
                                      wasm_struct_obj_t args_array)
{
    wasm_array_obj_t arr_ref = { 0 };
    dyn_value_t ret = NULL;
    dyn_value_t *argv = NULL;
    wasm_value_t tmp;
    int argc = 0;
    int i = 0;

    arr_ref = get_array_ref(args_array);
    argc = get_array_length(args_array);
    if (argc) {
        argv = wasm_runtime_malloc(sizeof(dyn_value_t) * argc);
        if (!argv) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
    }

    for (i = 0; i < argc; i++) {
        wasm_array_obj_get_elem(arr_ref, i, false, &tmp);
        argv[i] = (dyn_value_t)UNBOX_ANYREF(tmp.gc_obj);
    }

    ret = dyntype_new_object_with_class(UNBOX_ANYREF(ctx), name, argc, argv);

    if (!ret) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "dyntype_new_object_with_class failed");
        return NULL;
    }
    if (argv) {
        wasm_runtime_free(argv);
    }

    RETURN_BOX_ANYREF(ret);
}

/******************* Function callback *******************/
void *
dyntype_call_func_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                          void *obj, int argc, wasm_anyref_obj_t *args)
{
    dyn_value_t *argv = NULL;
    dyn_value_t ret = NULL;
    int i = 0;

    if (argc > 0) {
        argv = (dyn_value_t *)wasm_runtime_malloc(sizeof(dyn_value_t) * argc);
        if (!argv) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
    }

    for (i = 0; i < argc; i++) {
        argv[i] = UNBOX_ANYREF(args[i]);
    }
    ret = dyntype_call_func(UNBOX_ANYREF(ctx), UNBOX_ANYREF(obj), argc, argv);

    if (!ret) {
        wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                   "dyntype_call_func failed");
        return NULL;
    }
    if (argv) {
        wasm_runtime_free(argv);
    }

    RETURN_BOX_ANYREF(ret);
}

static void *
box_value_to_dyntype(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                     void *value, wasm_ref_type_t type, uint32 slot_count)
{
    void *ret = NULL;
    wasm_defined_type_t ret_defined_type = { 0 };
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    int tag = 0;
    uint32 table_idx = 0;

    if (type.value_type == VALUE_TYPE_I32) {
        uint32 ori_value = 0;
        bh_assert(slot_count == 1);
        bh_memcpy_s(&ori_value, sizeof(uint32), value, sizeof(uint32));
        ret = dyntype_new_boolean_wrapper(exec_env, ctx, (bool)ori_value);
    }
    else if (type.value_type == VALUE_TYPE_F64) {
        double ori_value = 0;
        bh_assert(slot_count == 2);
        bh_memcpy_s(&ori_value, sizeof(double), value, sizeof(double));
        ret = dyntype_new_number_wrapper(exec_env, ctx, ori_value);
    }
    else if (type.value_type == REF_TYPE_ANYREF) {
        void *ori_value = NULL;
        bh_memcpy_s(&ori_value, sizeof(void *), value, sizeof(void *));
        ret = ori_value;
    }
    else {
        void *ori_value = NULL;
        bh_memcpy_s(&ori_value, sizeof(void *), value, sizeof(void *));

        ret_defined_type = wasm_get_defined_type(module, type.heap_type);
        if (wasm_defined_type_is_struct_type(ret_defined_type)) {
            if (is_ts_string_type(module, ret_defined_type)) {
                ret = dyntype_new_string_wrapper(exec_env, ctx,
                                                 (wasm_struct_obj_t)ori_value);
            }
            else {
                wasm_runtime_set_exception(
                    wasm_runtime_get_module_inst(exec_env),
                    "Not support box to extref in native yet");
                // TODO: put value into table
                // TODO: get table idx
                if (is_infc((wasm_obj_t)ori_value)) {
                    tag = ExtInfc;
                }
                else if (is_ts_array_type(module, ret_defined_type)) {
                    tag = ExtArray;
                }
                else if (is_ts_closure_type(module, ret_defined_type)) {
                    tag = ExtFunc;
                }
                else {
                    tag = ExtObj;
                }
                ret = dyntype_new_extref_wrapper(
                    exec_env, ctx, (void *)(uintptr_t)table_idx, tag);
            }
        }
    }

    return ret;
}

static void
unbox_value_from_dyntype(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                         void *obj, wasm_ref_type_t type, void *unboxed_value,
                         uint32 slot_count)
{
    uint32 table_idx = 0;
    wasm_defined_type_t ret_defined_type = { 0 };
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);

    if (type.value_type == VALUE_TYPE_I32) {
        int ret_value = dyntype_to_bool_wrapper(exec_env, ctx, obj);
        bh_assert(slot_count == 1);
        bh_memcpy_s(unboxed_value, sizeof(uint32), &ret_value, sizeof(uint32));
    }
    else if (type.value_type == VALUE_TYPE_F64) {
        double ret_value = dyntype_to_number_wrapper(exec_env, ctx, obj);
        bh_assert(slot_count == 2);
        bh_memcpy_s(unboxed_value, sizeof(double), &ret_value, sizeof(double));
    }
    else if (type.value_type == REF_TYPE_ANYREF) {
        bh_memcpy_s(unboxed_value, sizeof(void *), &obj, sizeof(void *));
    }
    else {
        ret_defined_type = wasm_get_defined_type(module, type.heap_type);
        if (wasm_defined_type_is_struct_type(ret_defined_type)) {
            if (is_ts_string_type(module, ret_defined_type)) {
                void *ret_value = dyntype_to_string_wrapper(exec_env, ctx, obj);
                bh_memcpy_s(unboxed_value, sizeof(void *), &ret_value,
                            sizeof(void *));
            }
            else {
                void *ret_value;
                table_idx = (uint32)(uintptr_t)dyntype_to_extref_wrapper(
                    exec_env, ctx, obj);
                ret_value = wamr_utils_get_table_element(exec_env, table_idx);
                bh_memcpy_s(unboxed_value, sizeof(void *), &ret_value,
                            sizeof(void *));
            }
        }
    }
}

static uint32
get_slot_count(wasm_ref_type_t type)
{
    if (type.value_type == VALUE_TYPE_I32) {
        return sizeof(uint32) / sizeof(uint32);
    }
    else if (type.value_type == VALUE_TYPE_F64) {
        return sizeof(double) / sizeof(uint32);
    }
    else {
        return sizeof(void *) / sizeof(uint32);
    }
}

void *
call_wasm_func_with_boxing(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                           wasm_anyref_obj_t func_any_obj, uint32 argc,
                           wasm_anyref_obj_t *func_args,
                           bool is_wasm_source_func)
{
    int i;
    void *ret = NULL;
    wasm_value_t context = { 0 };
    wasm_value_t func_ref = { 0 };
    wasm_func_obj_t func_obj = { 0 };
    wasm_func_type_t func_type = { 0 };
    wasm_ref_type_t result_type = { 0 };
    wasm_ref_type_t tmp_param_type = { 0 };
    wasm_struct_obj_t closure_obj = { 0 };
    wasm_value_t tmp_result;
    wasm_value_t tmp_param;
    uint32 slot_count = 0;
    uint32 occupied_slots = 0;
    uint32 *argv = NULL;
    uint32 bsize = 0;
    uint32 result_count = 0;
    uint32 param_count = 0;
    bool is_success;

    if (wasm_obj_is_struct_obj((wasm_obj_t)func_any_obj)) {
        closure_obj = (wasm_struct_obj_t)func_any_obj;
        wasm_struct_obj_get_field(closure_obj, 0, false, &context);
        wasm_struct_obj_get_field(closure_obj, 1, false, &func_ref);
        func_obj = (wasm_func_obj_t)(func_ref.gc_obj);
        func_type = wasm_func_obj_get_func_type(func_obj);
        result_count = wasm_func_type_get_result_count(func_type);
        param_count = wasm_func_type_get_param_count(func_type);

        if (is_wasm_source_func && param_count != argc + 1) {
            wasm_runtime_set_exception(
                wasm_runtime_get_module_inst(exec_env),
                "function param count not equal with the real param");
            return NULL;
        }

        /* must add one length to store context */
        bsize = sizeof(uint64) * (argc + 1);
        argv = wasm_runtime_malloc(bsize);
        if (!argv) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }

        /* reserve space for the biggest slots */
        bh_memcpy_s(argv, bsize - occupied_slots, &(context.gc_obj),
                    sizeof(wasm_anyref_obj_t));
        occupied_slots += sizeof(wasm_anyref_obj_t) / sizeof(uint32);

        for (i = 0; i < argc; i++) {
            if (is_wasm_source_func) {
                tmp_param_type =
                    wasm_func_type_get_param_type(func_type, i + 1);
                slot_count = get_slot_count(tmp_param_type);
                unbox_value_from_dyntype(exec_env, ctx, func_args[i],
                                         tmp_param_type, &tmp_param,
                                         slot_count);
            }
            else {
                /* workaround: in onfulfilled func, the wasm function's param
                type is (context type), but the real arg passed from quickjs is
                (JSUndefined).
                    const promiseInst: any = Promise.resolve();
                    promiseInst.then(
                        () => {
                            xxx
                        }
                    );
                */
                if (dyntype_is_undefined_wrapper(exec_env, ctx, func_args[i])) {
                    slot_count = sizeof(void *) / sizeof(uint32);
                    bh_memcpy_s(&tmp_param, sizeof(void *), &func_args[i],
                                sizeof(void *));
                }
                else {
                    tmp_param_type =
                        wasm_func_type_get_param_type(func_type, i + 1);
                    slot_count = get_slot_count(tmp_param_type);
                    unbox_value_from_dyntype(exec_env, ctx, func_args[i],
                                             tmp_param_type, &tmp_param,
                                             slot_count);
                }
            }
            bh_memcpy_s(argv + occupied_slots,
                        bsize - occupied_slots * sizeof(uint32), &tmp_param,
                        slot_count * sizeof(uint32));
            occupied_slots += slot_count;
        }

        is_success =
            wasm_runtime_call_func_ref(exec_env, func_obj, bsize, argv);
        if (!is_success) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "Call wasm func failed");
            /* static throw or dynamic throw can not be defined in compilation
             */
            /* workaround: exception-handling proposal is not implemented in
             * WAMR yet, so where to get the thrown exception in unkown, just
             * pass undefined as exception */
            return dyntype_throw_exception(
                UNBOX_ANYREF(ctx), dyntype_new_undefined(UNBOX_ANYREF(ctx)));
        }

        if (result_count > 0) {
            result_type = wasm_func_type_get_result_type(func_type, 0);
            slot_count = get_slot_count(result_type);
            bh_memcpy_s(&tmp_result, slot_count * sizeof(uint32), argv,
                        slot_count * sizeof(uint32));
            ret = box_value_to_dyntype(exec_env, ctx, &tmp_result, result_type,
                                       slot_count);
        }
        else {
            ret = dyntype_new_undefined_wrapper(exec_env, ctx);
        }

        return ret;
    }
    else {
        ret = dyntype_call_func_wrapper(exec_env, ctx, func_any_obj, argc,
                                        func_args);
        return ret;
    }
}

void *
invoke_func_wrapper(wasm_exec_env_t exec_env, wasm_anyref_obj_t ctx,
                    wasm_anyref_obj_t closure_any_obj,
                    wasm_struct_obj_t args_array)
{
    int i;
    void *ret = NULL;
    wasm_array_obj_t arr_ref = { 0 };
    uint32 argc = 0;
    wasm_value_t *func_value_args = NULL;
    wasm_anyref_obj_t *func_args = NULL;

    argc = get_array_length(args_array);
    arr_ref = get_array_ref(args_array);

    if (argc > 0) {
        func_value_args = wasm_runtime_malloc(sizeof(wasm_value_t) * (argc));
        func_args = wasm_runtime_malloc(sizeof(wasm_anyref_obj_t) * (argc));
        if (!func_value_args || !func_args) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
    }

    for (i = 0; i < argc; i++) {
        wasm_array_obj_get_elem(arr_ref, i, false, func_value_args + i);
        func_args[i] = (wasm_anyref_obj_t)(func_value_args + i)->gc_obj;
    }

    ret = call_wasm_func_with_boxing(exec_env, ctx, closure_any_obj, argc,
                                     func_args, true);

    if (func_value_args) {
        wasm_runtime_free(func_value_args);
    }
    if (func_args) {
        wasm_runtime_free(func_args);
    }

    return ret;
}

dyn_value_t
dyntype_callback_wasm_dispatcher(void *exec_env_v, dyn_ctx_t ctx, void *vfunc,
                                 dyn_value_t this_obj, int argc,
                                 dyn_value_t *args)
{
    wasm_exec_env_t exec_env = exec_env_v;
    uint32_t func_id = (uint32_t)(uintptr_t)vfunc;
    void *closure = NULL;
    void *res = NULL;
    wasm_anyref_obj_t *func_args = NULL;
    wasm_local_obj_ref_t local_refs[10];
    int i;
    wasm_anyref_obj_t ctx_any_obj = NULL;

    if (argc > 0) {
        func_args = wasm_runtime_malloc(sizeof(wasm_obj_t) * (argc));
        if (!func_args) {
            wasm_runtime_set_exception(wasm_runtime_get_module_inst(exec_env),
                                       "alloc memory failed");
            return NULL;
        }
    }

    closure = wamr_utils_get_table_element(exec_env, func_id);

    for (i = 0; i < argc; i++) {
        wasm_anyref_obj_t argi = BOX_ANYREF(args[i]);
        wasm_runtime_push_local_object_ref(exec_env, &local_refs[i]);
        local_refs[i].val = (wasm_obj_t)argi;
        func_args[i] = (wasm_anyref_obj_t)argi;
    }
    ctx_any_obj = BOX_ANYREF(ctx);
    wasm_runtime_push_local_object_ref(exec_env, &local_refs[argc]);
    local_refs[argc].val = (wasm_obj_t)ctx_any_obj;

    res = call_wasm_func_with_boxing(exec_env, ctx_any_obj,
                                     (wasm_anyref_obj_t)closure, argc,
                                     func_args, false);

    wasm_runtime_pop_local_object_refs(exec_env, argc + 1);

    if (func_args) {
        wasm_runtime_free(func_args);
    }

    return UNBOX_ANYREF(res);
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }

static NativeSymbol native_symbols[] = {
    REG_NATIVE_FUNC(dyntype_get_context, "()r"),

    REG_NATIVE_FUNC(dyntype_new_number, "(rF)r"),
    REG_NATIVE_FUNC(dyntype_new_boolean, "(ri)r"),
    REG_NATIVE_FUNC(dyntype_new_string, "(rr)r"),
    REG_NATIVE_FUNC(dyntype_new_undefined, "(r)r"),
    REG_NATIVE_FUNC(dyntype_new_null, "(r)r"),
    REG_NATIVE_FUNC(dyntype_new_object, "(r)r"),
    REG_NATIVE_FUNC(dyntype_new_array_with_length, "(ri)r"),
    REG_NATIVE_FUNC(dyntype_new_array, "(r)r"),
    REG_NATIVE_FUNC(dyntype_add_elem, "(rrr)"),
    REG_NATIVE_FUNC(dyntype_set_elem, "(rrir)"),
    REG_NATIVE_FUNC(dyntype_get_elem, "(rri)r"),
    REG_NATIVE_FUNC(dyntype_new_extref, "(rii)r"),
    REG_NATIVE_FUNC(dyntype_new_object_with_proto, "(rr)r"),

    REG_NATIVE_FUNC(dyntype_set_prototype, "(rrr)i"),
    REG_NATIVE_FUNC(dyntype_get_prototype, "(rr)r"),
    REG_NATIVE_FUNC(dyntype_get_own_property, "(rrir)r"),

    REG_NATIVE_FUNC(dyntype_set_property, "(rr$r)i"),
    REG_NATIVE_FUNC(dyntype_define_property, "(rrrr)i"),
    REG_NATIVE_FUNC(dyntype_get_property, "(rr$)r"),
    REG_NATIVE_FUNC(dyntype_has_property, "(rr$)i"),
    REG_NATIVE_FUNC(dyntype_delete_property, "(rr$)i"),

    REG_NATIVE_FUNC(dyntype_is_undefined, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_null, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_bool, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_number, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_string, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_object, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_array, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_extref, "(rr)i"),

    REG_NATIVE_FUNC(dyntype_to_bool, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_to_number, "(rr)F"),
    REG_NATIVE_FUNC(dyntype_to_cstring, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_to_string, "(rr)r"),
    REG_NATIVE_FUNC(dyntype_to_extref, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_is_falsy, "(rr)i"),

    REG_NATIVE_FUNC(dyntype_free_cstring, "(ri)"),

    REG_NATIVE_FUNC(dyntype_typeof, "(rr)r"),
    REG_NATIVE_FUNC(dyntype_typeof1, "(rr)i"),
    REG_NATIVE_FUNC(dyntype_type_eq, "(rrr)i"),
    REG_NATIVE_FUNC(dyntype_toString, "(rr)r"),
    REG_NATIVE_FUNC(dyntype_cmp, "(rrri)i"),

    REG_NATIVE_FUNC(dyntype_instanceof, "(rrr)i"),

    REG_NATIVE_FUNC(dyntype_new_object_with_class, "(r$r)r"),
    REG_NATIVE_FUNC(dyntype_invoke, "(r$rr)r"),

    REG_NATIVE_FUNC(dyntype_get_global, "(r$)r"),

    REG_NATIVE_FUNC(invoke_func, "(rrr)r"),

    /* TODO */
};
/* clang-format on */

uint32_t
get_libdyntype_symbols(char **p_module_name, NativeSymbol **p_native_symbols)
{
    *p_module_name = "libdyntype";
    *p_native_symbols = native_symbols;
    return sizeof(native_symbols) / sizeof(NativeSymbol);
}
