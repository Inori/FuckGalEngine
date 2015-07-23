#include <fstream>
#include <string>
using std::string;
using std::ifstream;
using std::ofstream;

int main(int argc, char **argv)
{
	if (argc != 3) return -1;

	string segName(argv[1]);
	string execName(argv[2]);

	//read exec file
	ifstream exec(execName, std::ios::in | std::ios::binary);
	if (!exec.is_open()) return -1;

	exec.seekg(0, std::ios::end);
	unsigned int execSize = exec.tellg();
	exec.seekg(0, std::ios::beg);

	unsigned char *execData = new unsigned char[execSize];
	exec.read((char*)execData, execSize);
	exec.close();

	const unsigned int resRva = 0x003F4000;
	const unsigned int execRvaOffset = 0x00000C40;
	const unsigned int execSizeOffset = execRvaOffset + 4;

	ofstream seg(segName, std::ios::binary | std::ios::out | std::ios::in);
	if (!seg.is_open()) return -1;

	seg.seekp(0, std::ios::end);
	unsigned int segSize = seg.tellp();
	seg.seekp(0, std::ios::beg);

	unsigned int execRva = resRva + segSize;
	seg.seekp(execRvaOffset, std::ios::beg);
	seg.write((char*)&execRva, 4);
	seg.seekp(execSizeOffset, std::ios::beg);
	seg.write((char*)&execSize, 4);
	seg.seekp(0, std::ios::end);
	seg.write((char*)execData, execSize);

	seg.close();
	delete[]execData;
	



}