#include "CatSystem2.h"
#include <string.h>

CatSystem2::CatSystem2():
	m_nKey(0),
	m_pFile(NULL)
{
	Init();
}

void CatSystem2::Init()
{
	char up, low;
	char *pUpStart, *pUpEnd, *pLowStart, *pLowEnd;

	pUpStart = m_pCharTable;
	pUpEnd = m_pCharTableReverse + countof(m_pCharTableReverse) - 1;
	pLowStart = pUpStart + 26;
	pLowEnd = pUpEnd - 26;

	up = 'A';
	low = 'a';
	for (size_t i = 26; i; --i)
	{
		*pUpStart++ = up;
		*pUpEnd-- = up;
		*pLowStart++ = low;
		*pLowEnd-- = low;

		++up;
		++low;
	}
}

void CatSystem2::Unit()
{
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
}

CatSystem2::~CatSystem2()
{
	Unit();
}

bool CatSystem2::Open(char* pszFileName)
{
	if (!pszFileName)
	{
		return false;
	}

	m_pFile = fopen(pszFileName, "rb+");
	if (!m_pFile)
	{
		return false;
	}

	CS2IntHeader oHeader = {0};

	fread(&oHeader, sizeof(oHeader), 1, m_pFile);

	if (oHeader.Magic != CS2_INT_HEADER_MAGIC ||
		oHeader.dwFileNum == 0 ||
		_stricmp(oHeader.KeyName, "__key__.dat"))
	{
		return false;
	}

	m_nKey = oHeader.dwKey;

	return InitIndex(--oHeader.dwFileNum);
}

void CatSystem2::Process(bool bPack)
{

}

void CatSystem2::Close()
{

}

bool CatSystem2::InitIndex(int nFileNum)
{
	Int32 FileNameHash;
	CS2IntEntry *pEntry, *pIntEntry;
	MyCS2IntEntry *pMyEntry;

	pEntry = (CS2IntEntry *)Alloc(FileNum * sizeof(*pEntry));
	if (pEntry == NULL)
		return False;

	if (!m_file.Read(pEntry, FileNum * sizeof(*pEntry)))
	{
		Free(pEntry);
		return False;
	}

	m_Index.pEntry = (MY_FILE_ENTRY_BASE *)(MyCS2IntEntry *)Alloc(FileNum * sizeof(*m_Index.pEntry));
	if (m_Index.pEntry == NULL)
	{
		Free(pEntry);
		return False;
	}

	m_Index.cbEntrySize = sizeof(*m_Index.pEntry);
	m_Index.FileCount.QuadPart = FileNum;
	SetSeed(m_Key);
	m_Key = m_MTwist.GetRandom();
	m_BlowFish.Initialize((PByte)&m_Key, sizeof(m_Key));

	FileNameHash = HashString(GetKeyCode());
	pIntEntry = pEntry;
	pMyEntry = m_Index.pEntry;
	for (SizeT i = 1; FileNum; ++i, --FileNum)
	{
		DecryptFileName(pIntEntry->FileName, FileNameHash + i);
		pIntEntry->dwOffset += i;
		m_BlowFish.Decode((PByte)&pIntEntry->dwOffset, (PByte)&pIntEntry->dwOffset, 8);
		pMyEntry->Offset.QuadPart = pIntEntry->dwOffset;
		pMyEntry->Size.QuadPart = pIntEntry->dwSize;
		MultiByteToWideChar(CP_ACP, 0, pIntEntry->FileName, -1, pMyEntry->FileName, countof(pMyEntry->FileName));
		++pIntEntry;
		++pMyEntry;
	}

	Free(pEntry);

	return True;
}

int CatSystem2::HashString(char* pszName)
{

}

void CatSystem2::DecryptFileName(char* pszFileName, uint nFileNameHash)
{

}

void CatSystem2::SetSeed(uint nSeed)
{

}

char* CatSystem2::GetKeyCode()
{

}

