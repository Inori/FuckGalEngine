/*
* DebugDirectory.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef DEBUGDIRECTORY_H
#define DEBUGDIRECTORY_H

namespace PeLib
{
	/// Class that handles the Debug directory.
	class DebugDirectory
	{
		private:
		  /// Stores the various DebugDirectory structures.
		  std::vector<PELIB_IMG_DEBUG_DIRECTORY> m_vDebugInfo;
		  
		  std::vector<PELIB_IMG_DEBUG_DIRECTORY> read(InputBuffer& ibBuffer, unsigned int uiSize);

		public:
		  void clear(); // EXPORT
		  /// Reads the Debug directory from a file.
		  int read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize); // EXPORT
		  int read(unsigned char* buffer, unsigned int buffersize);
		  /// Rebuilds the current Debug directory.
		  void rebuild(std::vector<byte>& obBuffer) const; // EXPORT
		  /// Returns the size the current Debug directory needs after rebuilding.
		  unsigned int size() const;
		  /// Writes the current Debug directory back to a file.
		  int write(const std::string& strFilename, unsigned int uiOffset) const; // EXPORT

		  /// Returns the number of DebugDirectory image structures in the current DebugDirectory.
		  unsigned int calcNumberOfEntries() const; // EXPORT
		  
		  /// Adds a new debug structure.
		  void addEntry(); // EXPORT
		  /// Removes a debug structure.
		  void removeEntry(unsigned int uiIndex); // EXPORT
		  
		  /// Returns the Characteristics value of a debug structure.
		  dword getCharacteristics(unsigned int uiIndex) const; // EXPORT
		  /// Returns the TimeDateStamp value of a debug structure.
		  dword getTimeDateStamp(unsigned int uiIndex) const; // EXPORT
		  /// Returns the MajorVersion value of a debug structure.
		  word getMajorVersion(unsigned int uiIndex) const; // EXPORT
		  /// Returns the MinorVersion value of a debug structure.
		  word getMinorVersion(unsigned int uiIndex) const; // EXPORT
		  /// Returns the Type value of a debug structure.
		  dword getType(unsigned int uiIndex) const; // EXPORT
		  /// Returns the SizeOfData value of a debug structure.
		  dword getSizeOfData(unsigned int uiIndex) const; // EXPORT
		  /// Returns the AddressOfRawData value of a debug structure.
		  dword getAddressOfRawData(unsigned int uiIndex) const; // EXPORT
		  /// Returns the PointerToRawData value of a debug structure.
		  dword getPointerToRawData(unsigned int uiIndex) const; // EXPORT
		  std::vector<byte> getData(unsigned int index) const; // EXPORT
		  
		  /// Sets the Characteristics value of a debug structure.
		  void setCharacteristics(unsigned int uiIndex, dword dwValue); // EXPORT
		  /// Sets the TimeDateStamp value of a debug structure.
		  void setTimeDateStamp(unsigned int uiIndex, dword dwValue); // EXPORT
		  /// Sets the MajorVersion value of a debug structure.
		  void setMajorVersion(unsigned int uiIndex, word wValue); // EXPORT
		  /// Sets the MinorVersion value of a debug structure.
		  void setMinorVersion(unsigned int uiIndex, word wValue); // EXPORT
		  /// Sets the Type value of a debug structure.
		  void setType(unsigned int uiIndex, dword dwValue); // EXPORT
		  /// Sets the SizeOfData value of a debug structure.
		  void setSizeOfData(unsigned int uiIndex, dword dwValue); // EXPORT
		  /// Sets the AddressOfRawData value of a debug structure.
		  void setAddressOfRawData(unsigned int uiIndex, dword dwValue); // EXPORT
		  /// Sets the PointerToRawData value of a debug structure.
		  void setPointerToRawData(unsigned int uiIndex, dword dwValue); // EXPORT
		  void setData(unsigned int index, const std::vector<byte>& data); // EXPORT
	};
}
#endif
