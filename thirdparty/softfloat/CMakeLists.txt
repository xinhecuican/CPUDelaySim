cmake_minimum_required(VERSION 3.0.0)
project(softfloat)
set(default_build_type "Release")


add_library(softfloat STATIC
    src/softfloat.c
    inc/fpu/softfloat-helpers.h
    inc/fpu/softfloat-macros.h
    inc/fpu/softfloat-parts-addsub.c.inc
    inc/fpu/softfloat-specialize.c.inc
    inc/fpu/softfloat-types.h
    inc/fpu/softfloat.h
    inc/qemu/atomic.h
    inc/qemu/bitops.h
    inc/qemu/bswap.h
    inc/qemu/compiler.h
    inc/qemu/fifo8.h
    inc/qemu/host-utils.h
    inc/qemu/int128.h
    inc/qemu/osdep.h
    inc/qemu/queue.h
    inc/qemu/registerfields.h
)
target_include_directories(softfloat PRIVATE inc/)
