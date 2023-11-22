/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_table.h"
#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"
#if WASM_ENABLE_GC != 0
#include "aot_emit_gc.h"
#endif

uint64
get_tbl_inst_offset(const AOTCompContext *comp_ctx,
                    const AOTFuncContext *func_ctx, uint32 tbl_idx)
{
    uint64 offset = 0, i = 0;
    AOTImportTable *imp_tbls = comp_ctx->comp_data->import_tables;
    AOTTable *tbls = comp_ctx->comp_data->tables;

    offset =
        offsetof(AOTModuleInstance, global_table_data.bytes)
        + (uint64)comp_ctx->comp_data->memory_count * sizeof(AOTMemoryInstance)
        /* Get global data size according to target info */
        + (comp_ctx->pointer_size == sizeof(uint64)
               ? comp_ctx->comp_data->global_data_size_64bit
               : comp_ctx->comp_data->global_data_size_32bit);

    while (i < tbl_idx && i < comp_ctx->comp_data->import_table_count) {
        offset += offsetof(AOTTableInstance, elems);
        /* avoid loading from current AOTTableInstance */
        offset +=
            (uint64)comp_ctx->pointer_size
            * aot_get_imp_tbl_data_slots(imp_tbls + i, comp_ctx->is_jit_mode);
        ++i;
    }

    if (i == tbl_idx) {
        return offset;
    }

    tbl_idx -= comp_ctx->comp_data->import_table_count;
    i -= comp_ctx->comp_data->import_table_count;
    while (i < tbl_idx && i < comp_ctx->comp_data->table_count) {
        offset += offsetof(AOTTableInstance, elems);
        /* avoid loading from current AOTTableInstance */
        offset += (uint64)comp_ctx->pointer_size
                  * aot_get_tbl_data_slots(tbls + i, comp_ctx->is_jit_mode);
        ++i;
    }

    return offset;
}

#if WASM_ENABLE_REF_TYPES != 0 || WASM_ENABLE_GC != 0

LLVMValueRef
aot_compile_get_tbl_inst(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 tbl_idx)
{
    LLVMValueRef offset, tbl_inst;

    if (!(offset =
              I64_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(tbl_inst = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                           func_ctx->aot_inst, &offset, 1,
                                           "tbl_inst"))) {
        HANDLE_FAILURE("LLVMBuildInBoundsGEP");
        goto fail;
    }

    return tbl_inst;
fail:
    return NULL;
}

bool
aot_compile_op_elem_drop(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 tbl_seg_idx)
{
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;
    LLVMValueRef param_values[2], ret_value, func, value;

    /* void aot_drop_table_seg(AOTModuleInstance *, uint32 ) */
    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = VOID_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_drop_table_seg, 2);
    else
        GET_AOT_FUNCTION(aot_drop_table_seg, 2);

    param_values[0] = func_ctx->aot_inst;
    if (!(param_values[1] = I32_CONST(tbl_seg_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* "" means return void */
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 2, ""))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    return true;
fail:
    return false;
}

