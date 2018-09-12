#ifndef BACKTRACE_H_INCLUDED
#define BACKTRACE_H_INCLUDED

/*
    Prints stack trace to log. Read comments on backtrace_mips32() for
    implications of backtracing.
*/
void print_stack_trace(int log_level);

/*
    Notes on building project for reliable extraction of stack traces.

    When debugging with stack traces, you probably want to add
    'CFLAGS += -fno-optimize-sibling-calls' to your makefile as it prevents
    compiler from omitting stack frames for sibling / tail recursive calls.
    You also probably want to add 'CFLAGS += -rdynamic' as it allows to have
    some more symbolic data (i.e. function names in stack trace).
    If you want to use addr2line on your developer machine, you should add
    'CFLAGS += -g' as it will allow addr2line to show you, what specific line
    of code corresponds to the address. After modifying your makefile make sure
    to run 'make clean' and rebuild your project.
    
    When using this library you will receive log output like this (if symbols
    are available):
    Stack trace: 7 frames (most recent call first)
           #00 ./driver_manager(print_stack_trace+0x3c) [0x44afcc]
           #01 ./driver_manager(sendMsgToVoip+0x7c) [0x446b38]
           #02 ./driver_manager(closeCalls+0x1b8) [0x44983c]
           #03 ./driver_manager(keyboardEventHandler+0xec4) [0x43320c]
           #04 ./driver_manager(main+0x848) [0x44a3ec]
           #05 /lib/libc.so.0(__uClibc_main+0x254) [0x2ac7b4d4]
           #06 ./driver_manager(__start+0x54) [0x405004]

    or like this (if symbols aren't available):
    Stack trace: 7 frames (most recent call first)
           #00 0x44afcc
           #01 0x4491b0
           #02 0x4498a0
           #03 0x43320c
           #04 0x44a3ec
           #05 0x2ac7b4d4
           #06 0x405004

    Good news are that you can use addr2line with that output:
    * cd driver_manager/
    * build with -g
    * get stack trace
    * get some address, e.g. 0x43320c and remove '0x' part from it
    * run addr2line -ifC -e driver_manager 43320c
    * have something like that:
      keyboardEventHandler
      /home/.../driver_manager/driver_manager.c:1022
*/

/*
    The following functions are generally the same thing as glibc's backtrace.h.
    For detailed information on how to use these two functions, refer to
    `man backtrace`.

    Implementation of backtrace_mips32() was taken from
    http://elinux.org/images/6/68/ELC2008_-_Back-tracing_in_MIPS-based_Linux_Systems.pdf

    Implementation of backtrace_symbols() was taken from
    https://github.com/hwoarang/uClibc/tree/master-metag/libubacktrace
*/

/*
    Note that this function can't handle stack frames from signal contexts.
    I'm not 100% sure that this function provides stable execution in all
    possible situations, so I recommend to avoid using it in upstream (i.e.
    BE CAREFUL COMMITING CODE USING THIS FUNCTION OR AVOID COMMITING IT AT ALL).
*/
int backtrace_mips32(void **buffer, int size);

char **backtrace_symbols(void *const *array,  int size);

#endif // BACKTRACE_H_INCLUDED
