/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __DYNTYPE_H_
#define __DYNTYPE_H_

#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DYNTYPE_FALSE 0
#define DYNTYPE_TRUE 1

#define DYNTYPE_SUCCESS 0
#define DYNTYPE_EXCEPTION 1
#define DYNTYPE_TYPEERR 2

typedef struct DynTypeContext DynTypeContext;

typedef DynTypeContext *dyn_ctx_t;
typedef void dyn_options_t;
typedef void *dyn_value_t;

typedef dyn_value_t (*dyntype_callback_dispatcher_t)(void *env, dyn_ctx_t ctx,
                                                     void *vfunc,
                                                     dyn_value_t this_obj,
                                                     int argc,
                                                     dyn_value_t *args);

typedef enum external_ref_tag {
    ExtObj,
    ExtFunc,
    ExtInfc,
    ExtArray,
} external_ref_tag;

typedef enum dyn_type_t {
    DynUnknown,
    DynNull,
    DynUndefined,
    DynObject,
    DynBoolean,
    DynNumber,
    DynString,
    DynFunction,
    DynSymbol,
    DynBigInt,
    DynExtRefObj,
    DynExtRefFunc,
    DynExtRefInfc,
    DynExtRefArray,
} dyn_type_t;

typedef enum cmp_operator {
    LessThanToken                = 29,
    GreaterThanToken             = 31,
    LessThanEqualsToken          = 32,
    GreaterThanEqualsToken       = 33,
    EqualsEqualsToken            = 34,
    ExclamationEqualsToken       = 35,
    EqualsEqualsEqualsToken      = 36,
    ExclamationEqualsEqualsToken = 37,
} cmp_operator;

/*****************************************************************
*                                                                *
*                          Section 1                             *
*                                                                *
*          Interface exposed to the runtime embedder             *
*                                                                *
*****************************************************************/

/******************* Initialization and destroy *****************/

/**
 * @brief Initialize the dynamic type system context
 *
 * @return dynamic type system context if success, NULL otherwise
 */
dyn_ctx_t
dyntype_context_init();

/**
 * @brief Initialize the dynamic type system context with given options
 *
 * @note options can contain allocator functions and maybe other GC related
 * things
 *
 * @param options options to set (TBD)
 * @return dynamic type system context if success, NULL otherwise
 */
dyn_ctx_t
dyntype_context_init_with_opt(dyn_options_t *options);

/**
 * @brief Destroy the dynamic type system context
 *
 * @param ctx context to destroy
 */
void
dyntype_context_destroy(dyn_ctx_t ctx);

/**
 * @brief Set the callback dispatcher for external functions. When calling
 * dyntype_invoke API, the argument may contain external functions which may be
 * invoked later (e.g. Map.forEach). Libdyntype doesn't know how to invoke the
 * external functions since they are not raw native pointers. The callback
 * dispatcher will be used as a common wrapper for calling all external
 * functions from libdyntype, so the implementer can decide how to invoke the
 * actual function.
 *
 * @note If another callback is set, the previous one will be overwrite.
 *
 * @param ctx the dynamic type system context
 * @param callback the callback to set
 */
void
dyntype_set_callback_dispatcher(dyn_ctx_t ctx,
                                dyntype_callback_dispatcher_t callback);

/******************* event loop *******************/

/**
 * @brief execute pending jobs in micro-tasks of js runtime
 * @param ctx the dynamic type system context
 */
int
dyntype_execute_pending_jobs(dyn_ctx_t ctx);

/******************* Dumping *******************/

/**
 * @brief Dump dynamic error to stdout
 *
 * @param ctx the dynamic type system context
 */
void
dyntype_dump_error(dyn_ctx_t ctx);

/*****************************************************************
*                                                                *
*                          Section 2                             *
*                                                                *
*              Interface exposed to application                  *
*                                                                *
*****************************************************************/

/****************** Context access *****************/