static bool
aot_check_table_access(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       uint32 tbl_idx, LLVMValueRef elem_idx)
{
    LLVMValueRef offset, tbl_sz, cmp_elem_idx;
    LLVMBasicBlockRef check_elem_idx_succ;

    /* get the cur size of the table instance */
    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, cur_size)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(tbl_sz = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                         func_ctx->aot_inst, &offset, 1,
                                         "cur_size_i8p"))) {
        HANDLE_FAILURE("LLVMBuildInBoundsGEP");
        goto fail;
    }

    if (!(tbl_sz = LLVMBuildBitCast(comp_ctx->builder, tbl_sz, INT32_PTR_TYPE,
                                    "cur_siuze_i32p"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    if (!(tbl_sz = LLVMBuildLoad2(comp_ctx->builder, I32_TYPE, tbl_sz,
                                  "cur_size"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        goto fail;
    }

    /* Check if (uint32)elem index >= table size */
    if (!(cmp_elem_idx = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGE, elem_idx,
                                       tbl_sz, "cmp_elem_idx"))) {
        aot_set_last_error("llvm build icmp failed.");
        goto fail;
    }

    /* Throw exception if elem index >= table size */
    if (!(check_elem_idx_succ = LLVMAppendBasicBlockInContext(
              comp_ctx->context, func_ctx->func, "check_elem_idx_succ"))) {
        aot_set_last_error("llvm add basic block failed.");
        goto fail;
    }

    LLVMMoveBasicBlockAfter(check_elem_idx_succ,
                            LLVMGetInsertBlock(comp_ctx->builder));

    if (!(aot_emit_exception(comp_ctx, func_ctx,
                             EXCE_OUT_OF_BOUNDS_TABLE_ACCESS, true,
                             cmp_elem_idx, check_elem_idx_succ)))
        goto fail;

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_get(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 tbl_idx)
{
    LLVMValueRef elem_idx, offset, func_idx;
    LLVMValueRef table_elem_base, table_elem_addr, table_elem;

    POP_I32(elem_idx);

    if (!aot_check_table_access(comp_ctx, func_ctx, tbl_idx, elem_idx)) {
        goto fail;
    }

    /* load data as i32* */
    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, elems)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(table_elem_base = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                                  func_ctx->aot_inst, &offset,
                                                  1, "table_elem_base_i8p"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }

    /* Load function object reference or function index */
    if (comp_ctx->enable_gc) {
        if (!(table_elem_base =
                  LLVMBuildBitCast(comp_ctx->builder, table_elem_base,
                                   GC_REF_PTR_TYPE, "table_elem_base"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }

        if (!(table_elem_addr = LLVMBuildInBoundsGEP2(
                  comp_ctx->builder, GC_REF_TYPE, table_elem_base, &elem_idx, 1,
                  "table_elem_addr"))) {
            HANDLE_FAILURE("LLVMBuildNUWAdd");
            goto fail;
        }

        if (!(table_elem = LLVMBuildLoad2(comp_ctx->builder, GC_REF_TYPE,
                                          table_elem_addr, "table_elem"))) {
            HANDLE_FAILURE("LLVMBuildLoad");
            goto fail;
        }

        PUSH_GC_REF(table_elem);
    }
    else {
        if (!(table_elem_base =
                  LLVMBuildBitCast(comp_ctx->builder, table_elem_base,
                                   INTPTR_T_PTR_TYPE, "table_elem_base"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }

        if (!(table_elem_addr = LLVMBuildInBoundsGEP2(
                  comp_ctx->builder, INTPTR_T_TYPE, table_elem_base, &elem_idx,
                  1, "table_elem_addr"))) {
            HANDLE_FAILURE("LLVMBuildNUWAdd");
            goto fail;
        }

        if (!(table_elem = LLVMBuildLoad2(comp_ctx->builder, INTPTR_T_TYPE,
                                          table_elem_addr, "table_elem"))) {
            HANDLE_FAILURE("LLVMBuildLoad");
            goto fail;
        }

        if (!(func_idx = LLVMBuildIntCast2(comp_ctx->builder, table_elem,
                                           I32_TYPE, true, "func_idx"))) {
            HANDLE_FAILURE("LLVMBuildIntCast");
            goto fail;
        }

        PUSH_I32(func_idx);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_set(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 tbl_idx)
{
    LLVMValueRef val = NULL, elem_idx, offset, table_elem_base, table_elem_addr;

    if (comp_ctx->enable_gc)
        POP_GC_REF(val);
    else {
        POP_I32(val);

        if (!(val = LLVMBuildIntCast2(comp_ctx->builder, val, INTPTR_T_TYPE,
                                      true, "val_intptr_t"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }
    }

    POP_I32(elem_idx);

    if (!aot_check_table_access(comp_ctx, func_ctx, tbl_idx, elem_idx)) {
        goto fail;
    }

    /* load data as gc_obj_ref* or i32* */
    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, elems)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(table_elem_base = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                                  func_ctx->aot_inst, &offset,
                                                  1, "table_elem_base_i8p"))) {
        HANDLE_FAILURE("LLVMBuildInBoundsGEP");
        goto fail;
    }

    if (comp_ctx->enable_gc) {
        if (!(table_elem_base =
                  LLVMBuildBitCast(comp_ctx->builder, table_elem_base,
                                   GC_REF_PTR_TYPE, "table_elem_base"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }

        if (!(table_elem_addr = LLVMBuildInBoundsGEP2(
                  comp_ctx->builder, GC_REF_TYPE, table_elem_base, &elem_idx, 1,
                  "table_elem_addr"))) {
            HANDLE_FAILURE("LLVMBuildInBoundsGEP");
            goto fail;
        }
    }
    else {
        if (!(table_elem_base =
                  LLVMBuildBitCast(comp_ctx->builder, table_elem_base,
                                   INTPTR_T_PTR_TYPE, "table_elem_base"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }

        if (!(table_elem_addr = LLVMBuildInBoundsGEP2(
                  comp_ctx->builder, INTPTR_T_TYPE, table_elem_base, &elem_idx,
                  1, "table_elem_addr"))) {
            HANDLE_FAILURE("LLVMBuildInBoundsGEP");
            goto fail;
        }
    }

    if (!(LLVMBuildStore(comp_ctx->builder, val, table_elem_addr))) {
        HANDLE_FAILURE("LLVMBuildStore");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_init(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 tbl_idx, uint32 tbl_seg_idx)

{
    LLVMValueRef func, param_values[6], value;
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = I32_TYPE;
    param_types[4] = I32_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = VOID_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_table_init, 6);
    else
        GET_AOT_FUNCTION(aot_table_init, 6);

    param_values[0] = func_ctx->aot_inst;

    if (!(param_values[1] = I32_CONST(tbl_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(param_values[2] = I32_CONST(tbl_seg_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* n */
    POP_I32(param_values[3]);
    /* s */
    POP_I32(param_values[4]);
    /* d */
    POP_I32(param_values[5]);

    /* "" means return void */
    if (!(LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 6,
                         ""))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_copy(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 src_tbl_idx, uint32 dst_tbl_idx)
{
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;
    LLVMValueRef func, param_values[6], value;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = I32_TYPE;
    param_types[4] = I32_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = VOID_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_table_copy, 6);
    else
        GET_AOT_FUNCTION(aot_table_copy, 6);

    param_values[0] = func_ctx->aot_inst;

    if (!(param_values[1] = I32_CONST(src_tbl_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(param_values[2] = I32_CONST(dst_tbl_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* n */
    POP_I32(param_values[3]);
    /* s */
    POP_I32(param_values[4]);
    /* d */
    POP_I32(param_values[5]);

    /* "" means return void */
    if (!(LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 6,
                         ""))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_size(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 tbl_idx)
{
    LLVMValueRef offset, tbl_sz;

    if (!(offset = I32_CONST(get_tbl_inst_offset(comp_ctx, func_ctx, tbl_idx)
                             + offsetof(AOTTableInstance, cur_size)))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(tbl_sz = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                         func_ctx->aot_inst, &offset, 1,
                                         "tbl_sz_ptr_i8"))) {
        HANDLE_FAILURE("LLVMBuildInBoundsGEP");
        goto fail;
    }

    if (!(tbl_sz = LLVMBuildBitCast(comp_ctx->builder, tbl_sz, INT32_PTR_TYPE,
                                    "tbl_sz_ptr"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    if (!(tbl_sz =
              LLVMBuildLoad2(comp_ctx->builder, I32_TYPE, tbl_sz, "tbl_sz"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        goto fail;
    }

    PUSH_I32(tbl_sz);

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_grow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 tbl_idx)
{
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;
    LLVMValueRef func, param_values[4], ret, value;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    ret_type = I32_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_table_grow, 4);
    else
        GET_AOT_FUNCTION(aot_table_grow, 4);

    param_values[0] = func_ctx->aot_inst;

    if (!(param_values[1] = I32_CONST(tbl_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* n */
    POP_I32(param_values[2]);
    /* v */

    if (comp_ctx->enable_gc) {
        POP_GC_REF(param_values[3]);
        if (!(param_values[3] =
                  LLVMBuildBitCast(comp_ctx->builder, param_values[3],
                                   INT8_PTR_TYPE, "table_elem_i8p"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }
    }
    else {
        POP_I32(param_values[3]);
        if (!(param_values[3] =
                  LLVMBuildIntToPtr(comp_ctx->builder, param_values[3],
                                    INT8_PTR_TYPE, "table_elem_i8p"))) {
            HANDLE_FAILURE("LLVMBuildIntToPtr");
            goto fail;
        }
    }

    if (!(ret = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               4, "table_grow"))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    PUSH_I32(ret);

    return true;
fail:
    return false;
}

bool
aot_compile_op_table_fill(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 tbl_idx)
{
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;
    LLVMValueRef func, param_values[5], value;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = I32_TYPE;
    ret_type = VOID_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_table_fill, 5);
    else
        GET_AOT_FUNCTION(aot_table_fill, 5);

    param_values[0] = func_ctx->aot_inst;

    if (!(param_values[1] = I32_CONST(tbl_idx))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    /* n */
    POP_I32(param_values[2]);
    /* v */

    if (comp_ctx->enable_gc) {
        POP_GC_REF(param_values[3]);
        if (!(param_values[3] =
                  LLVMBuildBitCast(comp_ctx->builder, param_values[3],
                                   INT8_PTR_TYPE, "table_elem_i8p"))) {
            HANDLE_FAILURE("LLVMBuildBitCast");
            goto fail;
        }
    }
    else {
        POP_I32(param_values[3]);
        if (!(param_values[3] =
                  LLVMBuildIntToPtr(comp_ctx->builder, param_values[3],
                                    INT8_PTR_TYPE, "table_elem_i8p"))) {
            HANDLE_FAILURE("LLVMBuildIntToPtr");
            goto fail;
        }
    }
    /* i */
    POP_I32(param_values[4]);

    /* "" means return void */
    if (!(LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values, 5,
                         ""))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    return true;
fail:
    return false;
}

#endif /*  WASM_ENABLE_REF_TYPES != 0 || WASM_ENABLE_GC !=0 */
