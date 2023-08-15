/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "dyntype.h"
#include "cutils.h"
#include "quickjs.h"
#include <string.h>

typedef struct DynTypeContext {
    JSRuntime *js_rt;
    JSContext *js_ctx;
    JSValue *js_undefined;
    JSValue *js_null;
    dyntype_callback_dispatcher_t cb_dispatcher;
    JSClassID extref_class_id;
    JSValue *extref_class;
} DynTypeContext;
