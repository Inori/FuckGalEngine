	include \masm32\include\ole32.inc
	includelib \masm32\lib\ole32.lib

.data?
	_BrowseFolderTmp dd ?
.const
	_szDirInfo db 'ÇëÑ¡ÔñÄ¿Â¼£º',0

.code

_BrowseFolderCallBack proc hwnd,uMsg,lParam,lpData
	local @szBuffer[260]:byte
	mov eax,uMsg
	.if eax==BFFM_INITIALIZED
		invoke SendMessage,hwnd,BFFM_SETSELECTION,TRUE,_BrowseFolderTmp
	.elseif eax==BFFM_SELCHANGED
		invoke SHGetPathFromIDList,lParam,addr @szBuffer
		invoke SendMessage,hwnd,BFFM_SETSTATUSTEXT,0,addr @szBuffer
	.endif
	xor eax,eax
	ret
_BrowseFolderCallBack endp
;
_BrowseFolder proc _hwnd,_lpszBuffer
	local @stBrowseInfo:BROWSEINFO
	local @stMalloc
	local @pidlParent,@dwReturn
	
	pushad
	invoke CoInitialize,NULL
	invoke SHGetMalloc,addr @stMalloc
	.if eax==E_FAIL
		mov @dwReturn,FALSE
		jmp @F
	.endif
	invoke RtlZeroMemory,addr @stBrowseInfo,sizeof @stBrowseInfo
	push _hwnd
	pop @stBrowseInfo.hwndOwner
	push _lpszBuffer
	pop _BrowseFolderTmp
	mov @stBrowseInfo.lpfn,offset _BrowseFolderCallBack
	mov @stBrowseInfo.lpszTitle,offset _szDirInfo
	mov @stBrowseInfo.ulFlags,BIF_RETURNONLYFSDIRS OR BIF_STATUSTEXT
	invoke SHBrowseForFolder,addr @stBrowseInfo
	mov @pidlParent,eax
	.if eax!=NULL
		invoke SHGetPathFromIDList,eax,_lpszBuffer
		mov eax,TRUE
	.else
		mov eax,FALSE
	.endif
	
	mov @dwReturn,eax
	mov eax,@stMalloc
	mov eax,[eax]
	invoke (IMalloc ptr [eax]).Free,@stMalloc,@pidlParent
	mov eax,@stMalloc
	mov eax,[eax]
	invoke (IMalloc ptr [eax]).Release,@stMalloc
	
	@@:
	invoke CoUninitialize
	popad
	mov eax,@dwReturn
	ret
_BrowseFolder endp