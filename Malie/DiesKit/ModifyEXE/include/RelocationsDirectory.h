/*
* Relocations.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef RELOCATIONSDIRECTORY_H
#define RELOCATIONSDIRECTORY_H

namespace PeLib
{
	/// Class that handles the relocations directory.
	/**
	* This class handles the relocations directory.
	**/
	class RelocationsDirectory
	{
		private:
		  std::vector<IMG_BASE_RELOC> m_vRelocations; ///< Used to store the relocation data.

		  void read(InputBuffer& inputbuffer, unsigned int uiSize);

		public:
		  /// Returns the number of relocations in the relocations directory.
		  unsigned int calcNumberOfRelocations() const; // EXPORT
		  /// Returns the number of relocation data entries of a specific relocation.
		  unsigned int calcNumberOfRelocationData(unsigned int ulRelocation) const; // EXPORT

		  /// Read a file's relocations directory.
		  int read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize); // EXPORT
		  int read(const unsigned char* buffer, unsigned int buffersize); // EXPORT
		  /// Rebuilds the relocations directory.
		  void rebuild(std::vector<byte>& vBuffer) const; // EXPORT
		  /// Returns the size of the relocations directory.
		  unsigned int size() const; // EXPORT
		  /// Writes the relocations directory to a file.
		  int write(const std::string& strFilename, unsigned int dwOffset) const; // EXPORT

		  /// Returns the VA of a relocation.
		  dword getVirtualAddress(unsigned int ulRelocation) const; // EXPORT
		  /// Returns the SizeOfBlock value of a relocation.
		  dword getSizeOfBlock(unsigned int ulRelocation) const; // EXPORT
		  /// Returns the RelocationData of a relocation.
		  word getRelocationData(unsigned int ulRelocation, unsigned int ulDataNumber) const; // EXPORT
		  
		  /// Changes the relocation data of a relocation.
		  void setRelocationData(unsigned int ulRelocation, unsigned int ulDataNumber, word wData); // EXPORT
		  
		  /// Changes the VirtualAddress of a relocation.
		  void setVirtualAddress(unsigned int ulRelocation, dword dwValue); // EXPORT
		  /// Changes the SizeOfBlock of a relocation.
		  void setSizeOfBlock(unsigned int ulRelocation, dword dwValue); // EXPORT
		  
		  void addRelocation(); // EXPORT
		  /// Adds new data to a relocation.
		  void addRelocationData(unsigned int ulRelocation, word wValue); // EXPORT
		  /// Removes data from a relocation.
//		  void removeRelocationData(unsigned int ulRelocation, word wValue); // EXPORT
		  void removeRelocation(unsigned int index); // EXPORT
		  void removeRelocationData(unsigned int relocindex, unsigned int dataindex); // EXPORT
	};
}

#endif
