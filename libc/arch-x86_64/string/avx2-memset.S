/*
    memset with AVX2
   Copyright (C) 2014-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.
*/

#include <private/bionic_asm.h>


#ifndef L
# define L(label)       .L##label
#endif

ENTRY(__memset_chk)
  # %rdi = dst, %rsi = byte, %rdx = n, %rcx = dst_len
  cmp %rcx, %rdx
  ja __memset_chk_fail
  // Fall through to memset...
END(__memset_chk)


 .section .text.avx2,"ax",@progbits

ENTRY (memset)
	vpxor	%xmm0, %xmm0, %xmm0
	vmovd	%esi, %xmm1
	lea	(%rdi, %rdx), %rsi
	mov	%rdi, %rax
	vpshufb	%xmm0, %xmm1, %xmm0
	cmp	$16, %rdx
	jb	L(less_16bytes)
	cmp	$256, %rdx
	jae	L(256bytesormore)
	cmp	$128, %dl
	jb	L(less_128bytes)
	vmovdqu	%xmm0, (%rdi)
	vmovdqu %xmm0, 0x10(%rdi)
	vmovdqu %xmm0, 0x20(%rdi)
	vmovdqu %xmm0, 0x30(%rdi)
	vmovdqu %xmm0, 0x40(%rdi)
	vmovdqu %xmm0, 0x50(%rdi)
	vmovdqu %xmm0, 0x60(%rdi)
	vmovdqu %xmm0, 0x70(%rdi)
	vmovdqu %xmm0, -0x80(%rsi)
	vmovdqu %xmm0, -0x70(%rsi)
	vmovdqu %xmm0, -0x60(%rsi)
	vmovdqu %xmm0, -0x50(%rsi)
	vmovdqu %xmm0, -0x40(%rsi)
	vmovdqu %xmm0, -0x30(%rsi)
	vmovdqu %xmm0, -0x20(%rsi)
	vmovdqu %xmm0, -0x10(%rsi)
	ret

	.p2align 4
L(less_128bytes):
	cmp	$64, %dl
	jb	L(less_64bytes)
	vmovdqu %xmm0, (%rdi)
	vmovdqu %xmm0, 0x10(%rdi)
	vmovdqu %xmm0, 0x20(%rdi)
	vmovdqu %xmm0, 0x30(%rdi)
	vmovdqu %xmm0, -0x40(%rsi)
	vmovdqu %xmm0, -0x30(%rsi)
	vmovdqu %xmm0, -0x20(%rsi)
	vmovdqu %xmm0, -0x10(%rsi)
	ret

	.p2align 4
L(less_64bytes):
	cmp	$32, %dl
	jb	L(less_32bytes)
	vmovdqu %xmm0, (%rdi)
	vmovdqu %xmm0, 0x10(%rdi)
	vmovdqu %xmm0, -0x20(%rsi)
	vmovdqu %xmm0, -0x10(%rsi)
	ret

	.p2align 4
L(less_32bytes):
	vmovdqu %xmm0, (%rdi)
	vmovdqu %xmm0, -0x10(%rsi)
	ret

	.p2align 4
L(less_16bytes):
	cmp	$8, %dl
	jb	L(less_8bytes)
	vmovq %xmm0, (%rdi)
	vmovq %xmm0, -0x08(%rsi)
	ret

	.p2align 4
L(less_8bytes):
	vmovd	%xmm0, %ecx
	cmp	$4, %dl
	jb	L(less_4bytes)
	mov	%ecx, (%rdi)
	mov	%ecx, -0x04(%rsi)
	ret

	.p2align 4
L(less_4bytes):
	cmp	$2, %dl
	jb	L(less_2bytes)
	mov	%cx, (%rdi)
	mov	%cx, -0x02(%rsi)
	ret

	.p2align 4
L(less_2bytes):
	cmp	$1, %dl
	jb	L(less_1bytes)
	mov	%cl, (%rdi)
L(less_1bytes):
	ret

	.p2align 4
L(256bytesormore):
	vinserti128 $1, %xmm0, %ymm0, %ymm0
	and	$-0x20, %rdi
	add	$0x20, %rdi
	vmovdqu	%ymm0, (%rax)
	sub	%rdi, %rax
	lea	-0x80(%rax, %rdx), %rcx
	cmp	$4096, %rcx
	ja	L(gobble_data)
L(gobble_128_loop):
	vmovdqa	%ymm0, (%rdi)
	vmovdqa	%ymm0, 0x20(%rdi)
	vmovdqa	%ymm0, 0x40(%rdi)
	vmovdqa	%ymm0, 0x60(%rdi)
	sub	$-0x80, %rdi
	add	$-0x80, %ecx
	jb	L(gobble_128_loop)
	mov	%rsi, %rax
	vmovdqu	%ymm0, -0x80(%rsi)
	vmovdqu	%ymm0, -0x60(%rsi)
	vmovdqu	%ymm0, -0x40(%rsi)
	vmovdqu	%ymm0, -0x20(%rsi)
	sub	%rdx, %rax
	vzeroupper
	ret

	.p2align 4
L(gobble_data):
	sub	$-0x80, %rcx
	vmovd	%xmm0, %eax
	rep	stosb
	mov	%rsi, %rax
	sub	%rdx, %rax
	vzeroupper
	ret

END (memset)
