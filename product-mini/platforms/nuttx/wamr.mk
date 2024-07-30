# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

CORE_ROOT := wamr/core
IWASM_ROOT := wamr/core/iwasm
SHARED_ROOT := wamr/core/shared

TS2WASM_RUNTIMELIB_ROOT := $(APPDIR)/frameworks/typescript/ts2wasm/runtime-library
QUICKJS_ROOT := ../quickjs/quickjs
DYNTYPE_ROOT := ${TS2WASM_RUNTIMELIB_ROOT}/libdyntype
STDLIB_ROOT := ${TS2WASM_RUNTIMELIB_ROOT}/stdlib
STRUCT_DYN_ROOT := ${TS2WASM_RUNTIMELIB_ROOT}/struct-dyn
UTILS_ROOT := ${TS2WASM_RUNTIMELIB_ROOT}/utils

ifeq ($(CONFIG_INTERPRETERS_WAMR_USE_SIMPLE_LIBDYNTYPE), y)
CFLAGS += -DUSE_SIMPLE_LIBDYNTYPE=1
LIBDYNTYPE_DYNAMIC_DIR := ${DYNTYPE_ROOT}/dynamic-simple
else
LIBDYNTYPE_DYNAMIC_DIR := ${DYNTYPE_ROOT}/dynamic-qjs
endif
LIBDYNTYPE_EXTREF_DIR := ${DYNTYPE_ROOT}/extref
STRUCT_INDIRECT_DIR := ${TS2WASM_RUNTIMELIB_ROOT}/struct-indirect
STRINGREF_DIR := ${TS2WASM_RUNTIMELIB_ROOT}/stringref


ifeq ($(CONFIG_ARCH_ARMV6M),y)
WAMR_BUILD_TARGET := THUMBV6M
else ifeq ($(CONFIG_ARCH_ARMV7A),y)
WAMR_BUILD_TARGET := THUMBV7
else ifeq ($(CONFIG_ARCH_ARMV7M),y)
WAMR_BUILD_TARGET := THUMBV7EM
else ifeq ($(CONFIG_ARCH_ARMV8M),y)
WAMR_BUILD_TARGET := THUMBV8M
else ifeq ($(CONFIG_ARCH_ARM64),y)
WAMR_BUILD_TARGET := AARCH64
else ifeq ($(CONFIG_ARCH_X86),y)
WAMR_BUILD_TARGET := X86_32
else ifeq ($(CONFIG_ARCH_X86_64),y)
WAMR_BUILD_TARGET := X86_64
else ifeq ($(CONFIG_ARCH_XTENSA),y)
WAMR_BUILD_TARGET := XTENSA
# RV64GC and RV32IM used in older
# version NuttX
else ifeq ($(CONFIG_ARCH_RV64GC),y)
WAMR_BUILD_TARGET := RISCV64
else ifeq ($(CONFIG_ARCH_RV32IM),y)
WAMR_BUILD_TARGET := RISCV32
else ifeq ($(CONFIG_ARCH_RV64),y)
WAMR_BUILD_TARGET := RISCV64
else ifeq ($(CONFIG_ARCH_RV32),y)
WAMR_BUILD_TARGET := RISCV32
else ifeq ($(CONFIG_ARCH_SIM),y)
ifeq ($(CONFIG_SIM_M32),y)
WAMR_BUILD_TARGET := X86_32
else ifeq ($(CONFIG_HOST_X86),y)
WAMR_BUILD_TARGET := X86_32
else ifeq ($(CONFIG_HOST_ARM),y)
WAMR_BUILD_TARGET := ARM
else ifeq ($(CONFIG_HOST_ARM64),y)
WAMR_BUILD_TARGET := AARCH64
else
WAMR_BUILD_TARGET := X86_64
endif
ifeq ($(CONFIG_HOST_MACOS),y)
# Note: invokeNative_em64.s needs BH_PLATFORM_DARWIN
AFLAGS += -DBH_PLATFORM_DARWIN
endif
endif

WAMR_BUILD_PLATFORM := nuttx

CFLAGS += -DBH_MALLOC=wasm_runtime_malloc
CFLAGS += -DBH_FREE=wasm_runtime_free
CFLAGS += -Wno-format
CFLAGS += -Wno-pointer-sign