/**
 * @brief Get the global dynamic type system context
 *
 * @return dynamic type system context if success, NULL otherwise
 */
dyn_ctx_t
dyntype_get_context();

/******************* Field access *******************/
/* Creation */

/**
 * @brief Boxing a number to dynamic value
 *
 * @param ctx the dynamic type system context
 * @param value the number to be boxed
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_number(dyn_ctx_t ctx, double value);

/**
 * @brief Boxing a bool to dynamic value
 *
 * @param ctx the dynamic type system context
 * @param value the bool value to be boxed
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_boolean(dyn_ctx_t ctx, bool value);

/**
 * @brief Create a new dynamic string value with the given c-string
 *
 * @note the string must be null-terminated
 *
 * @param ctx the dynamic type system context
 * @param str the string to initialize the dynamic value
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_string(dyn_ctx_t ctx, const char *str);

/**
 * @brief Create a new dynamic string value with the given char* and len
 *
 * @param ctx the dynamic type system context
 * @param str the string to initialize the dynamic value
 * @param len the length of the given string
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_string_with_length(dyn_ctx_t ctx, const char *str, int len);

/**
 * @brief Create a undefined value
 *
 * @param ctx the dynamic type system context
 * @return dynamic undefined value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_undefined(dyn_ctx_t ctx);

/**
 * @brief Create a null value
 *
 * @param ctx the dynamic type system context
 * @return dynamic null value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_null(dyn_ctx_t ctx);

/**
 * @brief Create a new dynamic object without any property
 *
 * @param ctx the dynamic type system context
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_object(dyn_ctx_t ctx);

/**
 * @brief Create a new dynamic JSvalue with the given c JSON string
 *
 * @note the string must be null-terminated
 *
 * @param ctx the dynamic type system context
 * @param value the JSON string to initialize the dynamic value
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_parse_json(dyn_ctx_t ctx, const char *str);

/**
 * @brief Create a new dynamic array object with array length
 *
 * @param ctx the dynamic type system context
 * @param len array length
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_array_with_length(dyn_ctx_t ctx, int len);

/**
 * @brief Create a new dynamic array object without any elements
 *
 * @param ctx the dynamic type system context
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_array(dyn_ctx_t ctx);

/**
 * @brief get builtin global object by name
 *
 * @param name the name of object
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_get_global(dyn_ctx_t ctx, const char *name);

/**
 * @brief Create an object with class name
 *
 * @param ctx the dynamic type system context
 * @param name the name of class
 * @param argc the count of arguments
 * @param args the argument array
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_object_with_class(dyn_ctx_t ctx, const char *name, int argc,
                              dyn_value_t *args);

/**
 * @brief Boxing an external reference to a dynamic value
 *
 * @param ctx the dynamic type system context
 * @param ptr opaque pointer to external reference
 * @param tag external reference tag
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_extref(dyn_ctx_t ctx, void *ptr, external_ref_tag tag, void* opaque);

/**
 * @brief Set the value element of a dynamic object by index.
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param index the index of the element to be set
 * @param elem the value to be set to the element
 * @return 0: SUCCESS, -1: EXCEPTION, -2: TYPE ERROR
 */
int
dyntype_set_elem(dyn_ctx_t ctx, dyn_value_t obj, int index, dyn_value_t elem);

/**
 * @brief Get the value of a dynamic object by index.
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param index the index of the element to be get
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_get_elem(dyn_ctx_t ctx, dyn_value_t obj, int index);

/**
 * @brief Set the property of a dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @param value the value to be set to the property
 * @return 0 if success, error code otherwise
 * @retval -1:EXCEPTION, -2: TYPE ERROR
 */
int
dyntype_set_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop,
                     dyn_value_t value);

/**
 * @brief Define the property of a dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @param value the value to be set to the property
 * @return 0 if success, error code otherwise
 * @retval -1: EXCEPTION, -2: TYPE ERROR
 */
int
dyntype_define_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop,
                        dyn_value_t desc);

