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

	bool Open(char* pszFileName);

	void Process(bool bPack);

	void Close();

private:
	void	Init();
	void	Unit();

	bool	InitIndex(int nFileNum);
	int		HashString(char* pszName);
	void	DecryptFileName(char* pszFileName, uint nFileNameHash);
	void	SetSeed(uint nSeed);
	char*	GetKeyCode();
private:
	FILE*	m_pFile;
	uint	m_nKey;
	CMTwister	m_oMTwist;
	Blowfish	m_oBlowFish;
	char	m_pCharTable[26 * 2];
	char	m_pCharTableReverse[26 * 2];
};