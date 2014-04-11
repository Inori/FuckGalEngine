.486
.model flat,stdcall
option casemap:none


include arcpack.inc
include com.inc
include _BrowseFolder.asm

.code

start:
;……………………………………………………………………………………
invoke GetModuleHandle,NULL
mov hInstance,eax
invoke LoadIcon,hInstance,500
mov hIcon,eax
invoke GetProcessHeap
mov hHeap,eax

invoke DialogBoxParam,hInstance,DLG_MAIN,0,offset _DlgMainProc,NULL
invoke ExitProcess,NULL

;……………………………………………………………………………………

_DlgMainProc proc uses ebx edi esi hwnd,uMsg,wParam,lParam
	local @opFileName:OPENFILENAME
	mov eax,uMsg
	.if eax==WM_COMMAND
		mov eax,wParam
		.if ax==IDC_REXP
			invoke GetDlgItem,hwnd,IDC_DIR
			invoke EnableWindow,eax,FALSE
			invoke GetDlgItem,hwnd,IDC_BROWSEDIR
			invoke EnableWindow,eax,FALSE
		.elseif ax==IDC_RCOMP
			invoke GetDlgItem,hwnd,IDC_DIR
			invoke EnableWindow,eax,TRUE
			invoke GetDlgItem,hwnd,IDC_BROWSEDIR
			invoke EnableWindow,eax,TRUE
		.elseif ax==IDC_BROWSEORI
			mov ecx,sizeof @opFileName
			lea edi,@opFileName
			xor eax,eax
			rep stosb
			mov @opFileName.lStructSize,sizeof @opFileName
			push hWinMain
			pop @opFileName.hwndOwner
			mov @opFileName.lpstrFilter,offset szFormat2
			mov @opFileName.lpstrFile,offset szArcName
			mov @opFileName.nMaxFile,1024
			mov @opFileName.Flags,OFN_FILEMUSTEXIST OR OFN_PATHMUSTEXIST
			lea eax,@opFileName
			invoke GetOpenFileName,eax
			.if eax
				invoke SetDlgItemText,hwnd,IDC_ORI,offset szArcName
			.endif
		.elseif ax==IDC_BROWSEDIR
			invoke _BrowseFolder,hwnd,offset szDirName
			.if eax
				invoke SetDlgItemText,hwnd,IDC_DIR,offset szDirName
			.endif
		.elseif ax==IDC_START
			invoke IsDlgButtonChecked,hwnd,IDC_REXP
			.if eax==BST_CHECKED
				invoke GetDlgItemText,hwnd,IDC_ORI,offset szArcName,1024
				invoke _ArcUpk,offset szArcName
			.else
				invoke GetDlgItemText,hwnd,IDC_ORI,offset szArcName,1024
				invoke GetDlgItemText,hwnd,IDC_DIR,offset szDirName,1024
				invoke _ArcPack,offset szArcName,offset szDirName
			.endif
		.endif
	.elseif eax==WM_INITDIALOG
		invoke LoadIcon,hInstance,IDI_APPLICATION
		invoke SendMessage,hwnd,WM_SETICON,ICON_BIG,eax
		mov eax,hwnd
		mov hWinMain,eax
		invoke CheckDlgButton,hwnd,IDC_REXP,BST_CHECKED
	.elseif eax==WM_CLOSE
		invoke EndDialog,hwnd,0
	.else
		mov eax,FALSE
		ret
	.endif
	
	mov eax,TRUE
	ret
_DlgMainProc endp

