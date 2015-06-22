/*
	Moe-Sky(ÃÈ¿Õºº»¯×é) File Stream Sources

	201724 @ Moe-Sky

	2013-01-20
*/
#include "stdafx.h"
#include "NumberFileStream.h"
#include "FileStream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <shlwapi.h>
#include "zlib.h"
#pragma comment(lib,"zlib.lib")
#pragma comment(lib,"shlwapi.lib")
#include <malloc.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
CFileStream::CFileStream()
{
	
}

CFileStream::~CFileStream()
{

}
bool CFileStream::Open(char* pszFilePath)
{
	DWORD fileSize;
	DWORD readSize;
	fileHandle = CreateFile(pszFilePath,GENERIC_READ,NULL,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		
		fileSize = GetFileSize(fileHandle,&fileSize);

		if(fileSize > sizeof(file_header_s))
		{
			if(ReadFile(fileHandle,(LPVOID)&fileHeader,sizeof(fileHeader),&readSize,NULL))
			{
				if( strcmp(fileHeader.Sign,SIGN_STRING)==0 && 
					fileHeader.strCount &&
					fileHeader.strData &&
					fileHeader.treeData &&
					fileHeader.treeList)
				{
					memoryData = new BYTE[fileSize];
					SetFilePointer(fileHandle,0,NULL,FILE_BEGIN);
					if(ReadFile(fileHandle,(LPVOID)memoryData,fileSize,&readSize,NULL))
					{
						fileType = FileOpen;
						return true;
					}
				}
			}
		}
	}
	return false;
}
DWORD CFileStream::GetHashValue(char* str)
{
	DWORD hashValue=0;
	int len = strlen(str);
	for(int i=0;i<len;i++)
	{
		hashValue += ~(((str[i] << 8) | ~str[i]) << 8);
		hashValue += (((str[i] << 8) | str[i]) << 8);
		hashValue *= hashValue;
	}
	return hashValue;
}
bool CFileStream::Create(char* pszFilePath)
{
	fileHandle = CreateFile(pszFilePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		fileType = FileCreate;
		memset(&fileHeader,0,sizeof(fileHeader));

		memoryTreeRoot.bDirectory =  TRUE;
		memoryTreeRoot.dataOffset = NULL;
		memoryTreeRoot.dataSize = NULL;
		memoryTreeRoot.dwHashValue = GetHashValue("/");
		memoryTreeRoot.nStringIdx = 0;
		
		stringtable_s * string = new stringtable_s;
		string->size = strlen("/")+1;
		string->data = new char[strlen("/")+1];
		strcpy(string->data,"/");
		memoryString.push_back(string);

		return true;
	}
	return false;
}
bool CFileStream::m_strcmp(int idx,char* string)
{
	return (strcmp((const char*)memoryString[idx]->data,string)==0);
}
char* CFileStream::GetFolderName(char* TreePath)
{
	static char szFolderName[MAX_PATH];
	int len = strlen(TreePath);
	strcpy(szFolderName,TreePath);
	bool find=false;
	for(int i=0;i<len;i++)
	{
		if(szFolderName[i] == '/')
		{
			szFolderName[i] = 0;
			find = true;
			break;
		}
	}
	if(!find)
		return FALSE;
	return (char*)&szFolderName;
}
char* CFileStream::GetFileName(char* TreePath)
{
	int len = strlen(TreePath);
	for(int i=len;i>-1;i--)
	{
		if(TreePath[i] == '/')
			return &TreePath[i+1];
	}
	return NULL;
}
memory_tree_s* CFileStream::RecursionFileExists(char* TreePath,memory_tree_s* DirTree)
{
	char* pszFolderName = GetFolderName(TreePath);
	if(pszFolderName)
	{
		DWORD hashvalue = GetHashValue(pszFolderName);
		
		int listsize = DirTree->dataList.size();
		if(listsize)
		{
			for(int i=0;i<listsize;i++)
			{
				if(DirTree->dataList[i]->bDirectory && DirTree->dataList[i]->dwHashValue == hashvalue && m_strcmp(DirTree->dataList[i]->nStringIdx,pszFolderName))
				{
					char* nextdir = &TreePath[strlen(pszFolderName)+1];
					return RecursionFileExists(nextdir,DirTree->dataList[i]);
				}
			}
		}
		else
		{
			return NULL;
		}

	}
	else
	{
		int listsize = DirTree->dataList.size();
		if(listsize)
		{
			DWORD hashvalue = GetHashValue(TreePath);
			for(int i=0;i<listsize;i++)
			{
				if(!DirTree->dataList[i]->bDirectory)
				{
					if( DirTree->dataList[i]->dwHashValue == hashvalue &&
						m_strcmp(DirTree->dataList[i]->nStringIdx,TreePath))
						return DirTree->dataList[i];
				}
			}
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}
memory_tree_t * CFileStream::FileIsExists(char* TreePath)
{
	static char szPath[MAX_PATH];
	char* szChars;
	char* pszFolderName;
	if(TreePath && TreePath[0] == '/')
	{
		strcpy(szPath,TreePath);
		szChars = (char*)&szPath;
		if(memoryTreeRoot.dataList.size() == 0)
		{
			return NULL;
		}
		szChars++;
		pszFolderName = GetFolderName(szChars);
		if(pszFolderName)
		{
			int listsize = memoryTreeRoot.dataList.size();
			if(listsize)
			{
				DWORD hashvalue = GetHashValue(pszFolderName);
				for(int i=0;i<listsize;i++)
				{
					if(memoryTreeRoot.dataList[i]->bDirectory &&
						memoryTreeRoot.dataList[i]->dwHashValue == hashvalue &&
						m_strcmp(memoryTreeRoot.dataList[i]->nStringIdx,pszFolderName))
					{
						char* nextdir = &szChars[strlen(pszFolderName)+1];
						return RecursionFileExists(nextdir,memoryTreeRoot.dataList[i]);
					}
				}
			}
			return NULL;
		}
		else
		{
			int listsize = memoryTreeRoot.dataList.size();
			if(listsize)
			{
				DWORD hashvalue = GetHashValue(szChars);
				for(int i=0;i<listsize;i++)
				{

					if(!memoryTreeRoot.dataList[i]->bDirectory &&
						memoryTreeRoot.dataList[i]->dwHashValue == hashvalue &&
						m_strcmp(memoryTreeRoot.dataList[i]->nStringIdx,szChars)
						)
					{
						return memoryTreeRoot.dataList[i];
					}
				}
			}
		}
	}
	return NULL;
}
bool CFileStream::RecursionInsertFile(char* treepath,memory_tree_s* fileTree,memory_tree_s* DirTree)
{
	char TreeRoot[MAX_PATH];
	if(treepath[0] == '/')
	{
		treepath++;
		return RecursionInsertFile(treepath,fileTree,&memoryTreeRoot);
	}
	if(DirTree)
	{
		strcpy(TreeRoot,treepath);
		char * folderName = GetFolderName(TreeRoot);
		if(folderName)
		{
			int listsize = DirTree->dataList.size();
			if(listsize)
			{
				DWORD hashvalue = GetHashValue(folderName);
				for(int i=0;i<listsize;i++)
				{
					if(DirTree->dataList[i]->bDirectory &&
						DirTree->dataList[i]->dwHashValue == hashvalue &&
						m_strcmp(DirTree->dataList[i]->nStringIdx,folderName))
					{
						char* nextdir = &treepath[strlen(folderName)+1];
						return RecursionInsertFile(nextdir,fileTree,DirTree->dataList[i]); 
					}
				}
				memory_tree_s* newdir = new memory_tree_s;
				newdir->bDirectory = TRUE;
				newdir->nStringIdx = AllocString(folderName);
				newdir->dataSize = 0;
				newdir->dataOffset = 0;
				newdir->dwHashValue = GetHashValue(folderName);
				DirTree->dataList.push_back(newdir);
				char* nextdir = &treepath[strlen(folderName)+1];
				return RecursionInsertFile(nextdir,fileTree,newdir);
			}
			else
			{
				memory_tree_s* newdir = new memory_tree_s;
				newdir->bDirectory = TRUE;
				newdir->nStringIdx = AllocString(folderName);
				newdir->dataSize = 0;
				newdir->dataOffset = 0;
				newdir->dwHashValue = GetHashValue(folderName);
				DirTree->dataList.push_back(newdir);
				char* nextdir = &treepath[strlen(folderName)+1];
				return RecursionInsertFile(nextdir,fileTree,newdir);
			}
		}
		else
		{
			DirTree->dataList.push_back(fileTree);
			return true;
		}
	}
	return false;
}
int CFileStream::AllocString(char* str)
{
	char fileName[32];

	strcpy(fileName,str);

	stringtable_s * string = new stringtable_s;

	string->size = strlen(fileName) + 1;
	string->data = new char[string->size];
	strcpy(string->data,fileName);

	memoryString.push_back(string);

	return memoryString.size() - 1;
}
bool CFileStream::AddFile(char* pszFilePath,char* treepath)
{
	HANDLE addFileHandle;
	DWORD fileSize;
	DWORD readSize;
	if(FileIsExists(treepath))
		return false;
	addFileHandle = CreateFile(pszFilePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(addFileHandle != INVALID_HANDLE_VALUE)
	{
		memory_tree_s* fileTree = new memory_tree_s;
		
		fileSize = GetFileSize(addFileHandle,&fileSize);
		fileTree->bDirectory = FALSE;
		fileTree->dataSize = fileSize;
		fileTree->dataOffset = new BYTE[fileSize];
		fileTree->dwHashValue = GetHashValue(GetFileName(treepath));
		fileTree->nStringIdx = AllocString(GetFileName(treepath));
		
		ReadFile(addFileHandle,fileTree->dataOffset,fileSize,&readSize,NULL);
		CloseHandle(addFileHandle);
		if(RecursionInsertFile(treepath,fileTree))
			return true;
	}
	return false;
}
char* CFileStream::GetName(int idx)
{
	return (char*)memoryString[idx]->data;
}
memory_tree_s* CFileStream::GetFile(char* treepath)
{
	return FileIsExists(treepath);
}
void CFileStream::RecursionTreeList(memory_tree_s* root,save_data_t* save)
{
	save->treeList = (disk_tree_s*)realloc((void*)save->treeList,save->treeSize + sizeof(disk_tree_s));
	disk_tree_s* tree = (disk_tree_s*)((DWORD)save->treeList + save->treeSize);

	tree->bDirectory = root->bDirectory;
	tree->dwHashValue = root->dwHashValue;
	tree->nStringIdx = root->nStringIdx;

	save->treeSize += sizeof(disk_tree_s);
	if(root->bDirectory)
	{
		int listsize = root->dataList.size();
		tree->fileCount = listsize;
		tree->dataOffset = save->treeSize;

		for(int i=0;i<listsize;i++)
		{
			RecursionTreeList(root->dataList[i],save);
		}
	}
	else
	{
		tree->fileCount = 0;
		tree->dataSize = root->dataSize;
		tree->dataOffset = save->treeDataSize;

		save->treeData = realloc(save->treeData,save->treeDataSize + root->dataSize);

		memcpy((void*)((DWORD)save->treeData + save->treeDataSize),root->dataOffset,root->dataSize);

		save->treeDataSize += root->dataSize;
	}
}
void CFileStream::RecursionTreeList(disk_tree_s* tree,void* treedata,PDWORD offset,DWORD startaddr,memory_tree_s* List)
{
	*offset += sizeof(disk_tree_s);
	if(tree->nStringIdx==0)
	{
		memoryTreeRoot.bDirectory = tree->bDirectory;
		memoryTreeRoot.dataOffset = NULL;
		memoryTreeRoot.dataSize = NULL;
		memoryTreeRoot.dwHashValue = tree->dwHashValue;
		memoryTreeRoot.nStringIdx = tree->nStringIdx;
		for(DWORD i=0;i<tree->fileCount;i++)
		{
			RecursionTreeList((disk_tree_s*)((DWORD)tree+*offset),treedata,offset,(DWORD)tree,&memoryTreeRoot);
		}
	}
	else
	{
		memory_tree_s* m_list = new memory_tree_s;
		List->dataList.push_back(m_list);
		m_list->bDirectory = tree->bDirectory;
		m_list->dwHashValue = tree->dwHashValue;
		m_list->nStringIdx = tree->nStringIdx;
		if(tree->bDirectory)
		{
			m_list->dataOffset = NULL;
			m_list->dataSize = NULL;
			for(DWORD i=0;i<tree->fileCount;i++)
			{
				RecursionTreeList((disk_tree_s*)(startaddr+*offset),treedata,offset,startaddr,m_list);
			}
		}
		else
		{
			m_list->dataOffset =(PBYTE)( tree->dataOffset + (DWORD)treedata);
			m_list->dataSize = tree->dataSize;
		}
		
	}
	
}
bool CFileStream::SaveToStream(BOOL bCompress)
{
	int stringCount = memoryString.size();
	int dataCount = memoryTreeRoot.dataList.size();

	save_data_t save_data;
	
	DWORD fileSize;
	PBYTE fileData = (PBYTE)malloc(0);
	file_header_s * fileHeader;
	
	save_data.treeSize = 0;
	save_data.treeDataSize = 0;
	save_data.treeList = (disk_tree_s*)malloc(0);
	save_data.treeData = malloc(0);
	save_data.strData = (char*)malloc(0);
	save_data.strCount = stringCount;
	save_data.strSize = 0;
	
	for(int i=0;i<stringCount;i++)
	{
		save_data.strData = (char*)realloc(save_data.strData,save_data.strSize+memoryString[i]->size);
		strcpy(save_data.strData+save_data.strSize,memoryString[i]->data);
		save_data.strSize += memoryString[i]->size;
	}

	RecursionTreeList(&memoryTreeRoot,&save_data);

	fileData = (PBYTE)realloc(fileData,sizeof(file_header_s));
	fileSize = sizeof(file_header_s);

	fileHeader = (file_header_s*)fileData;
	memset(fileHeader->Sign,0,sizeof(fileHeader->Sign));
	strcpy(fileHeader->Sign,SIGN_STRING);

	fileHeader->strCount = save_data.strCount;
	fileHeader->strData = fileSize;
	
	fileData = (PBYTE)realloc(fileData,fileSize+save_data.strSize);
	memcpy(&fileData[fileSize],save_data.strData,save_data.strSize);
	fileSize += save_data.strSize;

	fileHeader = (file_header_s*)fileData;
	fileHeader->treeList = fileSize;
	fileData = (PBYTE)realloc(fileData,fileSize+save_data.treeSize);
	memcpy(&fileData[fileSize],save_data.treeList,save_data.treeSize);
	fileSize += save_data.treeSize;
	PBYTE compBuffer;
	if(bCompress)
	{
		save_data.treeCompSize =  compressBound(save_data.treeDataSize);
		compBuffer = new BYTE[save_data.treeCompSize];
		if(compress(compBuffer,&save_data.treeCompSize,(PBYTE)save_data.treeData,save_data.treeDataSize)==Z_OK)
		{
			fileData = (PBYTE)realloc(fileData,fileSize+save_data.treeCompSize);
			fileHeader = (file_header_s*)fileData;
			fileHeader->treeData = fileSize;
			fileHeader->treeCompSize = save_data.treeCompSize;
			fileHeader->treeDataSize = save_data.treeDataSize;

			memcpy(&fileData[fileSize],compBuffer,save_data.treeCompSize);
		}
		else
		{
			MessageBox(NULL,"Ñ¹ËõÊ§°Ü","",MB_OK);
			abort();
		}
		fileSize += save_data.treeCompSize;
	}
	else
	{
		fileData = (PBYTE)realloc(fileData,fileSize+save_data.treeDataSize);
		fileHeader = (file_header_s*)fileData;
		fileHeader->treeData = fileSize;
		fileHeader->treeCompSize = NULL;
		fileHeader->treeDataSize = save_data.treeDataSize;
		
		memcpy(&fileData[fileSize],save_data.treeData,save_data.treeDataSize);
		fileSize += save_data.treeDataSize;
	}
	
	
	SetFilePointer(fileHandle,0,NULL,FILE_BEGIN);

	DWORD nWriteSize;
	WriteFile(fileHandle,fileData,fileSize,&nWriteSize,NULL);
	free(fileData);
	free(save_data.treeList);
	free(save_data.treeData);
	free(save_data.strData);
	if(bCompress) delete compBuffer;
	return true;
}

BOOL CFileStream::RecursionDirectory(char* lpszDir ,int start)
{
	int i=0;
	if( ::PathIsDirectory( lpszDir ) ) return TRUE;

	char szPreDir[MAX_PATH];
	int len = strlen(lpszDir);
	szPreDir[0] = 0;
	for(i=start;i<len;i++)
	{
		if(lpszDir[i] == '/')
		{
			memcpy(szPreDir,lpszDir,i);
			szPreDir[i] = 0;
			break;
		}
	}
	i++;
	if(szPreDir[0] == 0)
	{
		strcpy(szPreDir,lpszDir);
	}
	if( !::PathIsDirectory( szPreDir ) )
	{
		CreateDirectory( szPreDir,NULL );
	}
	if(lpszDir[i] != 0)
	{
		RecursionDirectory(lpszDir,i);
	}

	return TRUE;
}
bool CFileStream::LoadFromStream()
{
	file_header_s* fileHeader = (file_header_s*)memoryData;
	fileHeader->strData += (DWORD)memoryData;
	fileHeader->treeData += (DWORD)memoryData;
	fileHeader->treeList += (DWORD)memoryData;

	if(strcmp(fileHeader->Sign,SIGN_STRING)==0)
	{
		DWORD nCount = 0;
		DWORD readOffset = 0;
		for(nCount = 0;nCount < fileHeader->strCount;nCount++)
		{
			AllocString((char*)fileHeader->strData + readOffset);
			readOffset += strlen((char*)fileHeader->strData + readOffset) +1;
		}
		DWORD offset=0;
		if(fileHeader->treeCompSize)
		{
			PBYTE pTreeData = new BYTE[fileHeader->treeDataSize];
			if(uncompress(pTreeData,&fileHeader->treeDataSize,(PBYTE)fileHeader->treeData,fileHeader->treeCompSize)==Z_OK)
			{
				RecursionTreeList((disk_tree_s*)fileHeader->treeList,(void*)pTreeData,&offset);
				return true;
			}
		}
		else
		{
				RecursionTreeList((disk_tree_s*)fileHeader->treeList,(void*)fileHeader->treeData,&offset);
				return true;
		}
	}
	return false;
}