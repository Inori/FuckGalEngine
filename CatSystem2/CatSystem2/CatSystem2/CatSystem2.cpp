#include "CatSystem2.h"
#include <string.h>
#include "zlib.h"
#include <cstdlib>

void* AllocAlign(uint nSize, uint nAlign)
{
	uint nMod = nSize % nAlign;
	uint nAllocSize = 0;
	if (nMod != 0)
	{
		nAllocSize = nSize - nMod + nAlign;
	}
	else
	{
		nAllocSize = nSize;
	}

	void* pMem = malloc(nAllocSize);
	memset(pMem, 0, nAllocSize);

	return pMem;
}

void Free(void* pMem)
{
	free(pMem);
}

CatSystem2::CatSystem2():
	m_pInFile(NULL),
	m_pOutFile(NULL),
	m_pBackupFileNames(NULL)
{
	memset(&m_oHeader, 0, sizeof(m_oHeader));
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
	if (m_pInFile)
	{
		fclose(m_pInFile);
		m_pInFile = NULL;
	}
	if (m_pOutFile)
	{
		fclose(m_pOutFile);
		m_pOutFile = NULL;
	}

	if (m_pBackupFileNames)
	{
		delete[] m_pBackupFileNames;
		m_pBackupFileNames = NULL;
	}
}

#define CS2_DEFAULT_KEYNAME "__key__.dat"

bool CatSystem2::OpenInput(char* pszName)
{
	if (!pszName)
	{
		return false;
	}

	m_pInFile = fopen(pszName, "rb");
	if (!m_pInFile)
	{
		return false;
	}

	fread(&m_oHeader, sizeof(m_oHeader), 1, m_pInFile);

	if (m_oHeader.Magic != CS2_INT_HEADER_MAGIC ||
		m_oHeader.dwFileNum == 0 ||
		_stricmp(m_oHeader.KeyName, CS2_DEFAULT_KEYNAME))
	{
		fclose(m_pInFile);
		m_pInFile = NULL;
		return false;
	}

	return true;
}

bool CatSystem2::OpenOutput(char* pszName)
{
	if (!pszName)
	{
		return false;
	}

	m_pOutFile = fopen(pszName, "wb");
	if (!m_pOutFile)
	{
		return false;
	}

	fwrite(&m_oHeader, sizeof(m_oHeader), 1, m_pOutFile);

	return true;
}

CatSystem2::~CatSystem2()
{
}

bool CatSystem2::Open(char* pszInputName, char* pszOutputName)
{
	if (!pszInputName)
	{
		return false;
	}

	if (!OpenInput(pszInputName))
	{
		return false;
	}

	if (pszOutputName)
	{
		return OpenOutput(pszOutputName);
	}

	return true;
}

void CatSystem2::Process(bool bPack)
{
	CS2IntEntry *pEntries = NULL;
	uint nFileCount = m_oHeader.dwFileNum;
	

	pEntries = new CS2IntEntry[nFileCount];
	fread(pEntries, sizeof(CS2IntEntry) * nFileCount, 1, m_pInFile);

	BackupFileNames(pEntries);

	if (!DecryptEntries(pEntries))
	{
		delete[] pEntries;
		return;
	}

	if (!bPack)
	{
		ExtractFiles(pEntries);
	}
	else
	{
		InsertFiles(pEntries);
		EncryptEntries(pEntries);
		WriteEntries(pEntries);
	}

	delete[] pEntries;
}

void CatSystem2::Close()
{
	Unit();
}

bool CatSystem2::DecryptEntries(CS2IntEntry* pEntries)
{
	uint nFileCount = m_oHeader.dwFileNum;
	uint nKey = m_oHeader.dwKey;
	int nFileNameHash = 0;

	SetSeed(nKey);
	nKey = m_oMTwist.GetRandom();
	m_oBlowFish.Initialize((byte*)&nKey, sizeof(nKey));

	nFileNameHash = HashString(GetKeyCode());
	CS2IntEntry* pIntEntry = pEntries;
	for (size_t i = 1; nFileCount; ++i, --nFileCount)
	{
		DecryptFileName(pIntEntry->FileName, nFileNameHash + i);
		pIntEntry->dwOffset += i;
		m_oBlowFish.Decode((byte*)&pIntEntry->dwOffset, (byte*)&pIntEntry->dwOffset, 8);
		++pIntEntry;
	}

	return true;
}

