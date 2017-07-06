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

	fwrite(&m_oHeader, sizeof(CS2IntHeader), 1, m_pOutFile);

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
		if (!strcmp(pIntEntry->FileName, CS2_DEFAULT_KEYNAME))
		{
			++pIntEntry;
			continue;
		}
	
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
		if (!strcmp(pIntEntry->FileName, CS2_DEFAULT_KEYNAME))
		{
			++pIntEntry;
			continue;
		}
		RecoverFileName(pIntEntry->FileName, i - 1);
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
	CS2IntEntry* pEntry = NULL;
	for (uint i = 0; i != nFileCount; ++i)
	{
		pEntry = &pEntries[i];
		if (!strcmp(pEntry->FileName, CS2_DEFAULT_KEYNAME))
		{
			continue;
		}
		ExtractOneFile(pEntry);
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
	pBuffer = new byte[nBufferSize];

	fseek(m_pInFile, pEntry->dwOffset, SEEK_SET);
	fread(pBuffer, nBufferSize, 1, m_pInFile);

	uint nDecodeSize = (nBufferSize / 8) * 8;
	m_oBlowFish.Decode(pBuffer, pBuffer, nDecodeSize);

	if (((PCS2SceneHeader)pBuffer)->Magic == CS2_SCENE_MAGIC)
	{
		byte* pUncomp;
		ulong  nOutSize;
		PCS2SceneHeader pSceneHeader = (PCS2SceneHeader)pBuffer;

		nOutSize = pSceneHeader->UncompressedSize;
		pUncomp = new byte[nOutSize];

		if (uncompress(pUncomp, &nOutSize, pBuffer + sizeof(CS2SceneHeader), pSceneHeader->CompressedSize) != Z_OK)
		{
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

		if (uncompress(pUncomp, &nOutSize, pBuffer + sizeof(CS2FESHeader), pFesHeader->CompressedSize) != Z_OK)
		{
			delete[] pUncomp;
			goto RETURN_PROC;
		}

		WriteNewFile(pEntry->FileName, pUncomp, nOutSize);

		delete[] pUncomp;
	}

	printf("Extract file: %s -> done.\n", pEntry->FileName);

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
	CS2IntEntry* pEntry = NULL;
	for (uint i = 0; i != nFileCount; ++i)
	{
		pEntry = &pEntries[i];
		if (!strcmp(pEntry->FileName, CS2_DEFAULT_KEYNAME))
		{
			continue;
		}
		InsertOneFile(pEntry);
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

	

	byte* pEncBuffer = NULL;
	uint nEncSize = 0;
	uint nHeaderSize = 0;
	if (strstr(pEntry->FileName, ".cst"))
	{
		CS2SceneHeader oHeader = { 0 };
		oHeader.Magic = CS2_SCENE_MAGIC;
		oHeader.CompressedSize = nCompSize;
		oHeader.UncompressedSize = nUncompSize;

		nHeaderSize = sizeof(CS2SceneHeader);
		nEncSize = nHeaderSize + nCompSize;
		pEncBuffer = new byte[nEncSize];
		memcpy(pEncBuffer, &oHeader, sizeof(CS2SceneHeader));
	}
	else if (strstr(pEntry->FileName, ".fes"))
	{
		CS2FESHeader oHeader = { 0 };
		oHeader.Magic = CS2_FES_MAGIC;
		oHeader.CompressedSize = nCompSize;
		oHeader.UncompressedSize = nUncompSize;

		nHeaderSize = sizeof(CS2FESHeader);
		nEncSize = nHeaderSize + nCompSize;
		pEncBuffer = new byte[nEncSize];
		memcpy(pEncBuffer, &oHeader, sizeof(CS2FESHeader));
	}

	memcpy(pEncBuffer + nHeaderSize, pComp, nCompSize);
	uint nEncodeSize = (nEncSize / 8) * 8;
	m_oBlowFish.Encode(pEncBuffer, pEncBuffer, nEncodeSize);


	pEntry->dwOffset = ftell(m_pOutFile);
	pEntry->dwSize = nEncSize;

	fwrite(pEncBuffer, nEncSize, 1, m_pOutFile);
	


	delete[] pEncBuffer;

	printf("Insert file: %s -> done.\n", pEntry->FileName);

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

	uint nEntryOffset = sizeof(CS2IntHeader);
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

void CatSystem2::DecryptFileName(char* pszFileName, uint nSeed)
{
	static unsigned char FWD[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static unsigned char REV[] = "zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA";

	SetSeed(nSeed);
	unsigned long nKey = m_oMTwist.GetRandom();
	unsigned long nShift = (unsigned char)((nKey >> 24) + (nKey >> 16) + (nKey >> 8) + nKey);

	for (unsigned char* p = (unsigned char*)pszFileName; *p; p++) 
	{
		unsigned long nIndex = 0;
		unsigned long index2 = nShift;

		while (REV[index2 % 0x34] != *p) 
		{
			if (REV[(nShift + nIndex + 1) % 0x34] == *p) 
			{
				nIndex += 1;
				break;
			}

			if (REV[(nShift + nIndex + 2) % 0x34] == *p) 
			{
				nIndex += 2;
				break;
			}

			if (REV[(nShift + nIndex + 3) % 0x34] == *p)
			{
				nIndex += 3;
				break;
			}

			nIndex += 4;
			index2 += 4;

			if (nIndex > 0x34)
			{
				break;
			}
		}

		if (nIndex < 0x34) 
		{
			*p = FWD[nIndex];
		}

		nShift++;
	}
}

void CatSystem2::SetSeed(uint nSeed)
{
	m_oMTwist.SetSeed(nSeed);
}

//´ÓÓÎÏ·ExeÖÐÕÒKey

//void copy_resource(HMODULE         h,
//	const string&   name,
//	const string&   type,
//	unsigned char*& buff,
//	unsigned long&  len)
//{
//	HRSRC r = FindResource(h, name.c_str(), type.c_str());
//	if (!r) {
//		fprintf(stderr, "Failed to find resource %s:%s\n", name.c_str(), type.c_str());
//		exit(-1);
//	}
//
//	HGLOBAL g = LoadResource(h, r);
//	if (!g) {
//		fprintf(stderr, "Failed to load resource %s:%s\n", name.c_str(), type.c_str());
//		exit(-1);
//	}
//
//	len = SizeofResource(h, r);
//	buff = new unsigned char[(len + 7) & ~7];
//
//	void* locked_buff = LockResource(g);
//	if (!locked_buff) {
//		fprintf(stderr, "Failed to lock resource %s:%s\n", name.c_str(), type.c_str());
//		exit(-1);
//	}
//
//	memcpy(buff, locked_buff, len);
//}
//
//string find_vcode2(const string& exe_filename) {
//	HMODULE h = LoadLibraryEx(exe_filename.c_str(), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
//	if (!h) {
//		fprintf(stderr, "Failed to load %s\n", exe_filename.c_str());
//		exit(-1);
//	}
//
//	unsigned long  key_len = 0;
//	unsigned char* key_buff = NULL;
//	copy_resource(h, "KEY", "KEY_CODE", key_buff, key_len);
//
//	for (unsigned long i = 0; i < key_len; i++) {
//		key_buff[i] ^= 0xCD;
//	}
//
//	unsigned long  vcode2_len = 0;
//	unsigned char* vcode2_buff = NULL;
//	copy_resource(h, "DATA", "V_CODE2", vcode2_buff, vcode2_len);
//
//	Blowfish bf;
//	bf.Set_Key(key_buff, key_len);
//	bf.Decrypt(vcode2_buff, (vcode2_len + 7) & ~7);
//
//	string vcode2((char*)vcode2_buff, vcode2_len);
//
//	delete[] vcode2_buff;
//	delete[] key_buff;
//
//	FreeLibrary(h);
//
//	return vcode2;
//}
char* CatSystem2::GetKeyCode()
{
	//return "FW-6JD55162";  //GRISAIA
	return "FW-M2RT6IID";  //ISLAND
}

