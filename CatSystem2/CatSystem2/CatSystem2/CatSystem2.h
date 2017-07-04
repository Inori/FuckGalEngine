#pragma once

#include <cstdio>

#include "blowfish.h"
#include "MTwister.h"
#include "CS2Structs.h"
#include "types.h"




class CatSystem2
{
public:
	CatSystem2();
	~CatSystem2();

	bool Open(char* pszInputName, char* pszOutputName = NULL);

	void Process(bool bPack);

	void Close();

private:
	void	Init();
	void	Unit();

	bool	OpenInput(char* pszName);
	bool	OpenOutput(char* pszName);

	bool	DecryptEntries(CS2IntEntry* pEntries);
	bool	EncryptEntries(CS2IntEntry* pEntries);

	void	ExtractFiles(CS2IntEntry* pEntries);
	void	ExtractOneFile(CS2IntEntry* pEntry);
	void	InsertFiles(CS2IntEntry* pEntries);
	void	InsertOneFile(CS2IntEntry* pEntry);

	void	WriteNewFile(const char* pszName, byte* pBuffer, uint nSize);
	bool ReadNewFile(const char* pszName, byte** ppBuffer, uint* pSize);
	void	WriteEntries(CS2IntEntry* pEntries);

	void	BackupFileNames(CS2IntEntry* pEntries);
	void	RecoverFileName(char* pOutName, uint nIndex);

	int		HashString(char* pszName);
	void	DecryptFileName(char* pszFileName, uint nFileNameHash);

	void	SetSeed(uint nSeed);
	char*	GetKeyCode();
private:
	FILE*	m_pInFile;
	FILE*	m_pOutFile;
	CS2IntHeader m_oHeader;
	CMTwister	m_oMTwist;
	Blowfish	m_oBlowFish;
	char	m_pCharTable[26 * 2];
	char	m_pCharTableReverse[26 * 2];
	char*	m_pBackupFileNames;
};