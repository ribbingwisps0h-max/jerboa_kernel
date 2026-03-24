cmake_minimum_required(VERSION 3.28)
project(jerboa_kernel C)

# Устанавливаем стандарт C11
set(CMAKE_C_STANDARD 11)

# Путь к исходникам ядра Ubuntu 24.04
set(KERNEL_SRC "/lib/modules/${CMAKE_HOST_SYSTEM_PROCESSOR}/build")

# Добавляем пути поиска заголовков ядра, чтобы работал автокомплит
include_directories(
        ${KERNEL_SRC}/include
        ${KERNEL_SRC}/arch/x86/include
        ${KERNEL_SRC}/include/uapi
        ${KERNEL_SRC}/arch/x86/include/uapi
)

add_executable(jerboa_kernel jtp_mod.c)