ifeq ($(WAMR_BUILD_TARGET), X86_32)
  CFLAGS += -DBUILD_TARGET_X86_32
  INVOKE_NATIVE := invokeNative_ia32.s
  AOT_RELOC := aot_reloc_x86_32.c
else ifeq ($(WAMR_BUILD_TARGET), X86_64)
  CFLAGS += -DBUILD_TARGET_X86_64
  INVOKE_NATIVE := invokeNative_em64.s
  AOT_RELOC := aot_reloc_x86_64.c
else ifeq ($(WAMR_BUILD_TARGET), AARCH64)
  CFLAGS += -DBUILD_TARGET_AARCH64
  CFLAGS += -DBUILD_TARGET=\"$(WAMR_BUILD_TARGET)\"
  INVOKE_NATIVE := invokeNative_aarch64.s
  AOT_RELOC := aot_reloc_aarch64.c
else ifeq ($(findstring ARM,$(WAMR_BUILD_TARGET)), ARM)
  CFLAGS += -DBUILD_TARGET_ARM
  CFLAGS += -DBUILD_TARGET=\"$(WAMR_BUILD_TARGET)\"
  INVOKE_NATIVE := invokeNative_arm.s
  AOT_RELOC := aot_reloc_arm.c
else ifeq ($(findstring THUMB,$(WAMR_BUILD_TARGET)), THUMB)
  CFLAGS += -DBUILD_TARGET=\"$(WAMR_BUILD_TARGET)\"
  ifeq ($(CONFIG_ARCH_FPU),y)
  CFLAGS += -DBUILD_TARGET_THUMB_VFP
  INVOKE_NATIVE := invokeNative_thumb_vfp.s
  else
  CFLAGS += -DBUILD_TARGET_THUMB
  INVOKE_NATIVE := invokeNative_thumb.s
  endif
  AOT_RELOC := aot_reloc_thumb.c
else ifeq (${WAMR_BUILD_TARGET}, MIPS)
  CFLAGS += -DBUILD_TARGET_MIPS
  INVOKE_NATIVE := invokeNative_mips.s
  AOT_RELOC := aot_reloc_mips.c
else ifeq (${WAMR_BUILD_TARGET}, XTENSA)
  CFLAGS += -DBUILD_TARGET_XTENSA
  INVOKE_NATIVE := invokeNative_xtensa.s
  AOT_RELOC := aot_reloc_xtensa.c
else ifeq (${WAMR_BUILD_TARGET}, RISCV64)

ifeq (${CONFIG_ARCH_DPFPU},y)
  CFLAGS += -DBUILD_TARGET_RISCV64_LP64D
else ifneq (${CONFIG_ARCH_FPU},y)
  CFLAGS += -DBUILD_TARGET_RISCV64_LP64
else
  $(error riscv64 lp64f is unsupported)
endif
  INVOKE_NATIVE += invokeNative_riscv.S

  AOT_RELOC := aot_reloc_riscv.c

else ifeq (${WAMR_BUILD_TARGET}, RISCV32)

ifeq (${CONFIG_ARCH_DPFPU},y)
  CFLAGS += -DBUILD_TARGET_RISCV32_ILP32D
else ifneq (${CONFIG_ARCH_FPU},y)
  CFLAGS += -DBUILD_TARGET_RISCV32_ILP32
else
  $(error riscv32 ilp32f is unsupported)
endif

  INVOKE_NATIVE += invokeNative_riscv.S
  AOT_RELOC := aot_reloc_riscv.c

else
  $(error Build target is unsupported)
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LOG),y)
CFLAGS += -DWASM_ENABLE_LOG=1
else
CFLAGS += -DWASM_ENABLE_LOG=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_AOT),y)
CFLAGS += -I$(IWASM_ROOT)/aot
CFLAGS += -DWASM_ENABLE_AOT=1
CSRCS += $(IWASM_ROOT)/aot/aot_loader.c \
         $(IWASM_ROOT)/aot/arch/$(AOT_RELOC) \
         $(IWASM_ROOT)/aot/aot_intrinsic.c \
         $(IWASM_ROOT)/aot/aot_runtime.c