/**
 * @brief Get the property of a dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_get_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop);

/**
 * @brief Test if the property exists on the given object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @return TRUE if exists, FALSE if not exists, -1 if EXCEPTION
 */
int
dyntype_has_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop);

/**
 * @brief Delete the property of the given object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @return TRUE if success, FALSE if failed, -1 if EXCEPTION
 */
int
dyntype_delete_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop);

/******************* function fallback *******************/

/**
 * @brief invoke a method by this and method name
 *
 * @param name the name of method name
 * @param this_obj the The object bound by this
 * @param argc the count of arguments
 * @param args the argument array
 * @return dynamic value determined by the method
 */
dyn_value_t
dyntype_invoke(dyn_ctx_t ctx, const char *name, dyn_value_t this_obj, int argc,
               dyn_value_t *args);

/**
 * @brief Call original function object in js runtime
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param argc the count of arguements
 * @param args the arguements
 * @return The returned dynamic value
 */
dyn_value_t
dyntype_call_func(dyn_ctx_t ctx, dyn_value_t obj, int argc, dyn_value_t *args);

/******************* Runtime type checking *******************/
/* undefined and null */
bool
dyntype_is_undefined(dyn_ctx_t ctx, dyn_value_t obj);
bool
dyntype_is_null(dyn_ctx_t ctx, dyn_value_t obj);
/* boolean */
bool
dyntype_is_bool(dyn_ctx_t ctx, dyn_value_t obj);
int
dyntype_to_bool(dyn_ctx_t ctx, dyn_value_t bool_obj, bool *pres);
/* number */
bool
dyntype_is_number(dyn_ctx_t ctx, dyn_value_t obj);
int
dyntype_to_number(dyn_ctx_t ctx, dyn_value_t obj, double *pres);
/* string */
bool
dyntype_is_string(dyn_ctx_t ctx, dyn_value_t obj);
int
dyntype_to_cstring(dyn_ctx_t ctx, dyn_value_t str_obj, char **pres);
void
dyntype_free_cstring(dyn_ctx_t ctx, char *str);
/* object */
bool
dyntype_is_object(dyn_ctx_t ctx, dyn_value_t obj);
/* function */
bool
dyntype_is_function(dyn_ctx_t ctx, dyn_value_t obj);
/* array */
bool
dyntype_is_array(dyn_ctx_t ctx, dyn_value_t obj);
/* extern ref */
bool
dyntype_is_extref(dyn_ctx_t ctx, dyn_value_t obj);
/**
 * @brief Get the extern reference pointer
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param pres [OUTPUT] pointer to the result
 * @return external_ref_tag if success, negative error code otherwise
 */
int
dyntype_to_extref(dyn_ctx_t ctx, dyn_value_t obj, void **pres);

/**
 * @brief Check if a dynamic value is an exception
 *
 * @param ctx the dynamic type system context
 * @param value dynamic object
 * @return TRUE if the value is exception, FALSE otherwise
 */
bool dyntype_is_exception(dyn_ctx_t ctx, dyn_value_t value);

/**
 * @brief Get the value of any type as a bool condition
 *
 * @param ctx the dynamic type system context
 * @param value dynamic object
 * @return TRUE if the value is falsy, FALSE otherwise
 */
bool dyntype_is_falsy(dyn_ctx_t ctx, dyn_value_t value);

/******************* Type equivalence *******************/

/**
 * @brief Get actual type of the dynamic value
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @return type of the dynamic value
 */
dyn_type_t
dyntype_typeof(dyn_ctx_t ctx, dyn_value_t obj);

/**
 * @brief Check if two dynamic value has the same type
 *
 * @param ctx the dynamic type system context
 * @param lhs left hand operand
 * @param rhs right hand operand
 * @return true if the two dynamic values have same type (shape), false
 * otherwise
 */
bool
dyntype_type_eq(dyn_ctx_t ctx, dyn_value_t lhs, dyn_value_t rhs);

