/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "type.h"

extern JSValue *
dyntype_dup_value(JSContext *ctx, JSValue value);

/******************* function fallback *******************/

/** this_obj is void*, it comes from dyntype_dup_value(), so actually it has
 * type JSValue* so the cast void* to JSValue* is safe.
 */
dyn_value_t
dyntype_invoke(dyn_ctx_t ctx, const char *name, dyn_value_t this_obj, int argc,
               dyn_value_t *args)
{
    JSValue this_val = *(JSValue *)this_obj;
    JSClassCall *call_func = NULL;
    JSAtom atom = find_atom(ctx->js_ctx, name);
    JSValue func = JS_GetProperty(ctx->js_ctx, this_val, atom);
    JSObject *func_obj;
    uint32_t class_id;
    uint64_t total_size;
    JSValue *argv = NULL, v;
    dyn_value_t res = NULL;

    if (!JS_IsFunction(ctx->js_ctx, func)) {
        goto end;
    }

    func_obj = JS_VALUE_GET_OBJ(func);
    class_id = getClassIdFromObject(func_obj);

    call_func = getCallByClassId(ctx->js_rt, class_id);
    if (!call_func) {
        goto end;
    }

    total_size = sizeof(JSValue) * argc;
    if (total_size > 0) {
        argv = js_malloc(ctx->js_ctx, total_size);
        if (!argv) {
            goto end;
        }
    }

    for (int i = 0; i < argc; i++) {
        argv[i] = *(JSValue *)args[i];
    }
    v = call_func(ctx->js_ctx, func, this_val, argc, argv,
                  0); // flags is 0 because quickjs.c:17047

    res = dyntype_dup_value(ctx->js_ctx, v);

end:
    JS_FreeAtom(ctx->js_ctx, atom);
    JS_FreeValue(ctx->js_ctx, func);

    if (argv) {
        js_free(ctx->js_ctx, argv);
    }

    return res;
}

dyn_value_t
dyntype_call_func(dyn_ctx_t ctx, dyn_value_t obj, int argc, dyn_value_t *args)
{
    JSValue obj_value = *(JSValue *)obj;
    JSValue *argv = NULL;

    if (!JS_IsFunction(ctx->js_ctx, obj_value)) {
        return NULL;
    }
    if (argc > 0) {
        argv = js_malloc(ctx->js_ctx, sizeof(JSValue) * argc);
        for (int i = 0; i < argc; i++) {
            argv[i] = *(JSValue *)args[i];
        }
    }

    JSValue ret = JS_Call(ctx->js_ctx, obj_value, JS_UNDEFINED, argc, argv);

    js_free(ctx->js_ctx, argv);
    if (JS_IsException(ret)) {
        return NULL;
    }
    JSValue *ptr = dyntype_dup_value(ctx->js_ctx, ret);

    return ptr;
}

int
dyntype_execute_pending_jobs(dyn_ctx_t ctx)
{
    JSContext *js_ctx1;

    return JS_ExecutePendingJob(JS_GetRuntime(ctx->js_ctx), &js_ctx1);
}