else
CFLAGS += -DWASM_ENABLE_AOT=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_AOT_WORD_ALIGN_READ),y)
CFLAGS += -DWASM_ENABLE_WORD_ALIGN_READ=1
else
CFLAGS += -DWASM_ENABLE_WORD_ALIGN_READ=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MEM_DUAL_BUS_MIRROR),y)
CFLAGS += -DWASM_MEM_DUAL_BUS_MIRROR=1
else
CFLAGS += -DWASM_MEM_DUAL_BUS_MIRROR=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_FAST), y)
CFLAGS += -DWASM_ENABLE_FAST_INTERP=1
CFLAGS += -DWASM_ENABLE_INTERP=1
CSRCS += wasm_interp_fast.c
CSRCS += wasm_runtime.c
else
CFLAGS += -DWASM_ENABLE_FAST_INTERP=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_CLASSIC), y)
CFLAGS += -DWASM_ENABLE_INTERP=1
CSRCS += wasm_interp_classic.c
CSRCS += wasm_runtime.c
endif

ifeq ($(findstring y,$(CONFIG_INTERPRETERS_WAMR_FAST)$(CONFIG_INTERPRETERS_WAMR_CLASSIC)), y)
ifeq ($(CONFIG_INTERPRETERS_WAMR_MINILOADER),y)
CFLAGS += -DWASM_ENABLE_MINI_LOADER=1
CSRCS += wasm_mini_loader.c
else
CFLAGS += -DWASM_ENABLE_MINI_LOADER=0
CSRCS += wasm_loader.c
endif
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_DEBUG_INTERP),y)
# Note: INTERPRETERS_WAMR_CLASSIC/INTERPRETERS_WAMR_THREAD_MGR
# dependencies are already handled in NuttX apps Kconfig
CFLAGS += -DWASM_ENABLE_DEBUG_INTERP=1
CFLAGS += -I$(IWASM_ROOT)/libraries/debug-engine
CSRCS += debug_engine.c
CSRCS += gdbserver.c
CSRCS += handler.c
CSRCS += packets.c
CSRCS += utils.c
VPATH += $(IWASM_ROOT)/libraries/debug-engine
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_STACK_GUARD_SIZE),)
CFLAGS += -DWASM_STACK_GUARD_SIZE=0
else
CFLAGS += -DWASM_STACK_GUARD_SIZE=CONFIG_INTERPRETERS_WAMR_STACK_GUARD_SIZE
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_SHARED_MEMORY),y)
CFLAGS += -DWASM_ENABLE_SHARED_MEMORY=1
CSRCS += wasm_shared_memory.c
else
CFLAGS += -DWASM_ENABLE_SHARED_MEMORY=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_BULK_MEMORY),y)
CFLAGS += -DWASM_ENABLE_BULK_MEMORY=1
else
CFLAGS += -DWASM_ENABLE_BULK_MEMORY=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_AOT_STACK_FRAME), y)
CFLAGS += -DWASM_ENABLE_AOT_STACK_FRAME=1
else
CFLAGS += -DWASM_ENABLE_AOT_STACK_FRAME=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_PERF_PROFILING),y)
CFLAGS += -DWASM_ENABLE_PERF_PROFILING=1
CFLAGS += -DWASM_ENABLE_AOT_STACK_FRAME=1
else
CFLAGS += -DWASM_ENABLE_PERF_PROFILING=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_GC_PERF_PROFILING),y)
CFLAGS += -DWASM_ENABLE_GC_PERF_PROFILING=1
else
CFLAGS += -DWASM_ENABLE_GC_PERF_PROFILING=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MEMORY_PROFILING),y)
CFLAGS += -DWASM_ENABLE_MEMORY_PROFILING=1
else
CFLAGS += -DWASM_ENABLE_MEMORY_PROFILING=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MEMORY_TRACING),y)
CFLAGS += -DWASM_ENABLE_MEMORY_TRACING=1
else
CFLAGS += -DWASM_ENABLE_MEMORY_TRACING=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_DUMP_CALL_STACK),y)
CFLAGS += -DWASM_ENABLE_DUMP_CALL_STACK=1
CFLAGS += -DWASM_ENABLE_AOT_STACK_FRAME=1
else
CFLAGS += -DWASM_ENABLE_DUMP_CALL_STACK=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIBC_BUILTIN),y)
CFLAGS += -DWASM_ENABLE_LIBC_BUILTIN=1
CSRCS += $(IWASM_ROOT)/libraries/libc-builtin/libc_builtin_wrapper.c
VPATH += $(IWASM_ROOT)/libraries/libc-builtin
else
CFLAGS += -DWASM_ENABLE_LIBC_BUILTIN=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_CONFIGURABLE_BOUNDS_CHECKS),y)
CFLAGS += -DWASM_CONFIGURABLE_BOUNDS_CHECKS=1
else
CFLAGS += -DWASM_CONFIGURABLE_BOUNDS_CHECKS=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIBC_WASI),y)
CFLAGS += -DWASM_ENABLE_LIBC_WASI=1
CFLAGS += -I$(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/src
CFLAGS += -I$(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/include
CFLAGS += -I${SHARED_ROOT}/platform/common/libc-util
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/blocking_op.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/posix_socket.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/posix_file.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/posix_clock.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/libc_errno.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/libc_wasi_wrapper.c
VPATH += $(IWASM_ROOT)/libraries/libc-wasi
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/src/posix.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/src/random.c
CSRCS += $(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/src/str.c
VPATH += $(IWASM_ROOT)/libraries/libc-wasi/sandboxed-system-primitives/src
# todo: use Kconfig select instead
CONFIG_INTERPRETERS_WAMR_MODULE_INSTANCE_CONTEXT = y
else
CFLAGS += -DWASM_ENABLE_LIBC_WASI=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MODULE_INSTANCE_CONTEXT),y)
CFLAGS += -DWASM_ENABLE_MODULE_INST_CONTEXT=1
else
CFLAGS += -DWASM_ENABLE_MODULE_INST_CONTEXT=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_MULTI_MODULE),y)
CFLAGS += -DWASM_ENABLE_MULTI_MODULE=1
else
CFLAGS += -DWASM_ENABLE_MULTI_MODULE=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_THREAD_MGR),y)
CFLAGS += -DWASM_ENABLE_THREAD_MGR=1
CSRCS += $(IWASM_ROOT)/libraries/thread-mgr/thread_manager.c
VPATH += $(IWASM_ROOT)/libraries/thread-mgr
else
CFLAGS += -DWASM_ENABLE_THREAD_MGR=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIB_WASI_THREADS),y)
CFLAGS += -DWASM_ENABLE_LIB_WASI_THREADS=1
CSRCS += $(IWASM_ROOT)/libraries/lib-wasi-threads/lib_wasi_threads_wrapper.c
CSRCS += $(IWASM_ROOT)/libraries/lib-wasi-threads/tid_allocator.c
VPATH += $(IWASM_ROOT)/libraries/lib-wasi-threads
else
CFLAGS += -DWASM_ENABLE_LIB_WASI_THREADS=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_GC),y)
CFLAGS += -DWASM_ENABLE_GC=1
CFLAGS += -DWASM_ENABLE_REF_TYPES=1
CFLAGS += -DWASM_ENABLE_GC_BINARYEN=1
CFLAGS += -DWASM_ENABLE_SPEC=0
CFLAGS += -DWASM_ENABLE_STRINGREF=1