/**
 * @brief Compare two dynamic values
 *
 * @param ctx the dynamic type system context
 * @param lhs left hand operand
 * @param rhs right hand operand
 * @param operator_kind the compare operator_kind
 * @return true if the two dynamic values compares are equal, false
 * otherwise
 */
bool
dyntype_cmp(dyn_ctx_t ctx, dyn_value_t lhs, dyn_value_t rhs, cmp_operator operator_kind);

/******************* Subtyping *******************/

/**
 * @brief Create new object with given prototype
 *
 * @param ctx the dynamic type system context
 * @param proto_obj prototype object
 * @return dynamic value if success, NULL otherwise
 */
dyn_value_t
dyntype_new_object_with_proto(dyn_ctx_t ctx, const dyn_value_t proto_obj);

/**
 * @brief Set prototype of the given dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param proto_obj the prototype object
 * @return 0 if success, error code otherwise
 * @retval -1:EXCEPTION, -2: TYPE ERROR
 */
int
dyntype_set_prototype(dyn_ctx_t ctx, dyn_value_t obj,
                      const dyn_value_t proto_obj);

/**
 * @brief Get the prototype of the given dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @return prototype object, NULL if failed
 */
const dyn_value_t
dyntype_get_prototype(dyn_ctx_t ctx, dyn_value_t obj);


/******************* property access *******************/

/**
 * @brief Get own property of the given dynamic object
 *
 * @param ctx the dynamic type system context
 * @param obj dynamic object
 * @param prop property name
 * @return dynamic value of the corresponding property if exists, NULL otherwise
 */
dyn_value_t
dyntype_get_own_property(dyn_ctx_t ctx, dyn_value_t obj, const char *prop);

/**
 * @brief Check if the src object is instance of the dst object
 *
 * @param ctx the dynamic type system context
 * @param src_obj src object
 * @param dst_obj dst object
 * @return true if src object is instance of dst object, false otherwise
 */
bool
dyntype_instanceof(dyn_ctx_t ctx, const dyn_value_t src_obj,
                   const dyn_value_t dst_obj);

/******************* Exception *******************/

/**
 * @brief Throw dynamic exception
 *
 * @param ctx the dynamic type system context
 * @param obj the dynamic exception value
 */
dyn_value_t
dyntype_throw_exception(dyn_ctx_t ctx, dyn_value_t obj);

/******************* Dumping *******************/

/**
 * @brief Dump dynamic value to stdout
 *
 * @param ctx the dynamic type system context
 * @param obj object to be dumped
 */
void
dyntype_dump_value(dyn_ctx_t ctx, dyn_value_t obj);

/**
 * @brief Dump dynamic value to given buffer
 *
 * @param ctx the dynamic type system context
 * @param obj object to be dumped
 * @param buffer buffer to store the dumped message
 * @param len length of the given buffer
 * @return On success, this function return length of bytes dumped to buffer.
 * When failed, a negative error code is returned and content in buffer is
 * undefined
 */
int
dyntype_dump_value_buffer(dyn_ctx_t ctx, dyn_value_t obj, void *buffer,
                          int len);

/******************* Garbage collection *******************/

/**
 * @brief Mark the object
 *
 * @param ctx the dynamic type system context
 * @param obj the dynamic value
 * @return On success, this function return a dyn_value_t which hold a strong
 * reference to the dynamic object, avoid this object to be claimed until this
 * dyn_value_t is freed through dyntype_release.
 */
dyn_value_t
dyntype_hold(dyn_ctx_t ctx, dyn_value_t obj);

/**
 * @brief Release the object
 *
 * @param ctx the dynamic type system context
 * @param obj the dynamic value
 */
void
dyntype_release(dyn_ctx_t ctx, dyn_value_t obj);

/**
 * @brief Start GC collect
 *
 * @param ctx the dynamic type system context
 */
void
dyntype_collect(dyn_ctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif /* end of  __DYNTYPE_H_ */
