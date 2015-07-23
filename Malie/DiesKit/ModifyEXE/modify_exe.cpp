#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>
#include <sys/stat.h>
#include "include\PeLib.h"
using std::string;
using std::vector;
using std::ofstream;
using std::ifstream;


unsigned int get_filesize(const char *filename)
{
	struct stat f_stat;
	if (stat(filename, &f_stat) == -1)
	{
		return -1;
	}
	return f_stat.st_size;
}

PeLib::ResourceLeaf* findExecNode(PeLib::ResourceElement* elem)
{
	bool isLeaf = elem->isLeaf();

	if (!isLeaf)
	{
		PeLib::ResourceNode* node = static_cast<PeLib::ResourceNode*>(elem);

		unsigned int uiNamedEntries = node->getNumberOfNamedEntries();
		unsigned int uiIdEntries = node->getNumberOfIdEntries();

		for (unsigned int i = 0; i<uiNamedEntries; i++)
		{
			string childname = node->getChildName(i);
			if (childname == "EXEC")
			{
				PeLib::ResourceNode* exec_node = static_cast<PeLib::ResourceNode*>(node->getChild(i));
				return static_cast<PeLib::ResourceLeaf*>(static_cast<PeLib::ResourceNode*>(exec_node->getChild(0))->getChild(0));
			}
			findExecNode(node->getChild(i));
		}
	}
}


int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "Usage: ModifyEXE exe_name exec_name" << std::endl;
		return 1;
	}

	string filename(argv[1]);
	string exec_name(argv[2]);

	ifstream exec(exec_name, std::ios::in | std::ios::binary);
	if (!exec.is_open()) return 1;

	unsigned int exec_size = get_filesize(exec_name.c_str());
	unsigned char *exec_data = new unsigned char[exec_size];
	exec.read((char*)exec_data, exec_size);
	exec.close();


	PeLib::PeFile* f = PeLib::openPeFile(filename);
	unsigned int exe_size = get_filesize(filename.c_str());

	if (!f)
	{
		std::cout << "Invalid PE file" << std::endl;
		return 2;
	}

	try
	{
		f->readMzHeader();
		f->readPeHeader();
		f->readResourceDirectory();
	}
	catch (...)
	{
		std::cout << "An error occured while reading the file. Maybe the file is not a valid PE file." << std::endl;
		delete f;
		return 3;
	}

	try
	{
		if (PeLib::getFileType(filename) == PeLib::PEFILE32)
		{
			PeLib::ResourceElement* root = f->resDir().getRoot();
			unsigned int resRva = root->getElementRva();
			unsigned int resRealOffset = static_cast<PeLib::PeFileT<32>*>(f)->peHeader().rvaToOffset(resRva);
			unsigned int execRva = exe_size - resRealOffset + resRva;

			PeLib::ResourceLeaf* exec_node = findExecNode(root);
			exec_node->setOffsetToData(execRva);
			exec_node->setSize(exec_size);
			exec_node->makeValid();

			f->resDir().makeValid();
			f->resDir().write(filename, resRealOffset, resRva);
			delete f;

			ofstream exe(filename, std::ios::app | std::ios::binary);
			if (!exe.is_open()) return 1;

			exe.seekp(0, std::ios::end);
			exe.write((char*)exec_data, exec_size);
			exe.close();
			//f->resDir().makeValid();
			//f->resDir().write("malie_cracked.exe", 0x10A000, 0x3F4000);
		}
		else if (PeLib::getFileType(filename) == PeLib::PEFILE64)
		{
			std::cout << "Don't support 64 bit PE file" << std::endl;
			delete f;
			return 2;
		}
		else
		{
			std::cout << "Invalid PE file" << std::endl;
			delete f;
			return 2;
		}

	}
	catch (...)
	{
		std::cout << "An error occured while reading the resource directory. Maybe the directory is invalid." << std::endl;
		delete f;
		return 4;
	}

	delete[]exec_data;

}
