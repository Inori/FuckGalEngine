/*
* IatDirectory.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef IATDIRECTORY_H
#define IATDIRECTORY_H

#include "PeLibInc.h"

namespace PeLib
{
	/// Class that handles the Import Address Table (IAT)
	/**
	* This class can read and modify the Import Address Table of a PE file.
	**/
	class IatDirectory
	{
		private:
		  std::vector<dword> m_vIat; ///< Stores the individual IAT fields.
		  
		  int read(InputBuffer& inputBuffer, unsigned int size);
		  
		public:
		  /// Reads the Import Address Table from a PE file.
		  int read(const std::string& strFilename, unsigned int dwOffset, unsigned int dwSize); // EXPORT
		  int read(unsigned char* buffer, unsigned int buffersize); // EXPORT
		  /// Returns the number of fields in the IAT.
		  unsigned int calcNumberOfAddresses() const; // EXPORT
		  /// Adds another address to the IAT.
		  void addAddress(dword dwValue); // EXPORT
		  /// Removes an address from the IAT.
		  void removeAddress(unsigned int index); // EXPORT
		  /// Empties the IAT.
		  void clear(); // EXPORT
		  // Rebuilds the IAT.
		  void rebuild(std::vector<byte>& vBuffer) const; // EXPORT
		  /// Returns the size of the current IAT.
		  unsigned int size() const; // EXPORT
		  /// Writes the current IAT to a file.
		  int write(const std::string& strFilename, unsigned int uiOffset) const; // EXPORT

		  /// Retrieve the value of a field in the IAT.
		  dword getAddress(unsigned int index) const; // EXPORT
		  /// Change the value of a field in the IAT.
		  void setAddress(dword dwAddrnr, dword dwValue); // EXPORT
	};
}

#endif