VPATH += $(IWASM_ROOT)/common/gc
VPATH += ${DYNTYPE_ROOT}
VPATH += ${STDLIB_ROOT}
VPATH += ${STRUCT_DYN_ROOT}
VPATH += ${UTILS_ROOT}
VPATH += ${TS2WASM_RUNTIMELIB_ROOT}
VPATH += ${LIBDYNTYPE_DYNAMIC_DIR}
VPATH += ${STRUCT_INDIRECT_DIR}
VPATH += ${LIBDYNTYPE_EXTREF_DIR}
VPATH += ${STRINGREF_DIR}

CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/context.c \
         ${LIBDYNTYPE_DYNAMIC_DIR}/fallback.c \
         ${LIBDYNTYPE_DYNAMIC_DIR}/object.c \
         ${LIBDYNTYPE_EXTREF_DIR}/extref.c \
         ${DYNTYPE_ROOT}/libdyntype.c \
         ${DYNTYPE_ROOT}/lib_dyntype_wrapper.c \
         ${STDLIB_ROOT}/lib_array.c \
         ${STDLIB_ROOT}/lib_console.c \
         ${STDLIB_ROOT}/lib_timer.c \
         ${UTILS_ROOT}/type_utils.c \
         ${UTILS_ROOT}/wamr_utils.c \
         ${UTILS_ROOT}/object_utils.c \
         ${STRUCT_INDIRECT_DIR}/lib_struct_indirect.c \
         $(IWASM_ROOT)/common/gc/gc_type.c  \
         $(IWASM_ROOT)/common/gc/gc_object.c  \
         $(IWASM_ROOT)/common/gc/gc_common.c

