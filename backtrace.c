#define _GNU_SOURCE

#include "backtrace.h"

#include "log.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <link.h>   // required for __ELF_NATIVE_CLASS

#if __ELF_NATIVE_CLASS == 32
# define WORD_WIDTH 8
#else
// We assume 64bits.
# define WORD_WIDTH 16
#endif

#define ABS(s) ((s) < 0 ? -(s) : (s))

void print_stack_trace(int log_level) {
    void *buffer[64];
    int nptrs = backtrace_mips32(buffer, 64);
    LOG(log_level, "Stack trace: %d frames (most recent call first)", nptrs);

    char **strings = backtrace_symbols(buffer, nptrs);
    for (int i = 0; i < nptrs; ++i) {
        if (!strings) {
            LOG(log_level, "\t#%02d %p", i, buffer[i]);
        } else {
            LOG(log_level, "\t#%02d %s", i, strings[i]);
        }
    }

    if (strings) {
        free(strings);
    }
}

int backtrace_mips32(void **buffer, int size) {
    if (size <= 0 || !buffer) {
        return 0;
    }

    // get current $ra & $sp
    unsigned long *ra;   // return address
    unsigned long *sp;   // stack pointer
    __asm__ __volatile__ (
        "move %0, $ra\n"
        "move %1, $sp\n"
        : "=r"(ra), "=r"(sp)
        );

    // scan this function's code to find the size of the current stack frame
    unsigned long *addr;
    size_t stack_size = 0;
    for (addr = (unsigned long *) backtrace_mips32; !stack_size; ++addr) {
        if ((*addr & 0xffff0000) == 0x27bd0000) {
            stack_size = ABS((short) (*addr & 0xffff));
        } else if (*addr == 0x03e00008) {
            break;
        }
    }

    sp = (unsigned long *) ((unsigned long) sp + stack_size);

    // repeat backward scanning
    size_t ra_offset;
    int depth;
    for (depth = 0; depth < size && ra; ++depth) {
        buffer[depth] = ra;

        ra_offset = 0;
        stack_size = 0;

        for (addr = ra; !ra_offset || !stack_size; --addr) {
            switch (*addr & 0xffff0000) {
            case 0x27bd0000:
                stack_size = ABS((short) (*addr & 0xffff));
                break;

            case 0xafbf0000:
                ra_offset = (short) (*addr & 0xffff);
                break;

            case 0x3c1c0000:
                return depth + 1;

            default:
                break;
            }
        }

        ra = *(unsigned long **) ((unsigned long) sp + ra_offset);
        sp = (unsigned long *) ((unsigned long) sp + stack_size);
    }

    return depth;
}


char **backtrace_symbols(void *const *array, int size) {
    Dl_info info[size];
    int status[size];
    int cnt;
    size_t total = 0;
    char **result;

    /* Fill in the information we can get from `dladdr'.  */
    for (cnt = 0; cnt < size; ++cnt) {
        status[cnt] = dladdr (array[cnt], &info[cnt]);
        if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
        /*
         * We have some info, compute the length of the string which will be
         * "<file-name>(<sym-name>) [+offset].
         */
        total += (strlen (info[cnt].dli_fname ?: "") +
                  (info[cnt].dli_sname ?
                  strlen (info[cnt].dli_sname) + 3 + WORD_WIDTH + 3 : 1)
                  + WORD_WIDTH + 5);
        else
            total += 5 + WORD_WIDTH;
    }

    /* Allocate memory for the result.  */
    result = (char **) malloc (size * sizeof (char *) + total);
    if (result != NULL) {
        char *last = (char *) (result + size);
        for (cnt = 0; cnt < size; ++cnt) {
            result[cnt] = last;

            if (status[cnt] && info[cnt].dli_fname
                && info[cnt].dli_fname[0] != '\0') {

                char buf[20];

                if (array[cnt] >= (void *) info[cnt].dli_saddr)
                    sprintf (buf, "+%#lx",
                            (unsigned long)(array[cnt] - info[cnt].dli_saddr));
                else
                    sprintf (buf, "-%#lx",
                    (unsigned long)(info[cnt].dli_saddr - array[cnt]));

                last += 1 + sprintf (last, "%s%s%s%s%s[%p]",
                info[cnt].dli_fname ?: "",
                info[cnt].dli_sname ? "(" : "",
                info[cnt].dli_sname ?: "",
                info[cnt].dli_sname ? buf : "",
                info[cnt].dli_sname ? ") " : " ",
                array[cnt]);
            } else
                last += 1 + sprintf (last, "[%p]", array[cnt]);
        }
        assert (last <= (char *) result + size * sizeof (char *) + total);
    }

    return result;
}
