.486
.model flat,stdcall
option casemap:none


include Toabn.inc

.code

start proc	uses esi;±‰¬“¬Î
	LOCAL @stFindData:WIN32_FIND_DATAW
	LOCAL @hFindFile
	invoke GetProcessHeap
	mov hHeap,eax 
	invoke MessageBox,0,$CTA0("£Ó£Â£Ù    To    ÇéÇÖÇî"),$CTA0("«Î»∑»œ£°£°"),MB_YESNO or MB_ICONASTERISK
	cmp eax,IDNO
	je _Ex
	lea esi,lpNames
	assume esi:ptr _entry
	invoke FindFirstFileW,$CTW0("*.*"),addr @stFindData
	.if eax!=INVALID_HANDLE_VALUE
		mov @hFindFile,eax
		.repeat
			invoke HeapAlloc,hHeap,0,sizeof(_entry)
			or eax,eax
			je _Ex
			mov [esi].next,eax
			mov esi,eax
			mov [esi].next,0
			invoke lstrcpyW,addr [esi].fn2,addr @stFindData.cFileName
			invoke FindNextFileW,@hFindFile,addr @stFindData
		.until eax==FALSE
		invoke FindClose,@hFindFile
	.endif
	
	mov esi,lpNames
	.while esi
		invoke WideCharToMultiByte,932,0,addr [esi].fn2,-1,offset szfn1,1024,0,0
		invoke WideCharToMultiByte,936,0,addr [esi].fn2,-1,offset szfn2,1024,0,0
		invoke MoveFileA,offset szfn2,offset szfn1
		mov esi,[esi].next
	.endw
	assume esi:nothing
	invoke ExitProcess,0
_Ex:
	invoke ExitProcess,0
start endp

end start