override MAINSRC = ${TS2WASM_RUNTIMELIB_ROOT}/main.c
override PROGNAME  = iwasm
export MAINSRC
export PROGNAME

ifeq ($(CONFIG_INTERPRETERS_WAMR_USE_SIMPLE_LIBDYNTYPE), y)
CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value/dyn_value.c
CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value/class/date.c
CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value/class/dyn_class.c
CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value/class/object.c
CSRCS += ${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value/class/string.c
CSRCS += ${STRINGREF_DIR}/stringref_simple.c
CFLAGS += -I${LIBDYNTYPE_DYNAMIC_DIR}/dyn-value
else
CSRCS += ${STRINGREF_DIR}/stringref_qjs.c
endif

else
CFLAGS += -DWASM_ENABLE_GC=0
# CFLAGS += -DWASM_ENABLE_REF_TYPES = 0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_GC_MANUALLY),y)
CFLAGS += -DWASM_GC_MANUALLY=1
else
CFLAGS += -DWASM_GC_MANUALLY=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIB_PTHREAD),y)
CFLAGS += -DWASM_ENABLE_LIB_PTHREAD=1
CSRCS += $(IWASM_ROOT)/libraries/lib-pthread/lib_pthread_wrapper.c
else
CFLAGS += -DWASM_ENABLE_LIB_PTHREAD=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_LIB_PTHREAD_SEMAPHORE),y)
CFLAGS += -DWASM_ENABLE_LIB_PTHREAD_SEMAPHORE=1
else
CFLAGS += -DWASM_ENABLE_LIB_PTHREAD_SEMAPHORE=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_DISABLE_HW_BOUND_CHECK),y)
CFLAGS += -DWASM_DISABLE_HW_BOUND_CHECK=1
CFLAGS += -DWASM_DISABLE_STACK_HW_BOUND_CHECK=1
else
CFLAGS += -DWASM_DISABLE_HW_BOUND_CHECK=0
CFLAGS += -DWASM_DISABLE_STACK_HW_BOUND_CHECK=0
endif

# REVISIT: is this worth to have a Kconfig?
CFLAGS += -DWASM_DISABLE_WAKEUP_BLOCKING_OP=0

ifeq ($(CONFIG_INTERPRETERS_WAMR_LOAD_CUSTOM_SECTIONS),y)
CFLAGS += -DWASM_ENABLE_LOAD_CUSTOM_SECTION=1
else
CFLAGS += -DWASM_ENABLE_LOAD_CUSTOM_SECTION=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_CUSTOM_NAME_SECTIONS),y)
CFLAGS += -DWASM_ENABLE_CUSTOM_NAME_SECTION=1
else
CFLAGS += -DWASM_ENABLE_CUSTOM_NAME_SECTION=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_GLOBAL_HEAP_POOL),y)
CFLAGS += -DWASM_ENABLE_GLOBAL_HEAP_POOL=1
CFLAGS += -DWASM_GLOBAL_HEAP_SIZE="$(CONFIG_INTERPRETERS_WAMR_GLOBAL_HEAP_POOL_SIZE) * 1024"
else
CFLAGS += -DWASM_ENABLE_GLOBAL_HEAP_POOL=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_ENABLE_SPEC_TEST),y)
CFLAGS += -DWASM_ENABLE_SPEC_TEST=1
else
CFLAGS += -DWASM_ENABLE_SPEC_TEST=0
endif

ifeq ($(CONFIG_INTERPRETERS_WAMR_REF_TYPES),y)
CFLAGS += -DWASM_ENABLE_REF_TYPES=1
else
CFLAGS += -DWASM_ENABLE_REF_TYPES=0
endif



CFLAGS += -Wno-strict-prototypes -Wno-shadow -Wno-unused-variable
CFLAGS += -Wno-int-conversion -Wno-implicit-function-declaration

