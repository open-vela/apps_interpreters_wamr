#
# Copyright (C) 2023 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

cmake_minimum_required(VERSION 2.8)

project(libdyntype)

set(QUICKJS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../deps/quickjs)

set(UTILS_DIR ${CMAKE_CURRENT_LIST_DIR}/../utils)
set(LIBQUICKJS_IMPL_DIR ${CMAKE_CURRENT_LIST_DIR}/dyntype_qjs_impl)
set(STRUCT_INDIRECT_DIR ${CMAKE_CURRENT_LIST_DIR}/../struct-indirect)


include_directories(.)
include_directories(${QUICKJS_SRC_DIR})
include_directories(${STRUCT_INDIRECT_DIR})

include_directories(${UTILS_DIR})

set(LIB_QUICKJS
    ${QUICKJS_SRC_DIR}/cutils.c
    ${QUICKJS_SRC_DIR}/libregexp.c
    ${QUICKJS_SRC_DIR}/libunicode.c
    ${QUICKJS_SRC_DIR}/quickjs.c)

set(LIB_DYNTYPE
    ${UTILS_DIR}/type_utils.c
    ${LIBQUICKJS_IMPL_DIR}/context.c
    ${LIBQUICKJS_IMPL_DIR}/fallback.c
    ${LIBQUICKJS_IMPL_DIR}/object.c)

add_library(quickjs ${LIB_QUICKJS})
add_library(dyntype ${LIB_DYNTYPE})
target_link_libraries(dyntype quickjs)

include(${CMAKE_CURRENT_LIST_DIR}/../wamr_config.cmake)

add_library(dyntype_export SHARED lib_dyntype_wrapper.c)
target_link_libraries(dyntype_export dyntype)
