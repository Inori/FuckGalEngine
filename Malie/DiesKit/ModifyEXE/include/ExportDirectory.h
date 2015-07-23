/*
* ExportDirectory.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef EXPORTDIRECTORY_H
#define EXPORTDIRECTORY_H
#include "PeHeader.h"

namespace PeLib
{
	/// Class that handles the export directory.
	/**
	* This class handles the export directory.
	* \todo getNameString
	**/
//	template<int bits>
	class ExportDirectory
	{
		private:
		  /// Used to store all necessary information about a file's exported functions.
		  PELIB_IMAGE_EXP_DIRECTORY m_ied;

		public:
		  /// Add another function to be exported.
		  void addFunction(const std::string& strFuncname, dword dwFuncAddr); // EXPORT
		  unsigned int calcNumberOfFunctions() const; // EXPORT
		  void clear(); // EXPORT
		  /// Identifies a function through it's name.
		  int getFunctionIndex(const std::string& strFunctionName) const; // EXPORT
		  /// Read a file's export directory.
		  int read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize, const PeHeader& pehHeader); // EXPORT
		  /// Rebuild the current export directory.
		  void rebuild(std::vector<byte>& vBuffer, dword dwRva) const; // EXPORT
		  void removeFunction(unsigned int index); // EXPORT
		  /// Returns the size of the current export directory.
		  unsigned int size() const; // EXPORT
		  /// Writes the current export directory to a file.
		  int write(const std::string& strFilename, unsigned int uiOffset, unsigned int uiRva) const; // EXPORT

		  /// Changes the name of the file (according to the export directory).
		  void setNameString(const std::string& strFilename); // EXPORT
		  std::string getNameString() const; // EXPORT

		  /// Get the name of an exported function.
		  std::string getFunctionName(unsigned int index) const; // EXPORT
		  /// Get the ordinal of an exported function.
		  word getFunctionOrdinal(unsigned int index) const; // EXPORT
		  /// Get the address of the name of an exported function.
		  dword getAddressOfName(unsigned int index) const; // EXPORT
		  /// Get the address of an exported function.
		  dword getAddressOfFunction(unsigned int index) const; // EXPORT
		  
		  /// Change the name of an exported function.
		  void setFunctionName(unsigned int index, const std::string& strName); // EXPORT
		  /// Change the ordinal of an exported function.
		  void setFunctionOrdinal(unsigned int index, word wValue); // EXPORT
		  /// Change the address of the name of an exported function.
		  void setAddressOfName(unsigned int index, dword dwValue); // EXPORT
		  /// Change the address of an exported function.
		  void setAddressOfFunction(unsigned int index, dword dwValue); // EXPORT
		  
		  /*
		  word getFunctionOrdinal(std::string strFuncname) const;
		  dword getAddressOfName(std::string strFuncname) const;
		  dword getAddressOfFunction(std::string strFuncname) const;

		  void setFunctionOrdinal(std::string strFuncname, word wValue);
		  void setAddressOfName(std::string strFuncname, dword dwValue);
		  void setAddressOfFunction(std::string strFuncname, dword dwValue);
		  */

		  /// Return the Base value of the export directory.
		  dword getBase() const; // EXPORT
		  /// Return the Characteristics value of the export directory.
		  dword getCharacteristics() const; // EXPORT
		  /// Return the TimeDateStamp value of the export directory.
		  dword getTimeDateStamp() const; // EXPORT
		  /// Return the MajorVersion value of the export directory.
		  word getMajorVersion() const; // EXPORT
		  /// Return the MinorVersion value of the export directory.
		  word getMinorVersion() const; // EXPORT
		  /// Return the Name value of the export directory.
		  dword getName() const; // EXPORT
		  /// Return the NumberOfFunctions value of the export directory.
		  dword getNumberOfFunctions() const; // EXPORT
		  /// Return the NumberOfNames value of the export directory.
		  dword getNumberOfNames() const; // EXPORT
		  /// Return the AddressOfFunctions value of the export directory.
		  dword getAddressOfFunctions() const; // EXPORT
		  /// Return the AddressOfNames value of the export directory.
		  dword getAddressOfNames() const; // EXPORT
		  /// Returns the AddressOfNameOrdinals value.
		  dword getAddressOfNameOrdinals() const; // EXPORT

/*		  /// Returns the number of NameOrdinals.
		  dword getNumberOfNameOrdinals() const; // EXPORT
		  /// Returns the number of AddressOfFunctionNames values.
		  dword getNumberOfAddressOfFunctionNames() const; // EXPORT
		  /// Returns the number of AddressOfFunction values.
		  dword getNumberOfAddressOfFunctions() const; // EXPORT
*/
		  /// Set the Base value of the export directory.
		  void setBase(dword dwValue); // EXPORT
		  /// Set the Characteristics value of the export directory.
		  void setCharacteristics(dword dwValue); // EXPORT
		  /// Set the TimeDateStamp value of the export directory.
		  void setTimeDateStamp(dword dwValue); // EXPORT
		  /// Set the MajorVersion value of the export directory.
		  void setMajorVersion(word wValue); // EXPORT
		  /// Set the MinorVersion value of the export directory.
		  void setMinorVersion(word wValue); // EXPORT
		  /// Set the Name value of the export directory.
		  void setName(dword dwValue); // EXPORT
		  /// Set the NumberOfFunctions value of the export directory.
		  void setNumberOfFunctions(dword dwValue); // EXPORT
		  /// Set the NumberOfNames value of the export directory.
		  void setNumberOfNames(dword dwValue); // EXPORT
		  /// Set the AddressOfFunctions value of the export directory.
		  void setAddressOfFunctions(dword dwValue); // EXPORT
		  /// Set the AddressOfNames value of the export directory.
		  void setAddressOfNames(dword dwValue); // EXPORT
		  void setAddressOfNameOrdinals(dword value); // EXPORT
	};
}
#endif
