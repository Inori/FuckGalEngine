;KRKR 脚本解密
;文件头标志：FE FE 01 FF FE

.386
.model flat, stdcall
option casemap: none

include kernel32.inc
include user32.inc
include windows.inc
include masm32.inc

includelib kernel32.lib
includelib user32.lib
includelib masm32.lib

.const
    szUsage db 'KrkrSrcDecrypt.exe <input.ks>', 0dh, 0ah, 0
    byUTF16Bom db 0FFh, 0FEh
    szExt db '.txt', 0
    
.code

;解密文本
Decode proc pBuffer, dwLen
	mov     eax, pBuffer
	mov     ebx, dwLen
    xor     ecx, ecx
@@:
	mov     dx, word ptr [eax]           
	movzx   esi, dx
	and     dx, 5555h
	and     esi, 0AAAAAAAAh
	shr     esi, 1
	add     edx, edx
	or      si, dx
	mov     edx, esi
	mov     word ptr [eax], dx
	inc     ecx
	add     eax, 2h
	cmp     ebx, ecx
	ja      @B
    ret
Decode endp
    
main proc
    local hInFile   :DWORD
    local hOutFile  :DWORD
    
    local hHeap     :DWORD
    local pBuffer   :DWORD
    local dwSize    :DWORD
    local dwEncCount:DWORD
    
    local szInFile[260]: BYTE
    local szOutFile[260]:BYTE
    
    local dwRead    :DWORD
    local dwWrite   :DWORD
    
    invoke GetCL, 1, addr szInFile
    .if eax != 1
        invoke StdOut, offset szUsage
        jmp quit
    .endif
    
    ;读文件
    invoke CreateFile, addr szInFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, NULL, NULL
    mov hInFile, eax
    .if hInFile == INVALID_HANDLE_VALUE
        jmp quit
    .endif
    
    invoke GetFileSize, hInFile, NULL
    mov dwSize, eax
    .if dwSize <= 5
        jmp quit
    .endif
    
    sub dwSize, 5
    
    invoke GetProcessHeap
    mov hHeap, eax
    invoke HeapAlloc, hHeap, 0, dwSize
    mov pBuffer, eax
    
    invoke SetFilePointer, hInFile, 5, NULL, FILE_BEGIN
    invoke ReadFile, hInFile, pBuffer, dwSize, addr dwRead, NULL
    invoke CloseHandle, hInFile
    
    mov eax, dwSize
    shr eax, 1
    mov dwEncCount, eax
    
    ;解密
    invoke Decode, pBuffer, dwEncCount
    
    ;写新文件
    invoke szCatStr, addr szOutFile, addr szInFile
    invoke szCatStr, addr szOutFile, offset szExt
    
    invoke CreateFile, addr szOutFile, GENERIC_WRITE, 0, NULL, CREATE_NEW, NULL, NULL
    mov hOutFile, eax
    .if hOutFile == INVALID_HANDLE_VALUE
        jmp quit
    .endif
    
    invoke WriteFile, hOutFile, offset byUTF16Bom, 2, addr dwWrite, NULL
    invoke WriteFile, hOutFile, pBuffer, dwSize, addr dwWrite, NULL
    invoke CloseHandle, hOutFile
    
    invoke HeapFree, hHeap, 0, pBuffer
quit:
    ret
main endp

start:
    call main
    invoke ExitProcess, 0
end start
    
    
	
	