CFLAGS += -I${CORE_ROOT} \
          -I${IWASM_ROOT}/include \
          -I${IWASM_ROOT}/interpreter \
          -I${IWASM_ROOT}/common \
          -I${IWASM_ROOT}/libraries/thread-mgr \
          -I${SHARED_ROOT}/include \
          -I${SHARED_ROOT}/platform/include \
          -I${SHARED_ROOT}/utils \
          -I${SHARED_ROOT}/utils/uncommon \
          -I${SHARED_ROOT}/mem-alloc \
          -I${SHARED_ROOT}/platform/nuttx \
          -I${IWASM_ROOT}/common/gc \
          -I${IWASM_ROOT}/common/gc/stringref \
          -I${QUICKJS_ROOT} \
          -I${DYNTYPE_ROOT} \
          -I${STDLIB_ROOT} \
          -I${STRUCT_DYN_ROOT} \
          -I${UTILS_ROOT} \
          -I${LIBDYNTYPE_DYNAMIC_DIR} \
          -I${STRUCT_INDIRECT_DIR}

ifeq ($(WAMR_BUILD_INTERP), 1)
CFLAGS += -I$(IWASM_ROOT)/interpreter
endif

CSRCS += ${SHARED_ROOT}/platform/nuttx/nuttx_platform.c \
         $(SHARED_ROOT)/platform/common/posix/posix_blocking_op.c \
         $(SHARED_ROOT)/platform/common/posix/posix_thread.c \
         $(SHARED_ROOT)/platform/common/posix/posix_time.c \
         $(SHARED_ROOT)/platform/common/posix/posix_sleep.c \
         $(SHARED_ROOT)/platform/common/memory/mremap.c \
         ${SHARED_ROOT}/mem-alloc/mem_alloc.c \
         ${SHARED_ROOT}/mem-alloc/ems/ems_kfc.c \
         ${SHARED_ROOT}/mem-alloc/ems/ems_alloc.c \
         ${SHARED_ROOT}/mem-alloc/ems/ems_hmu.c \
         ${SHARED_ROOT}/mem-alloc/ems/ems_gc.c \
         ${SHARED_ROOT}/utils/bh_assert.c \
         ${SHARED_ROOT}/utils/bh_bitmap.c \
         ${SHARED_ROOT}/utils/bh_common.c \
         ${SHARED_ROOT}/utils/bh_hashmap.c \
         ${SHARED_ROOT}/utils/bh_list.c \
         ${SHARED_ROOT}/utils/bh_log.c \
         ${SHARED_ROOT}/utils/bh_queue.c \
         ${SHARED_ROOT}/utils/bh_vector.c \
         ${SHARED_ROOT}/utils/uncommon/bh_read_file.c \
         ${SHARED_ROOT}/utils/runtime_timer.c \
         ${IWASM_ROOT}/common/wasm_application.c \
         ${IWASM_ROOT}/common/wasm_blocking_op.c \
         ${IWASM_ROOT}/common/wasm_runtime_common.c \
         ${IWASM_ROOT}/common/wasm_native.c \
         ${IWASM_ROOT}/common/wasm_exec_env.c \
         ${IWASM_ROOT}/common/wasm_memory.c \
         ${IWASM_ROOT}/common/wasm_loader_common.c \
         ${IWASM_ROOT}/common/wasm_c_api.c \

ASRCS += $(INVOKE_NATIVE)

VPATH += $(SHARED_ROOT)/platform/nuttx
VPATH += $(SHARED_ROOT)/platform/common/posix
VPATH += $(SHARED_ROOT)/platform/common/libc-util
VPATH += $(SHARED_ROOT)/mem-alloc
VPATH += $(SHARED_ROOT)/mem-alloc/ems
VPATH += $(SHARED_ROOT)/utils
VPATH += $(SHARED_ROOT)/utils/uncommon
VPATH += $(IWASM_ROOT)/common
VPATH += $(IWASM_ROOT)/interpreter
VPATH += $(IWASM_ROOT)/libraries
VPATH += $(IWASM_ROOT)/libraries/lib-pthread
VPATH += $(IWASM_ROOT)/common/arch
VPATH += $(IWASM_ROOT)/aot
VPATH += $(IWASM_ROOT)/aot/arch
VPATH += ${QUICKJS_ROOT}