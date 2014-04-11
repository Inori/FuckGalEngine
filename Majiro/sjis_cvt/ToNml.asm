.486
.model flat,stdcall
option casemap:none


include Tonml.inc

.code

start2 proc	;变正常
	LOCAL @stFindData:WIN32_FIND_DATA
	LOCAL @hFindFile
	invoke GetProcessHeap
	mov hHeap,eax
	invoke MessageBox,0,$CTA0("値倕倲    To    ｎｅｔ"),$CTA0("请确认！！"),MB_YESNO or MB_ICONASTERISK
	cmp eax,IDNO
	je _Ex
	lea esi,lpNames
	assume esi:ptr _entry
	invoke FindFirstFileA,$CTA0("*.*"),addr @stFindData
	.if eax!=INVALID_HANDLE_VALUE
		mov @hFindFile,eax
		xor ebx,ebx
		.repeat
			invoke HeapAlloc,hHeap,0,sizeof(_entry)
			or eax,eax
			je _Ex
			mov [esi].next,eax
			mov esi,eax
			mov [esi].next,0
			invoke lstrcpy,addr [esi].fn2,addr @stFindData.cFileName
			invoke FindNextFileA,@hFindFile,addr @stFindData
		.until eax==FALSE
		invoke FindClose,@hFindFile
	.endif
	
	mov esi,lpNames
	.while esi
		invoke MultiByteToWideChar,932,0,addr [esi].fn2,-1,offset szfn1,512
		invoke MultiByteToWideChar,936,0,addr [esi].fn2,-1,offset szfn2,512
		invoke MoveFileW,offset szfn2,offset szfn1
		mov esi,[esi].next
	.endw
	assume esi:nothing
	invoke ExitProcess,0
_Ex:
	invoke ExitProcess,0
start2 endp

end start2