bool CatSystem2::EncryptEntries(CS2IntEntry* pEntries)
{
	uint nFileCount = m_oHeader.dwFileNum;
	uint nKey = m_oHeader.dwKey;

	SetSeed(nKey);
	nKey = m_oMTwist.GetRandom();
	m_oBlowFish.Initialize((byte*)&nKey, sizeof(nKey));

	CS2IntEntry* pIntEntry = pEntries;
	for (size_t i = 1; nFileCount; ++i, --nFileCount)
	{
		RecoverFileName(pIntEntry->FileName, i);
		m_oBlowFish.Encode((byte*)&pIntEntry->dwOffset, (byte*)&pIntEntry->dwOffset, 8);
		pIntEntry->dwOffset -= i;
		++pIntEntry;
	}
	return true;
}

void CatSystem2::ExtractFiles(CS2IntEntry* pEntries)
{
	if (!pEntries)
	{
		return;
	}
	uint nFileCount = m_oHeader.dwFileNum;
	for (uint i = 0; i != nFileCount; ++i)
	{
		ExtractOneFile(&pEntries[i]);
	}
}

void CatSystem2::ExtractOneFile(CS2IntEntry* pEntry)
{
	if (!pEntry)
	{
		return;
	}
	byte*   pBuffer;
	uint	nBufferSize;


	nBufferSize = pEntry->dwSize;
	uint nDecodeSize = m_oBlowFish.GetOutputLength(nBufferSize);
	pBuffer = new byte[nDecodeSize];
	memset(pBuffer, 0, nDecodeSize);

	fseek(m_pInFile, pEntry->dwOffset, SEEK_SET);
	fread(pBuffer, nDecodeSize, 1, m_pInFile);

	m_oBlowFish.Decode(pBuffer, pBuffer, nDecodeSize);

	if (((PCS2SceneHeader)pBuffer)->Magic == CS2_SCENE_MAGIC)
	{
		byte* pUncomp;
		ulong  nOutSize;
		PCS2SceneHeader pSceneHeader = (PCS2SceneHeader)pBuffer;

		nOutSize = pSceneHeader->UncompressedSize;
		pUncomp = new byte[nOutSize];

		if (int a = uncompress(pUncomp, &nOutSize, pSceneHeader->CompressedData, pSceneHeader->CompressedSize) != Z_OK)
		{
			delete[] pUncomp;
			goto RETURN_PROC;
		}

		WriteNewFile(pEntry->FileName, pUncomp, nOutSize);

		delete[] pUncomp;
	}
	else if (((PCS2FESHeader)pBuffer)->Magic == CS2_FES_MAGIC)
	{
		byte* pUncomp;
		ulong  nOutSize;
		PCS2FESHeader pFesHeader = (PCS2FESHeader)pBuffer;

		nOutSize = pFesHeader->UncompressedSize;
		pUncomp = new byte[nOutSize];

		if (uncompress(pUncomp, &nOutSize, pFesHeader->CompressedData, pFesHeader->CompressedSize) != Z_OK)
		{
			delete[] pUncomp;
			goto RETURN_PROC;
		}

		WriteNewFile(pEntry->FileName, pUncomp, nOutSize);

		delete[] pUncomp;
	}

RETURN_PROC:
	delete[] pBuffer;
}

void CatSystem2::InsertFiles(CS2IntEntry* pEntries)
{
	if (!pEntries)
	{
		return;
	}

	uint nFileOffset = sizeof(CS2IntHeader) + m_oHeader.dwFileNum * sizeof(CS2IntEntry);
	fseek(m_pOutFile, nFileOffset, SEEK_SET);

	uint nFileCount = m_oHeader.dwFileNum;
	for (uint i = 0; i != nFileCount; ++i)
	{
		InsertOneFile(&pEntries[i]);
	}
}

void CatSystem2::InsertOneFile(CS2IntEntry* pEntry)
{
	if (!pEntry)
	{
		return;
	}

	byte* pBuffer = NULL;
	uint nSize = 0;
	if (!ReadNewFile(pEntry->FileName, &pBuffer, &nSize))
	{
		printf("can't read file: %s\n", pEntry->FileName);
		return;
	}

	ulong nUncompSize = nSize;
	ulong nCompSize = compressBound(nUncompSize);
	byte* pComp = new byte[nCompSize];

	if (compress(pComp, &nCompSize, pBuffer, nUncompSize) != Z_OK)
	{
		printf("compress failed.");
		goto RETURN_PROC;
	}

	pEntry->dwOffset = ftell(m_pOutFile);
	pEntry->dwSize = nCompSize;

	if (strstr(pEntry->FileName, ".sc"))
	{
		CS2SceneHeader oHeader = { 0 };
		oHeader.Magic = CS2_SCENE_MAGIC;
		oHeader.CompressedSize = nCompSize;
		oHeader.UncompressedSize = nUncompSize;
		fwrite(&oHeader, 0x10, 1, m_pOutFile);
	}
	else if (strstr(pEntry->FileName, ".fes"))
	{
		CS2FESHeader oHeader = { 0 };
		oHeader.Magic = CS2_FES_MAGIC;
		oHeader.CompressedSize = nCompSize;
		oHeader.UncompressedSize = nUncompSize;
		fwrite(&oHeader, 0x10, 1, m_pOutFile);
	}

	fwrite(pComp, nCompSize, 1, m_pOutFile);

RETURN_PROC:
	delete[] pComp;
	if (pBuffer)
	{
		delete[] pBuffer;
	}
}