_ArcUpk proc uses esi edi ebx _lpszName
	LOCAL @hArcFile,@nFileSize,@hArcFileMap,@lpArcFile
	LOCAL @stHdr:_ArcHeader
	LOCAL @lpEntry,@lpNameTable
	LOCAL @bHasErr
	LOCAL @lpNewFileName
	invoke CreateFile,_lpszName,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0
	.if eax==INVALID_HANDLE_VALUE
		invoke MessageBox,hWinMain,$CTA0("无法打开文件！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	mov @hArcFile,eax
	invoke CreateFileMapping,@hArcFile,0,PAGE_READONLY,0,0,0
	.if !eax
		@@:
		invoke MessageBox,hWinMain,$CTA0("文件访问发生错误！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	mov @hArcFileMap,eax
	invoke MapViewOfFile,eax,FILE_MAP_READ,0,0,0
	or eax,eax
	je @B
	mov @lpArcFile,eax
	invoke GetFileSize,@hArcFile,0
	mov @nFileSize,eax
	invoke ReadFile,@hArcFile,addr @stHdr,sizeof _ArcHeader,offset dwTemp,0
;	invoke lstrcmp,addr @stHdr.szMagic,$CTA0("MajiroArcV2.000")
	mov ecx,10h
	lea edi,@stHdr.szMagic
	mov esi,$CTA0("MajiroArcV2.000")
	repe cmpsb
;	.if !ZERO?
;		invoke MessageBox,hWinMain,$CTA0("文件格式错误！"),0,MB_ICONERROR or MB_OK
;		ret
;	.endif
	mov eax,@stHdr.nCount
	mov ecx,sizeof _ArcEntry
	mul ecx
	mov ebx,eax
	invoke HeapAlloc,hHeap,0,eax
	or eax,eax
	je _NomemAU
	mov @lpEntry,eax
	invoke ReadFile,@hArcFile,eax,ebx,offset dwTemp,0
	mov ecx,@stHdr.nOffsetData
	sub ecx,@stHdr.nOffsetNameTable
	mov ebx,ecx
	invoke HeapAlloc,hHeap,0,ecx
	.if !eax
		invoke HeapFree,hHeap,0,@lpEntry
		jmp _NomemAU
	.endif
	mov @lpNameTable,eax
	invoke SetFilePointer,@hArcFile,@stHdr.nOffsetNameTable,0,FILE_BEGIN
	invoke ReadFile,@hArcFile,@lpNameTable,ebx,offset dwTemp,0
	invoke _MakeDir,_lpszName
	.if !eax
		invoke HeapFree,hHeap,0,@lpEntry
		invoke HeapFree,hHeap,0,@lpNameTable
		invoke MessageBox,hWinMain,$CTA0("无法创建文件夹！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	invoke HeapAlloc,hHeap,0,512*2
	.if !eax
		invoke HeapFree,hHeap,0,@lpEntry
		invoke HeapFree,hHeap,0,@lpNameTable
		jmp _NomemAU
	.endif
	mov @lpNewFileName,eax
	xor ebx,ebx
	mov @bHasErr,ebx
	mov esi,@lpEntry
	mov edi,@lpNameTable
	.while ebx<@stHdr.nCount
		assume esi:ptr _ArcEntry
		invoke lstrlen,edi
		mov ecx,edi
		lea edi,[edi+eax+1]
		invoke MultiByteToWideChar,932,0,ecx,-1,@lpNewFileName,512
		invoke CreateFileW,@lpNewFileName,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0
		.if eax==INVALID_HANDLE_VALUE
			mov @bHasErr,1
			inc ebx
			add esi,sizeof _ArcEntry
			.continue
		.endif
		push eax
		mov ecx,@lpArcFile
		add ecx,[esi].nOffset
		invoke WriteFile,eax,ecx,[esi].nLen,offset dwTemp,0
		call CloseHandle
		inc ebx
		add esi,sizeof _ArcEntry
		assume esi:nothing
	.endw
	.if @bHasErr
		mov ecx,$CTA0("解包过程中发生错误，未完全解包！")
	.else
		mov ecx,$CTA0("成功解包！")
	.endif
	invoke MessageBox,hWinMain,ecx,$CTA0("ArcV2 Unpacker"),MB_ICONEXCLAMATION or MB_OK
	invoke HeapFree,hHeap,0,@lpEntry
	invoke HeapFree,hHeap,0,@lpNameTable
	invoke HeapFree,hHeap,0,@lpNewFileName
	invoke UnmapViewOfFile,@lpArcFile
	invoke CloseHandle,@hArcFileMap
	invoke CloseHandle,@hArcFile
	ret
_NomemAU:
	invoke MessageBox,hWinMain,$CTA0("内存不足！"),0,MB_ICONERROR or MB_OK
	ret
_ArcUpk endp

_ArcPack proc _lpszName,_lpszDir
	LOCAL @hArcFile,@nFileSize,@lpEndOfArc
	LOCAL @hNewFile,@lpNewFile,@nNewFileSize
	LOCAL @stHdr:_ArcHeader
	LOCAL @lpEntry,@lpNameTable
	LOCAL @lpNewFileName
	LOCAL @bHasErr
	invoke CreateFile,_lpszName,GENERIC_READ or GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0
	.if eax==INVALID_HANDLE_VALUE
		invoke MessageBox,hWinMain,$CTA0("无法打开文件！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	mov @hArcFile,eax
	invoke SetCurrentDirectory,_lpszDir
	.if !eax
		invoke CloseHandle,@hArcFile
		invoke MessageBox,hWinMain,$CTA0("目录无效！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	invoke GetFileSize,@hArcFile,0
	mov @nFileSize,eax
	invoke ReadFile,@hArcFile,addr @stHdr,sizeof _ArcHeader,offset dwTemp,0
	mov ecx,10h
	lea edi,@stHdr.szMagic
	mov esi,$CTA0("MajiroArcV2.000")
	repe cmpsb
	.if !ZERO?
		invoke CloseHandle,@hArcFile
		invoke MessageBox,hWinMain,$CTA0("文件格式错误！"),0,MB_ICONERROR or MB_OK
		ret
	.endif
	mov eax,@stHdr.nCount
	mov ecx,eax
	shl ecx,2
	lea eax,[ecx+eax*8]
	mov ebx,eax
	invoke HeapAlloc,hHeap,0,eax
	or eax,eax
	je _NomemAP
	mov @lpEntry,eax
	invoke ReadFile,@hArcFile,eax,ebx,offset dwTemp,0
	mov ecx,@stHdr.nOffsetData
	sub ecx,@stHdr.nOffsetNameTable
	mov ebx,ecx
	invoke HeapAlloc,hHeap,0,ecx
	.if !eax
		invoke CloseHandle,@hArcFile
		invoke HeapFree,hHeap,0,@lpEntry
		jmp _NomemAP
	.endif
	mov @lpNameTable,eax
	invoke SetFilePointer,@hArcFile,@stHdr.nOffsetNameTable,0,FILE_BEGIN
	invoke ReadFile,@hArcFile,@lpNameTable,ebx,offset dwTemp,0
	invoke HeapAlloc,hHeap,0,512*2
	.if !eax
		invoke CloseHandle,@hArcFile
		invoke HeapFree,hHeap,0,@lpEntry
		invoke HeapFree,hHeap,0,@lpNameTable
		jmp _NomemAP
	.endif
	mov @lpNewFileName,eax
	xor ebx,ebx
	mov @bHasErr,ebx
	mov esi,@lpEntry
	mov edi,@lpNameTable
	.while ebx<@stHdr.nCount
		assume esi:ptr _ArcEntry
		invoke lstrlen,edi
		mov ecx,edi
		lea edi,[edi+eax+1]
		invoke MultiByteToWideChar,932,0,ecx,-1,@lpNewFileName,512
		invoke CreateFileW,@lpNewFileName,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0
;			invoke MessageBox,hWinMain,ecx,$CTA0("请保证待封装目录的目录结构与解包时完全相同！"),MB_ICONEXCLAMATION or MB_OK
;			jmp _ExAP
		cmp eax,-1
		je _CtnAP
		mov @hNewFile,eax
		invoke GetFileSize,eax,0
		mov @nNewFileSize,eax
		invoke HeapAlloc,hHeap,0,eax
		.if !eax
			invoke CloseHandle,@hNewFile
			mov @bHasErr,1
			jmp _CtnAP
		.endif
		mov @lpNewFile,eax
		invoke ReadFile,@hNewFile,eax,@nNewFileSize,offset dwTemp,0
		mov eax,@nNewFileSize
		.if eax<[esi].nLen
			mov [esi].nLen,eax
			invoke SetFilePointer,@hArcFile,[esi].nOffset,0,FILE_BEGIN
			invoke WriteFile,@hArcFile,@lpNewFile,@nNewFileSize,offset dwTemp,0
		.else
			mov [esi].nLen,eax
			invoke SetFilePointer,@hArcFile,0,0,FILE_END
			mov [esi].nOffset,eax
			invoke WriteFile,@hArcFile,@lpNewFile,@nNewFileSize,offset dwTemp,0
		.endif
		invoke HeapFree,hHeap,0,@lpNewFile
		invoke CloseHandle,@hNewFile
_CtnAP:
		inc ebx
		add esi,0ch
		assume esi:nothing
	.endw
	invoke SetFilePointer,@hArcFile,sizeof(_ArcHeader),0,FILE_BEGIN
	mov eax,@stHdr.nCount
	mov ecx,eax
	shl ecx,2
	lea eax,[ecx+eax*8]
	invoke WriteFile,@hArcFile,@lpEntry,eax,offset dwTemp,0
	.if @bHasErr
		mov ecx,$CTA0("由于内存问题，某些文件未被成功封装！")
	.else
		mov ecx,$CTA0("封装成功！")
	.endif
	invoke MessageBox,hWinMain,ecx,$CTA0("ArcV2 Packer"),MB_ICONEXCLAMATION or MB_OK
_ExAP:
	invoke HeapFree,hHeap,0,@lpEntry
	invoke HeapFree,hHeap,0,@lpNameTable
	invoke HeapFree,hHeap,0,@lpNewFileName
	invoke CloseHandle,@hArcFile
	ret
_NomemAP:
	invoke MessageBox,hWinMain,$CTA0("内存不足！"),0,MB_ICONERROR or MB_OK
	ret
_ArcPack endp

_MakeDir proc uses edi _lpszName
	mov edi,_lpszName
	xor al,al
	or ecx,-1
	repne scasb
	mov edx,edi
	dec edi
	.while byte ptr [edi]!='.'
		dec edi
		.if byte ptr [edi]=='\'
			mov word ptr [edx],'1'
			jmp @F
		.endif
		cmp edi,_lpszName
		je _ErrMD
	.endw
	mov byte ptr [edi],0
	@@:
	invoke SetCurrentDirectory,_lpszName
	.if !eax
		invoke CreateDirectory,_lpszName,0
		or eax,eax
		je _ErrMD
		invoke SetCurrentDirectory,_lpszName
		or eax,eax
		je _ErrMD
	.endif
	mov eax,1
	ret
_ErrMD:
	xor eax,eax
	ret
_MakeDir endp
end start