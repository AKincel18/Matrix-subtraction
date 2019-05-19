.code
MyProc1 proc
;ARGUMENTY
;rcx -> starting row
;rdx -> number of pos
;r8 -> t1
;r9 -> t2
;r10 -> dest matrix

mov rax,[rsp+8*5]
mov r10, rax

mov r11,rcx
imul r11,4

add r8, r11
add r9,r11
add r10,r11


mov rax, r8
mov rbx, r9

et0: jz koniec  ;czy zero
	 js koniec	;czy ujemne
	 movups xmm0, [rax]
	 movups xmm1, [rbx]
	 subps xmm0,xmm1
	 movups [r10], xmm0

	 add rax, 16
	 add rbx, 16
	 add r10, 16
	 sub rdx,4
jmp et0

koniec: ret
MyProc1 endp
end