void CatSystem2::WriteNewFile(const char* pszName, byte* pBuffer, uint nSize)
{
	if (!pszName || !pBuffer || !nSize)
	{
		return;
	}

	FILE* pFile = fopen(pszName, "wb");
	if (!pFile)
	{
		return;
	}
	fwrite(pBuffer, nSize, 1, pFile);
	fclose(pFile);
}

bool CatSystem2::ReadNewFile(const char* pszName, byte** ppBuffer, uint* pSize)
{
	if (!pszName || !ppBuffer || !pSize)
	{
		return false;
	}

	FILE* pFile = fopen(pszName, "rb");
	if (!pFile)
	{
		return false;
	}

	uint nFileSize = 0;
	fseek(pFile, 0, SEEK_END);
	nFileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	byte* pBuffer = new byte[nFileSize];
	fread(pBuffer, nFileSize, 1, pFile);
	fclose(pFile);

	*ppBuffer = pBuffer;
	*pSize = nFileSize;
	return true;
}

void CatSystem2::WriteEntries(CS2IntEntry* pEntries)
{
	if (!pEntries)
	{
		return;
	}

	uint nEntryOffset = sizeof(CS2FESHeader);
	fseek(m_pOutFile, nEntryOffset, SEEK_SET);
	fwrite(pEntries, m_oHeader.dwFileNum * sizeof(CS2IntEntry), 1, m_pOutFile);
}

void CatSystem2::BackupFileNames(CS2IntEntry* pEntries)
{
	if (!m_pBackupFileNames)
	{
		uint nNamesSize = m_oHeader.dwFileNum * CS2_FILENAME_LEN;
		m_pBackupFileNames = new char[nNamesSize];
		memset(m_pBackupFileNames, 0, nNamesSize);
	}

	uint nFileCount = m_oHeader.dwFileNum;
	CS2IntEntry* pEntry = pEntries;
	for (uint i = 0; i != nFileCount; ++i)
	{
		memcpy(m_pBackupFileNames + CS2_FILENAME_LEN * i, pEntry->FileName, CS2_FILENAME_LEN);
		pEntry++;
	}
}

void CatSystem2::RecoverFileName(char* pOutName, uint nIndex)
{
	if (!pOutName || nIndex >= m_oHeader.dwFileNum)
	{
		return;
	}
	if (!m_pBackupFileNames)
	{
		return;
	}
	memcpy(pOutName, m_pBackupFileNames + CS2_FILENAME_LEN * nIndex, CS2_FILENAME_LEN);
}

int CatSystem2::HashString(char* pszName)
{
	int ch, nHash;

	nHash = -1;
	ch = *pszName++;
	if (ch == 0)
		return nHash;

	do
	{
		nHash ^= ch << 24;
		for (size_t i = 8; i; --i)
		{
			if (nHash < 0)
				nHash = (nHash + nHash) ^ 0x4C11DB7;
			else
				nHash += nHash;
		}

		nHash = ~nHash;
		ch = *pszName++;
	} while (ch);

	return nHash;
}

#define IN_RANGE(s, c, e) ((s <= c) && (c <= e))

void CatSystem2::DecryptFileName(char* pszFileName, uint nFileNameHash)
{
	uint ch, nIndex;

	ch = *pszFileName;
	if (ch == 0)
		return;

	SetSeed(nFileNameHash);
	nIndex = m_oMTwist.GetRandom();
	nIndex = ((nIndex >> 16) + nIndex + (nIndex >> 24) + (nIndex >> 8)) & 0xFF;
	for (; ch; ++nIndex, ch = *++pszFileName)
	{
		if (!IN_RANGE('A', ch & 0xDF, 'Z'))
			continue;

		for (size_t j = nIndex, i = countof(m_pCharTableReverse); i; --i)
		{
			if ((uint)m_pCharTableReverse[j++ % countof(m_pCharTableReverse)] == ch)
			{
				*pszFileName = m_pCharTable[countof(m_pCharTableReverse) - i];
				break;
			}
		}
	}
}

void CatSystem2::SetSeed(uint nSeed)
{
	m_oMTwist.SetSeed(nSeed);
}

char* CatSystem2::GetKeyCode()
{
	//return "FW-6JD55162";  //GRISAIA
	return "FW-L3EY8BDY";  //ISLAND
}

