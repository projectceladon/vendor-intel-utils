/*
 * Copyright 2017 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * memcpy: An optimized memcpy implementation for x86_64.
 *
 * @author Bin Liu <binliu@fb.com>
 */

#ifndef L
# define L(label)       .L##label
#endif

#ifndef cfi_startproc
# define cfi_startproc  .cfi_startproc
#endif

#ifndef cfi_endproc
# define cfi_endproc    .cfi_endproc
#endif

#ifndef ENTRY
# define ENTRY(name)            \
        .type name,  @function;         \
        .globl name;            \
        .align 16;             \
name:           \
        cfi_startproc
#endif

#ifndef END
# define END(name)              \
        cfi_endproc;            \
        .size name, .-name
#endif



/*
 * _memcpy_short is a local helper used when length < 8. It cannot be called
 * from outside, because it expects a non-standard calling convention:
 *
 *    %rax:  destination buffer address.
 *    %rsi:  source buffer address.
 *    %edx:  length, in the range of [0, 7]
 */

        .type     _memcpy_short, @function
_memcpy_short:
.LSHORT:
       cfi_startproc
        //        if (length == 0) return;
        test      %edx, %edx
        jz        .LEND

        movzbl    (%rsi), %ecx
        //        if (length - 4 < 0) goto LS4;
        sub       $4, %edx
        jb        .LS4

        mov       (%rsi), %ecx
        mov       (%rsi, %rdx), %edi
        mov       %ecx, (%rax)
        mov       %edi, (%rax, %rdx)
.LEND:
       rep
       ret
       nop

.LS4:
        //        At this point, length can be 1 or 2 or 3, and $cl contains
        //        the first byte.
        mov       %cl, (%rax)
                //if (length - 4 + 2 < 0) return;
        add       $2, %edx
        jnc       .LEND

        //        length is 2 or 3 here. In either case, just copy the last
        //        two bytes.
        movzwl    (%rsi, %rdx), %ecx
        mov       %cx, (%rax, %rdx)
        ret
        cfi_endproc
        .size     _memcpy_short, .-_memcpy_short


/*
 * void* memcpy(void* dst, void* src, uint32_t length);
 *
 */
ENTRY(memcpy)
        mov       %rdx, %rcx
        mov       %rdi, %rax
        cmp       $8, %rdx
        jb        .LSHORT

        mov       -8(%rsi, %rdx), %r8
        mov       (%rsi), %r9
        mov       %r8, -8(%rdi, %rdx)
        and       $24, %rcx
        jz        .L32

        mov       %r9, (%rdi)
        mov       %rcx, %r8
        sub       $16, %rcx
        jb        .LT32
        vmovdqu   (%rsi, %rcx), %xmm1
        vmovdqu   %xmm1, (%rdi, %rcx)
        //        Test if there are 32-byte groups
.LT32:
        add       %r8, %rsi
        and       $-32, %rdx
        jnz       .L32_adjDI
        ret

        .align    16
.L32_adjDI:
        add       %r8, %rdi
.L32:
        vmovdqu   (%rsi), %ymm0
        shr       $6, %rdx
        jnc       .L64_32read
        vmovdqu   %ymm0, (%rdi)
        lea       32(%rsi), %rsi
        jnz       .L64_adjDI
        vzeroupper
        ret

.L64_adjDI:
        add       $32, %rdi

.L64:
        vmovdqu   (%rsi), %ymm0

.L64_32read:
        vmovdqu   32(%rsi), %ymm1
        add       $64, %rsi
        vmovdqu   %ymm0, (%rdi)
        vmovdqu   %ymm1, 32(%rdi)
        add       $64, %rdi
        dec       %rdx
        jnz       .L64
        vzeroupper
        ret
END(memcpy)

