// FileStream.h: interface for the CFileStream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESTREAM_H__F652F356_495C_4C6D_A3B9_C59BCE577810__INCLUDED_)
#define AFX_FILESTREAM_H__F652F356_495C_4C6D_A3B9_C59BCE577810__INCLUDED_

#include <vector>
using namespace std;
typedef struct file_header_s
{
	char Sign[32];      //前置标记
	DWORD strCount;
	DWORD strData;
	DWORD treeList;
	DWORD treeDataSize;
	DWORD treeCompSize;
	DWORD treeData;
}file_header_s;
typedef struct stringtable_t
{
	DWORD size;			//字符串的大小
	char* data;			//字符串的数据地址
}stringtable_s;
typedef struct memory_tree_t
{
	BOOL bDirectory;    //是否是目录
	DWORD dwHashValue;  //文件名的哈希值,用于快速寻址
	DWORD nStringIdx;   //字符串索引值,用于定位String位置(文件名)
	DWORD dataSize;     //数据文件大小
	vector <memory_tree_t*> dataList;
	PBYTE dataOffset;   //指向文件数据
}memory_tree_s;
typedef struct disk_tree_s
{
	BOOL bDirectory;    //是否是目录
	DWORD dwHashValue;  //文件名的哈希值,用于快速寻址
	DWORD fileCount;	//文件计数
	DWORD nStringIdx;   //字符串索引值,用于定位String位置(文件名)
	DWORD dataSize;     //数据文件大小
	DWORD dataOffset;   //指向文件数据
}disk_tree_t;
typedef struct save_data_s
{
	DWORD strSize;
	DWORD strCount;
	DWORD treeSize;
	DWORD treeDataSize;
	DWORD treeCompSize;
	char* strData;
	disk_tree_t* treeList;
	void* treeData;
}save_data_t;
enum FileStreamType
{
	FileOpen,
	FileCreate
};

#define SIGN_STRING "萌空汉化组"
class CFileStream  
{
private:
	
	file_header_s fileHeader;

	memory_tree_s memoryTreeRoot;
	vector <stringtable_s*> memoryString;

	PBYTE memoryData;
	HANDLE fileHandle;
	FileStreamType fileType;

	DWORD fileDataSize;

	bool m_strcmp(int idx,char* string);
	bool RecursionInsertFile(char* treepath,memory_tree_s* fileTree,memory_tree_s* DirTree=NULL);
	memory_tree_s* RecursionFileExists(char* TreePath,memory_tree_s* DirTree);
	
	void RecursionTreeList(memory_tree_s* root,save_data_t* save);
	void RecursionTreeList(disk_tree_s* tree,void* treedata,PDWORD offset,DWORD startaddr=0,memory_tree_s* List=NULL);

	BOOL RecursionDirectory(char* lpszDir ,int start=0);
public:
	CFileStream();
	virtual ~CFileStream();
	bool Open(char* pszFilePath);
	bool Create(char* pszFilePath);
	DWORD GetHashValue(char* str);
	memory_tree_s* FileIsExists(char* TreePath);
	char* GetFolderName(char* TreePath);
	char* GetFileName(char* TreePath);
	memory_tree_s * GetFile(char* treepath);
	char* GetName(int idx);
	int AllocString(char* str);
	bool SaveToStream(BOOL bCompress=TRUE);
	bool LoadFromStream();
	bool AddFile(char* pszFilePath,char* treepath);
};

#endif // !defined(AFX_FILESTREAM_H__F652F356_495C_4C6D_A3B9_C59BCE577810__INCLUDED_)
