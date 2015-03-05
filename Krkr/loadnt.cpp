#pragma pack(1)
typedef struct StartConfig
{
	BYTE fill;
	DWORD unknow;	//0x0
	DWORD pagecode;	//0x3A4
	DWORD pagecode2;	//0x411
	DWORD unk2;		//0xFFFFFDE4
	DWORD unk3;		//0x64
}StartConfig;
#pragma pack()
DWORD threadId;
HANDLE hWtEvent;
DWORD WINAPI LoadDllThread(VOID* param)
{
	BYTE b[] = {0xE9,0x90};
	LoadLibraryA("ntleah.dll");

	Sleep(100);


	WaitForSingleObject(hWtEvent,INFINITE);
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS,FALSE,threadId);
	SuspendThread(hThread);


	
	DWORD op;
	VirtualProtect((LPVOID)0x00401000,2,PAGE_EXECUTE_READWRITE,&op);
	WriteProcessMemory(GetCurrentProcess(),(LPVOID)0x00401000,&b,sizeof(b),NULL);
	FlushInstructionCache(GetCurrentProcess(),(LPVOID)0x401000,2);

	ResumeThread(hThread);
	return 0;
}

void ConvertJIS()
{
	HANDLE hMap;
	
	PVOID pBuf;
	BYTE* pBuffer;
	wchar_t ntlea_path[MAX_PATH];

	threadId = GetCurrentThreadId();

	hWtEvent = CreateEventA(NULL,FALSE,FALSE,"RcpEvent000");

	hMap = CreateFileMappingA(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,4,"RcpFileMap000");

	pBuf = MapViewOfFile(hMap,FILE_MAP_ALL_ACCESS,0,0,0x4);

	GetCurrentDirectory(sizeof(ntlea_path),ntlea_path);

	wcscat(ntlea_path,L"\\ntleah.dll");


	pBuffer = (BYTE *)VirtualAlloc(NULL,0x1000,MEM_COMMIT,PAGE_READWRITE);

	wcscpy((WCHAR*)pBuffer,ntlea_path);

	DWORD offsetA = (wcslen(ntlea_path) + 1 ) * sizeof(wchar_t);
	StartConfig cfg;

	cfg.pagecode = 0x3A4;
	cfg.pagecode2 = 0x411;
	cfg.unknow = 0x0;
	cfg.unk2 = 0xFFFFFDE4;
	cfg.unk3 = 0x64;
	cfg.fill = 0;

	memcpy(&pBuffer[offsetA],&cfg,sizeof(cfg));

	*(DWORD*)pBuf = (DWORD)pBuffer;

	BYTE b[] = {0xEB,0xFE};
	DWORD op;
	VirtualProtect((LPVOID)0x00401000,2,PAGE_EXECUTE_READWRITE,&op);
	WriteProcessMemory(GetCurrentProcess(),(LPVOID)0x00401000,&b,sizeof(b),NULL);
	FlushInstructionCache(GetCurrentProcess(),(LPVOID)0x401000,2);
	CreateThread(NULL,NULL,LoadDllThread,NULL,NULL,NULL);
}