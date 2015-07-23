/*
* PeHeader.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef PEHEADER_H
#define PEHEADER_H

#include "PeLibAux.h"

namespace PeLib
{
	class PeHeader
	{
//		protected:
//			virtual void readBaseOfData(InputBuffer& ibBuffer) = 0;
//			virtual void rebuildBaseOfData(OutputBuffer& obBuffer) const = 0;
			
		public:
			virtual ~PeHeader(){};
	};
	
	/// Class that handles the PE header of files.
	/**
	* This class can read and modify PE headers. It provides set- and get functions to access
	* all individual members of a PE header. Furthermore it's possible to validate and rebuild
	* PE headers. A PE header includes the IMAGE_Nt_HEADERS and the section definitions of a PE file.
	* \todo getIdReservedRva
	**/
	template<int x>
	class PeHeaderT : public PeHeader
	{
		private:
			void readBaseOfData(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const;
			void rebuildBaseOfData(OutputBuffer& obBuffer) const;
		
		protected:
		  std::vector<PELIB_IMAGE_SECTION_HEADER> m_vIsh; ///< Stores section header information.
		  PELIB_IMAGE_NT_HEADERS<x> m_inthHeader; ///< Stores Nt header information.
		  dword m_uiOffset; ///< Equivalent to the value returned by #PeLib::MzHeader::getAddressOfPeFile

		public:
		  typedef typename FieldSizes<x>::VAR4_8 VAR4_8;
		
		  PeHeaderT() : m_uiOffset(0)
		  {
		  }
			
		  /// Add a section to the header.
		  int addSection(const std::string& strName, dword dwSize); // EXPORT
		  
		  unsigned int calcSizeOfImage() const; // EXPORT
		  
		  /// Returns the unused space after the header.
		  unsigned int calcSpaceAfterHeader() const; // EXPORT
		  
		  /// Returns the address of the physically first section (not the first defined section).
		  unsigned int calcStartOfCode() const; // EXPORT

		  /// Calculates the offset for a new section of size uiSize.
		  unsigned int calcOffset() const; // EXPORT

		  /// Calculates the Rva for a new section of size uiSize.
		  unsigned int calcRva() const; // EXPORT
		  
		  /// Returns the number of sections in the current file.
		  word calcNumberOfSections() const; // EXPORT

		  void enlargeLastSection(unsigned int uiSize); // EXPORT
		  
		  /// Returns the section Id of the section that contains the offset.
		  word getSectionWithOffset(VAR4_8 dwOffset) const; // EXPORT

		  /// Returns the number of the section which the given relative address points to.
		  word getSectionWithRva(VAR4_8 rva) const; // EXPORT

          bool isValid() const; // EXPORT
          bool isValid(unsigned int foo) const; // EXPORT

		  /// Corrects the current PE header.
		  void makeValid(dword dwOffset); // EXPORT

		  /// Converts a file offset to a relative virtual offset.
		  unsigned int offsetToRva(VAR4_8 dwOffset) const; // EXPORT

		  /// Converts a file offset to a virtual address.
		  unsigned int offsetToVa(VAR4_8 dwOffset) const; // EXPORT

		  /// Reads the PE header of a file.
		  int read(std::string strFilename, unsigned int uiOffset); // EXPORT

		  int read(const unsigned char* pcBuffer, unsigned int uiSize, unsigned int uiOffset); // EXPORT

		  void readHeader(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const;
		  void readDataDirectories(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const;
		  std::vector<PELIB_IMAGE_SECTION_HEADER> readSections(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const;
	
		  /// Rebuilds the current PE header.
		  void rebuild(std::vector<byte>& vBuffer) const; // EXPORT

		  /// Converts a relative virtual address to a file offset.
		  VAR4_8 rvaToOffset(VAR4_8 dwRva) const; // EXPORT

		  /// Converts a relative virtual address to a virtual address.
		  VAR4_8 rvaToVa(VAR4_8 dwRva) const; // EXPORT

		  /// Calculates the size for the current PE header including all section definitions.
		  unsigned int size() const;

		  VAR4_8 vaToRva(VAR4_8 dwRva) const; // EXPORT
		  VAR4_8 vaToOffset(VAR4_8 dwRva) const; // EXPORT

		  /// Save the PE header to a file.
		  int write(std::string strFilename, unsigned int uiOffset) const; // EXPORT

		  /// Writes sections to a file.
		  int writeSections(const std::string& strFilename) const; // EXPORT
		  /// Overwrites a section with new data.
		  int writeSectionData(const std::string& strFilename, word wSecnr, const std::vector<byte>& vBuffer) const; // EXPORT

// header getters
		  /// Returns the Signature value of the header.
		  dword getNtSignature() const; // EXPORT
		  /// Returns the Machine value of the header.
		  word getMachine() const; // EXPORT
		  /// Returns the Sections value of the header.
		  word getNumberOfSections() const; // EXPORT
		  /// Returns the TimeDateStamp value of the header.
		  dword getTimeDateStamp() const; // EXPORT
		  /// Returns the PointerToSymbolTable value of the header.
		  dword getPointerToSymbolTable() const; // EXPORT
		  /// Returns the NumberOfSymbols value of the header.
		  dword getNumberOfSymbols() const; // EXPORT
		  /// Returns the SizeOfOptionalHeader value of the header.
		  word getSizeOfOptionalHeader() const; // EXPORT
		  /// Returns the Characteristics value of the header.
		  word getCharacteristics() const; // EXPORT

		  /// Returns the Magic value of the header.
		  word getMagic() const; // EXPORT
		  /// Returns the MajorLinkerVersion value of the header.
		  byte getMajorLinkerVersion() const; // EXPORT
		  /// Returns the MinorLinkerVersion value of the header.
		  byte getMinorLinkerVersion() const; // EXPORT
		  /// Returns the SizeOfCode value of the header.
		  dword getSizeOfCode() const; // EXPORT
		  /// Returns the SizeOfInitializedData value of the header.
		  dword getSizeOfInitializedData() const; // EXPORT
		  /// Returns the SizeOfUninitializedData value of the header.
		  dword getSizeOfUninitializedData() const; // EXPORT
		  /// Returns the AddressOfEntryPoint value of the header.
		  dword getAddressOfEntryPoint() const; // EXPORT
		  /// Returns the BaseOfCode value of the header.
		  dword getBaseOfCode() const; // EXPORT
		  /// Returns the ImageBase value of the header.
		  VAR4_8 getImageBase() const; // EXPORT
		  /// Returns the SectionAlignment value of the header.
		  dword getSectionAlignment() const; // EXPORT
		  /// Returns the FileAlignment value of the header.
		  dword getFileAlignment() const; // EXPORT
		  /// Returns the MajorOperatingSystemVersion value of the header.
		  word getMajorOperatingSystemVersion() const; // EXPORT
		  /// Returns the MinorOperatingSystemVersion value of the header.
		  word getMinorOperatingSystemVersion() const; // EXPORT
		  /// Returns the MajorImageVersion value of the header.
		  word getMajorImageVersion() const; // EXPORT
		  /// Returns the MinorImageVersion value of the header.
		  word getMinorImageVersion() const; // EXPORT
		  /// Returns the MajorSubsystemVersion value of the header.
		  word getMajorSubsystemVersion() const; // EXPORT
		  /// Returns the MinorSubsystemVersion value of the header.
		  word getMinorSubsystemVersion() const; // EXPORT
		  /// Returns the Reserved1 value of the header.
		  dword getWin32VersionValue() const; // EXPORT
		  /// Returns the SizeOfImage value of the header.
		  dword getSizeOfImage() const; // EXPORT
		  /// Returns the SizeOfHeaders value of the header.
		  dword getSizeOfHeaders() const; // EXPORT
		  /// Returns the CheckSum value of the header.
		  dword getCheckSum() const; // EXPORT
		  /// Returns the Subsystem value of the header.
		  word getSubsystem() const; // EXPORT
		  /// Returns the DllCharacteristics value of the header.
		  word getDllCharacteristics() const; // EXPORT
		  /// Returns the SizeOfStackReserve value of the header.
		  VAR4_8 getSizeOfStackReserve() const; // EXPORT
		  /// Returns the SizeOfStackCommit value of the header.
		  VAR4_8 getSizeOfStackCommit() const; // EXPORT
		  /// Returns the SizeOfHeapReserve value of the header.
		  VAR4_8 getSizeOfHeapReserve() const; // EXPORT
		  /// Returns the SizeOfHeapCommit value of the header.
		  VAR4_8 getSizeOfHeapCommit() const; // EXPORT
		  /// Returns the LoaderFlags value of the header.
		  dword getLoaderFlags() const; // EXPORT
		  /// Returns the NumberOfRvaAndSizes value of the header.
		  dword getNumberOfRvaAndSizes() const; // EXPORT
		  dword calcNumberOfRvaAndSizes() const; // EXPORT

		  void addDataDirectory(); // EXPORT
		  void removeDataDirectory(dword index); // EXPORT
		  
// image directory getters
		  /// Returns the relative virtual address of the image directory Export.
		  dword getIddExportRva() const; // EXPORT
		  /// Returns the size of the image directory Export.
		  dword getIddExportSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Import.
		  dword getIddImportRva() const; // EXPORT
		  /// Returns the size of the image directory Import.
		  dword getIddImportSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Resource.
		  dword getIddResourceRva() const; // EXPORT
		  /// Returns the size of the image directory Resource.
		  dword getIddResourceSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Exception.
		  dword getIddExceptionRva() const; // EXPORT
		  /// Returns the size of the image directory Exception.
		  dword getIddExceptionSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Security.
		  dword getIddSecurityRva() const; // EXPORT
		  /// Returns the size of the image directory Security.
		  dword getIddSecuritySize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Base Reloc.
		  dword getIddBaseRelocRva() const; // EXPORT
		  /// Returns the size of the image directory Base Reloc.
		  dword getIddBaseRelocSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Debug.
		  dword getIddDebugRva() const; // EXPORT
		  /// Returns the size of the image directory Debug.
		  dword getIddDebugSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Architecture.
		  dword getIddArchitectureRva() const; // EXPORT
		  /// Returns the size of the image directory Architecture.
		  dword getIddArchitectureSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory GlobalPtr.
		  dword getIddGlobalPtrRva() const; // EXPORT
		  /// Returns the size of the image directory GlobalPtr.
		  dword getIddGlobalPtrSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Tls.
		  dword getIddTlsRva() const; // EXPORT
		  /// Returns the size of the image directory Tls.
		  dword getIddTlsSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory LoadConfig.
		  dword getIddLoadConfigRva() const; // EXPORT
		  /// Returns the size of the image directory LoadConfig.
		  dword getIddLoadConfigSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory BoundImport.
		  dword getIddBoundImportRva() const; // EXPORT
		  /// Returns the size of the image directory BoundImport.
		  dword getIddBoundImportSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory Iat.
		  dword getIddIatRva() const; // EXPORT
		  /// Returns the size of the image directory Iat.
		  dword getIddIatSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory DelayImport.
		  dword getIddDelayImportRva() const; // EXPORT
		  /// Returns the size of the image directory DelayImport.
		  dword getIddDelayImportSize() const; // EXPORT
		  /// Returns the relative virtual address of the image directory COM Descriptor.
		  dword getIddComHeaderRva() const; // EXPORT
		  /// Returns the size of the image directory COM Descriptor.
		  dword getIddComHeaderSize() const; // EXPORT

		  /// Returns the relative virtual address of an image directory.
		  dword getImageDataDirectoryRva(dword dwDirectory) const; // EXPORT
		  /// Returns the size of an image directory.
		  dword getImageDataDirectorySize(dword dwDirectory) const; // EXPORT
		  
		  void setImageDataDirectoryRva(dword dwDirectory, dword value); // EXPORT
		  void setImageDataDirectorySize(dword dwDirectory, dword value); // EXPORT

// section getters
		  /// Returns the name of a section.
		  std::string getSectionName(word uiSectionnr) const; // EXPORT
		  /// Returns the virtual size of a section.
		  dword getVirtualSize(word uiSectionnr) const; // EXPORT
		  /// Returns the virtual address of a section.
		  dword getVirtualAddress(word uiSectionnr) const; // EXPORT
		  /// Returns the size of a section's raw data.
		  dword getSizeOfRawData(word uiSectionnr) const; // EXPORT
		  /// Returns file offset of the data of a section.
		  dword getPointerToRawData(word uiSectionnr) const; // EXPORT
		  /// Returns the rva of the relocations of a section.
		  dword getPointerToRelocations(word uiSectionnr) const; // EXPORT
		  /// Returns the rva of the line numbers of a section.
		  dword getPointerToLinenumbers(word uiSectionnr) const; // EXPORT
		  /// Returns the number of relocations of a section.
		  dword getNumberOfRelocations(word uiSectionnr) const; // EXPORT
		  /// Returns the number of line numbers of a section.
		  dword getNumberOfLinenumbers(word uiSectionnr) const; // EXPORT
		  /// Returns the characteristics of a section.
		  dword getCharacteristics(word uiSectionnr) const; // EXPORT _section

// header setters
		  /// Sets the Signature value of the header.
		  void setNtSignature(dword value); // EXPORT
		  /// Sets the Machine value of the header.
		  void setMachine(word value); // EXPORT
		  /// Sets the Sections value of the header.
		  void setNumberOfSections(word value); // EXPORT
		  /// Sets the TimeDateStamp value of the header.
		  void setTimeDateStamp(dword value); // EXPORT
		  /// Sets the PointerToSymbolTable value of the header.
		  void setPointerToSymbolTable(dword value); // EXPORT
		  /// Sets the NumberOfSymbols value of the header.
		  void setNumberOfSymbols(dword value); // EXPORT
		  /// Sets the SizeOfOptionalHeader value of the header.
		  void setSizeOfOptionalHeader(word value); // EXPORT
		  /// Sets the Characteristics value of the header.
		  void setCharacteristics(word value); // EXPORT _section

		  /// Sets the Magic value of the header.
		  void setMagic(word value); // EXPORT
		  /// Sets the MajorLinkerVersion value of the header.
		  void setMajorLinkerVersion(byte value); // EXPORT
		  /// Sets the MinorLinkerVersion value of the header.
		  void setMinorLinkerVersion(byte value); // EXPORT
		  /// Sets the SizeOfCode value of the header.
		  void setSizeOfCode(dword value); // EXPORT
		  /// Sets the SizeOfInitializedData value of the header.
		  void setSizeOfInitializedData(dword value); // EXPORT
		  /// Sets the SizeOfUninitializedData value of the header.
		  void setSizeOfUninitializedData(dword value); // EXPORT
		  /// Sets the AddressOfEntryPoint value of the header.
		  void setAddressOfEntryPoint(dword value); // EXPORT
		  /// Sets the BaseOfCode value of the header.
		  void setBaseOfCode(dword value); // EXPORT
		  /// Sets the ImageBase value of the header.
		  void setImageBase(VAR4_8 value); // EXPORT
		  /// Sets the SectionAlignment value of the header.
		  void setSectionAlignment(dword value); // EXPORT
		  /// Sets the FileAlignment value of the header.
		  void setFileAlignment(dword value); // EXPORT
		  /// Sets the MajorOperatingSystemVersion value of the header.
		  void setMajorOperatingSystemVersion(word value); // EXPORT
		  /// Sets the MinorOperatingSystemVersion value of the header.
		  void setMinorOperatingSystemVersion(word value); // EXPORT
		  /// Sets the MajorImageVersion value of the header.
		  void setMajorImageVersion(word value); // EXPORT
		  /// Sets the MinorImageVersion value of the header.
		  void setMinorImageVersion(word value); // EXPORT
		  /// Sets the MajorSubsystemVersion value of the header.
		  void setMajorSubsystemVersion(word value); // EXPORT
		  /// Sets the MinorSubsystemVersion value of the header.
		  void setMinorSubsystemVersion(word value); // EXPORT
		  /// Sets the Reserved1 value of the header.
		  void setWin32VersionValue(dword value); // EXPORT
		  /// Sets the SizeOfImage value of the header.
		  void setSizeOfImage(dword value); // EXPORT
		  /// Sets the SizeOfHeaders value of the header.
		  void setSizeOfHeaders(dword value); // EXPORT
		  /// Sets the CheckSum value of the header.
		  void setCheckSum(dword value); // EXPORT
		  /// Sets the Subsystem value of the header.
		  void setSubsystem(word value); // EXPORT
		  /// Sets the DllCharacteristics value of the header.
		  void setDllCharacteristics(word value); // EXPORT
		  /// Sets the SizeOfStackReserve value of the header.
		  void setSizeOfStackReserve(VAR4_8 value); // EXPORT
		  /// Sets the SizeOfStackCommit value of the header.
		  void setSizeOfStackCommit(VAR4_8 value); // EXPORT
		  /// Sets the SizeOfHeapReserve value of the header.
		  void setSizeOfHeapReserve(VAR4_8 value); // EXPORT
		  /// Sets the SizeOfHeapCommit value of the header.
		  void setSizeOfHeapCommit(VAR4_8 value); // EXPORT
		  /// Sets the LoaderFlags value of the header.
		  void setLoaderFlags(dword value); // EXPORT
		  /// Sets the NumberOfRvaAndSizes value of the header.
		  void setNumberOfRvaAndSizes(dword value); // EXPORT

// image directory getters
		  void setIddDebugRva(dword dwValue); // EXPORT
		  void setIddDebugSize(dword dwValue); // EXPORT
		  void setIddDelayImportRva(dword dwValue); // EXPORT
		  void setIddDelayImportSize(dword dwValue); // EXPORT
		  void setIddExceptionRva(dword dwValue); // EXPORT
		  void setIddExceptionSize(dword dwValue); // EXPORT
		  void setIddGlobalPtrRva(dword dwValue); // EXPORT
		  void setIddGlobalPtrSize(dword dwValue); // EXPORT
		  void setIddIatRva(dword dwValue); // EXPORT
		  void setIddIatSize(dword dwValue); // EXPORT
		  void setIddLoadConfigRva(dword dwValue); // EXPORT
		  void setIddLoadConfigSize(dword dwValue); // EXPORT
		  void setIddResourceRva(dword dwValue); // EXPORT
		  void setIddResourceSize(dword dwValue); // EXPORT
		  void setIddSecurityRva(dword dwValue); // EXPORT
		  void setIddSecuritySize(dword dwValue); // EXPORT
		  void setIddTlsRva(dword dwValue); // EXPORT
		  void setIddTlsSize(dword dwValue); // EXPORT

		  void setIddImportRva(dword dwValue); // EXPORT
		  void setIddImportSize(dword dwValue); // EXPORT
		  void setIddExportRva(dword dwValue); // EXPORT
		  void setIddExportSize(dword dwValue); // EXPORT
		  
		  void setIddBaseRelocRva(dword value); // EXPORT
		  void setIddBaseRelocSize(dword value); // EXPORT
		  void setIddArchitectureRva(dword value); // EXPORT
		  void setIddArchitectureSize(dword value); // EXPORT
		  void setIddComHeaderRva(dword value); // EXPORT
		  void setIddComHeaderSize(dword value); // EXPORT

		  /// Set the name of a section.
		  void setSectionName(word uiSectionnr, std::string strName); // EXPORT
		  /// Set the virtual size of a section.
		  void setVirtualSize(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the virtual address of a section.
		  void setVirtualAddress(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the size of raw data of a section.
		  void setSizeOfRawData(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the file offset of a section.
		  void setPointerToRawData(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the pointer to relocations of a section.
		  void setPointerToRelocations(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the pointer to linenumbers of a section.
		  void setPointerToLinenumbers(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the number of relocations a section.
		  void setNumberOfRelocations(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the number of linenumbers section.
		  void setNumberOfLinenumbers(word uiSectionnr, dword dwValue); // EXPORT
		  /// Set the characteristics of a section.
		  void setCharacteristics(word uiSectionnr, dword dwValue); // EXPORT
	};
	
	class PeHeader32 : public PeHeaderT<32>
	{
		public:
		  /// Returns the BaseOfData value of the header.
		  dword getBaseOfData() const; // EXPORT
		  /// Sets the BaseOfData value of the header.
		  void setBaseOfData(dword value); // EXPORT
	};
	
	class PeHeader64 : public PeHeaderT<64>
	{
	};
	
	template<int x>
	void PeHeaderT<x>::addDataDirectory()
	{
		m_inthHeader.dataDirectories.push_back(PELIB_IMAGE_DATA_DIRECTORY());
	}

	template<int x>
	void PeHeaderT<x>::removeDataDirectory(dword index)
	{
		m_inthHeader.dataDirectories.erase(m_inthHeader.dataDirectories.begin() + index);
	}
		  
	/**
	* Adds a new section to the header. The physical and virtual address as well as the virtual
	* size of the section will be determined automatically from the raw size. The section
	* characteristics will be set to IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ |
	* IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_CODE. All other values will be set to 0.
	* Note: It's important that if the current header's FileAlignment and/or SectionAlignment values are
	* 0 this function will fail.
	* @param strName Name of the new section. If this name is longer than 8 bytes only the first 8 bytes will be used.
	* @param dwSize Physical size of the new section in bytes.
	* \todo Better code that handles files with 0 sections.
	**/
	template<int x>
	int PeHeaderT<x>::addSection(const std::string& strName, dword dwSize)
	{
		unsigned int uiSecnr = calcNumberOfSections();

		if (!getFileAlignment())
		{
			return ERROR_NO_FILE_ALIGNMENT;
		}
		else if (!getSectionAlignment())
		{
			return ERROR_NO_SECTION_ALIGNMENT;
		}
			
		if (uiSecnr) // Always allow 1 section.
		{
			if (uiSecnr == 0xFFFF)
			{
				return ERROR_TOO_MANY_SECTIONS;
			}
			else if (calcSpaceAfterHeader() < PELIB_IMAGE_SECTION_HEADER::size())
			{
				return ERROR_NOT_ENOUGH_SPACE;
			}
		}

		dword dwOffset = calcOffset(/*dwSize*/);
		dword dwRva = calcRva(/*dwSize*/);

		PELIB_IMAGE_SECTION_HEADER ishdCurr;
		m_vIsh.push_back(ishdCurr);

		setSectionName(uiSecnr, strName);
		setSizeOfRawData(uiSecnr, alignOffset(dwSize, getFileAlignment()));
		setPointerToRawData(uiSecnr, dwOffset);
		setVirtualSize(uiSecnr, alignOffset(dwSize, getSectionAlignment()));
		setVirtualAddress(uiSecnr, dwRva);
		setCharacteristics(uiSecnr, PELIB_IMAGE_SCN_MEM_WRITE | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_CNT_CODE);
		
		return NO_ERROR;
	}

	/**
	* Calculates a valid SizeOfImage value given the information from the current PE header.
	* Note that this calculation works in Win2K but probably does not work in Win9X. I didn't test that though.
	* @return Valid SizeOfImage value.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::calcSizeOfImage() const
	{
		// Major note here: It's possible for sections to exist with a Virtual Size of 0.
		//                  That's why it's necessary to use std::max(Vsize, RawSize) here.
		//                  An example for such a file is dbeng6.exe (made by Sybase).
		//                  In this file each and every section has a VSize of 0 but it still runs.
		
		std::vector<PELIB_IMAGE_SECTION_HEADER>::const_iterator ishLastSection = std::max_element(m_vIsh.begin(), m_vIsh.end(), std::mem_fun_ref(&PELIB_IMAGE_SECTION_HEADER::biggerVirtualAddress));
		if (ishLastSection->VirtualSize != 0) return ishLastSection->VirtualAddress + ishLastSection->VirtualSize;
		return ishLastSection->VirtualAddress + std::max(ishLastSection->VirtualSize, ishLastSection->SizeOfRawData);
	}

	/**
	* Calculates the space between the last byte of the header and the first byte that's used for something
	* else (that's either the first section or an image directory).
	* @return Unused space after the header.
	* \todo There are PE files with sections beginning at offset 0. They
	* need to be considered.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::calcSpaceAfterHeader() const
	{
		return (calcStartOfCode() > size() + m_uiOffset) ? calcStartOfCode() - (size() + m_uiOffset) : 0;
	}

	/**
	* Returns the first offset of the file that's actually used for something different than the header.
	* That something is not necessarily code, it can be a data directory too.
	* This offset can be the beginning of a section or the beginning of a directory.
	* \todo Some optimizization is surely possible here.
	* \todo There are PE files with sections beginning at offset 0. They
	* need to be considered. Returning 0 for these files doesn't really make sense.
	* So far these sections are disregarded.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::calcStartOfCode() const
	{
		unsigned int directories = calcNumberOfRvaAndSizes();
		dword dwMinOffset = 0xFFFFFFFF;
		if (directories >= 1 && getIddExportRva() && rvaToOffset(getIddExportRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddExportRva());
		if (directories >= 2 && getIddImportRva() && rvaToOffset(getIddImportRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddImportRva());
		if (directories >= 3 && getIddResourceRva() && rvaToOffset(getIddResourceRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddResourceRva());
		if (directories >= 4 && getIddExceptionRva()  && rvaToOffset(getIddExceptionRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddExceptionRva());
		if (directories >= 5 && getIddSecurityRva()  && rvaToOffset(getIddSecurityRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddSecurityRva());
		if (directories >= 6 && getIddBaseRelocRva()  && rvaToOffset(getIddBaseRelocRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddBaseRelocRva());
		if (directories >= 7 && getIddDebugRva()  && rvaToOffset(getIddDebugRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddDebugRva());
		if (directories >= 8 && getIddArchitectureRva()  && rvaToOffset(getIddArchitectureRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddArchitectureRva());
		if (directories >= 9 && getIddGlobalPtrRva()  && rvaToOffset(getIddGlobalPtrRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddGlobalPtrRva());
		if (directories >= 10 && getIddTlsRva()  && rvaToOffset(getIddTlsRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddTlsRva());
		if (directories >= 11 && getIddLoadConfigRva()  && rvaToOffset(getIddLoadConfigRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddLoadConfigRva());
		if (directories >= 12 && getIddBoundImportRva()  && rvaToOffset(getIddBoundImportRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddBoundImportRva());
		if (directories >= 13 && getIddIatRva()  && rvaToOffset(getIddIatRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddIatRva());
		if (directories >= 14 && getIddDelayImportRva()  && rvaToOffset(getIddDelayImportRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddDelayImportRva());
		if (directories >= 15 && getIddComHeaderRva()  && rvaToOffset(getIddComHeaderRva()) < dwMinOffset) dwMinOffset = rvaToOffset(getIddComHeaderRva());
		
		for (word i=0;i<calcNumberOfSections();i++)
		{
			if ((getPointerToRawData(i) < dwMinOffset || dwMinOffset == 0xFFFFFFFF) && getSizeOfRawData(i))
			{
				if (getPointerToRawData(i)) dwMinOffset = getPointerToRawData(i);
			}
		}
		return dwMinOffset;
	}

	/**
	* Calculates the file offset for a new section. The file offset will already be aligned to the file's FileAlignment.
	* @return Aligned file offset.
	* \todo uiSize isn't used yet. Will be used later on to search for caves.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::calcOffset(/*unsigned int uiSize*/) const
	{
		unsigned int maxoffset = size();

		for (word i=0;i<calcNumberOfSections();i++)
		{
			if (getPointerToRawData(i) + getSizeOfRawData(i) > maxoffset) maxoffset = getPointerToRawData(i) + getSizeOfRawData(i);
		}

		return alignOffset(maxoffset, getFileAlignment());
	}

	/**
	* Calculates the Rva for a new section. The Rva will already be aligned to the file's SectionAlignment.
	* \todo uiSize isn't used yet. Will be used later on to search for caves.
	* @return Aligned Rva.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::calcRva(/*unsigned int uiSize*/) const
	{
		// Major note here: It's possible for sections to exist with a Virtual Size of 0.
		//                  That's why it's necessary to use std::max(Vsize, RawSize) here.
		//                  An example for such a file is dbeng6.exe (made by Sybase).
		//                  In this file each and every section has a VSize of 0 but it still runs.
		
		unsigned int maxoffset = size();
		for (word i=0;i<calcNumberOfSections();i++)
		{
			if (getVirtualAddress(i) + std::max(getVirtualSize(i), getSizeOfRawData(i)) > maxoffset) maxoffset = getVirtualAddress(i) + std::max(getVirtualSize(i), getSizeOfRawData(i));
		}

		return alignOffset(maxoffset, getSectionAlignment());
	}

	/**
	* Returns the number of currently defined sections. Note that this value can be different from the number
	* of sections according to the header (see #PeLib::PeHeaderT<x>::getNumberOfSections).
	* @return Number of currently defined sections.
	**/
	template<int x>
	word PeHeaderT<x>::calcNumberOfSections() const
	{
		return static_cast<PeLib::word>(m_vIsh.size());
	}

	/**
	* Enlarges the physically last section in the file.
	* @param uiSize Additional size that's added to the section's size.
	**/
	template<int x>
	void PeHeaderT<x>::enlargeLastSection(unsigned int uiSize)
	{
		std::vector<PELIB_IMAGE_SECTION_HEADER>::iterator ishLastSection = std::max_element(m_vIsh.begin(), m_vIsh.end(), std::mem_fun_ref(&PELIB_IMAGE_SECTION_HEADER::biggerFileOffset));
		unsigned int uiRawDataSize = alignOffset(ishLastSection->SizeOfRawData + uiSize, getFileAlignment());
		
		ishLastSection->SizeOfRawData = uiRawDataSize;
		ishLastSection->VirtualSize = ishLastSection->SizeOfRawData;
		
		setSizeOfImage(calcSizeOfImage());
	}

	/**
	* Determines the section which contains the file offset.
	* @param dwOffset File offset.
	* @return Section Id of the section which contains the offset.
	**/
	template<int x>
	word PeHeaderT<x>::getSectionWithOffset(VAR4_8 dwOffset) const
	{
		// Offset = 0 must be handled explicitly as there are files
		// with sections that begin at offset 0, that means the section
		// only exists in memory.
		
		if (!dwOffset) return std::numeric_limits<word>::max();
		
		for (word i=0;i<calcNumberOfSections();i++)
		{
			// Explicity exclude sections with raw pointer = 0.
			dword rawptr = getPointerToRawData(i);
			if (rawptr && rawptr <= dwOffset && rawptr + getSizeOfRawData(i) > dwOffset) return i;
		}
		
		return std::numeric_limits<word>::max();
	}

	/**
	* Determines the section which contains the Rva.
	* @param dwRva A relative virtual address.
	* @return Section Id of the section which contains the Rva.
	**/
	template<int x>
	word PeHeaderT<x>::getSectionWithRva(VAR4_8 dwRva) const
	{
		// Major note here: It's possible for sections to exist with a Virtual Size of 0.
		//                  That's why it's necessary to use std::max(Vsize, RawSize) here.
		//                  An example for such a file is dbeng6.exe (made by Sybase).
		//                  In this file each and every section has a VSize of 0 but it still runs.
		
		for (word i=0;i<calcNumberOfSections();i++)
		{
			// Weird VC++7 error doesn't allow me to use std::max here.
			dword max = getVirtualSize(i) >= getSizeOfRawData(i) ? getVirtualSize(i) : getSizeOfRawData(i);
			if (getVirtualAddress(i) <= dwRva && getVirtualAddress(i) + max > dwRva) return i;
		}
		
		return -1;
	}

	/**
	* Corrects all faulty values of the current PE header. The following values will be corrected: NtSignature,
	* NumberOfSections, SizeOfOptionalHeader, FileAlignment (will be aligned to n*0x200),
	* SectionAlignment (will be aligned to n*0x1000), NumberOfRvaAndSizes, SizeOfHeaders, SizeOfImage,
	* Magic, Characteristics.
	* @param dwOffset Beginning of PeHeader (see #PeLib::MzHeader::getAddressOfPeHeader).
    * \todo 32bit and 64bit versions.
	**/
	template<int x>
	void PeHeaderT<x>::makeValid(dword dwOffset)
	{
		setNtSignature(PELIB_IMAGE_NT_SIGNATURE); // 'PE'
		setMachine(PELIB_IMAGE_FILE_MACHINE_I386);
		setNumberOfSections(calcNumberOfSections());
		
		// Check if 64 bits.
		setSizeOfOptionalHeader(PELIB_IMAGE_OPTIONAL_HEADER<x>::size() + calcNumberOfRvaAndSizes() * 8);

		// Check if 64 bits.
		dword dwCharacteristics = PELIB_IMAGE_FILE_EXECUTABLE_IMAGE | PELIB_IMAGE_FILE_32BIT_MACHINE;
		setCharacteristics(dwCharacteristics);

		// Check if 64 bits.
		setMagic(PELIB_IMAGE_NT_OPTIONAL_HDR32_MAGIC);

		// setImageBase(0x01000000);

		// Align file and section alignment values
		unsigned int dwAlignedOffset = alignOffset(getSectionAlignment(), 0x1000);
		setSectionAlignment(dwAlignedOffset ? dwAlignedOffset : 0x1000);

		dwAlignedOffset = alignOffset(getFileAlignment(), 0x200);
		setFileAlignment(dwAlignedOffset ? dwAlignedOffset : 0x200);

//		setMajorSubsystemVersion(4);
//		setSubsystem(IMAGE_SUBSYSTEM_WINDOWS_GUI);
		setNumberOfRvaAndSizes(calcNumberOfRvaAndSizes());
		
		// Code below depends on code above. Don't change the order.
		dword dwSizeOfHeaders = alignOffset(dwOffset + size(), getFileAlignment());
		setSizeOfHeaders(dwSizeOfHeaders);

		dword dwSizeOfImage = alignOffset(dwSizeOfHeaders, getSectionAlignment());

		for (int i=0;i<calcNumberOfSections();i++)
		{
			dwSizeOfImage += alignOffset(getVirtualSize(i), getSectionAlignment());
		}

		dwSizeOfImage = alignOffset(dwSizeOfImage, getSectionAlignment());
		setSizeOfImage(dwSizeOfImage);
	}

	template<int x>
	unsigned int PeHeaderT<x>::offsetToRva(VAR4_8 dwOffset) const
	{
		if (dwOffset < calcStartOfCode()) return dwOffset;
	
		PeLib::word uiSecnr = getSectionWithOffset(dwOffset);
		
		if (uiSecnr == 0xFFFF) return -1;
		
		return getVirtualAddress(uiSecnr) + dwOffset - getPointerToRawData(uiSecnr);
	}

	/**
	* Converts a file offset to a virtual address.
	* @param dwOffset File offset.
	* @return Virtual Address.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::offsetToVa(VAR4_8 dwOffset) const
	{
		if (dwOffset < calcStartOfCode()) return getImageBase() + dwOffset;
		
		PeLib::word uiSecnr = getSectionWithOffset(dwOffset);
		
		if (uiSecnr == 0xFFFF) return -1;
		
		return getImageBase() + getVirtualAddress(uiSecnr) + dwOffset - getPointerToRawData(uiSecnr);
	}

	template<int x>
	void PeHeaderT<x>::readHeader(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const
	{
		ibBuffer >> header.Signature;
		
		ibBuffer >> header.FileHeader.Machine;
		ibBuffer >> header.FileHeader.NumberOfSections;
		ibBuffer >> header.FileHeader.TimeDateStamp;
		ibBuffer >> header.FileHeader.PointerToSymbolTable;
		ibBuffer >> header.FileHeader.NumberOfSymbols;
		ibBuffer >> header.FileHeader.SizeOfOptionalHeader;
		ibBuffer >> header.FileHeader.Characteristics;
		ibBuffer >> header.OptionalHeader.Magic;

		ibBuffer >> header.OptionalHeader.MajorLinkerVersion;
		ibBuffer >> header.OptionalHeader.MinorLinkerVersion;
		ibBuffer >> header.OptionalHeader.SizeOfCode;
		ibBuffer >> header.OptionalHeader.SizeOfInitializedData;
		ibBuffer >> header.OptionalHeader.SizeOfUninitializedData;
		ibBuffer >> header.OptionalHeader.AddressOfEntryPoint;
		ibBuffer >> header.OptionalHeader.BaseOfCode;
		readBaseOfData(ibBuffer, header);
		ibBuffer >> header.OptionalHeader.ImageBase;
		ibBuffer >> header.OptionalHeader.SectionAlignment;
		ibBuffer >> header.OptionalHeader.FileAlignment;
		ibBuffer >> header.OptionalHeader.MajorOperatingSystemVersion;
		ibBuffer >> header.OptionalHeader.MinorOperatingSystemVersion;
		ibBuffer >> header.OptionalHeader.MajorImageVersion;
		ibBuffer >> header.OptionalHeader.MinorImageVersion;
		ibBuffer >> header.OptionalHeader.MajorSubsystemVersion;
		ibBuffer >> header.OptionalHeader.MinorSubsystemVersion;
		ibBuffer >> header.OptionalHeader.Win32VersionValue;
		ibBuffer >> header.OptionalHeader.SizeOfImage;
		ibBuffer >> header.OptionalHeader.SizeOfHeaders;
		ibBuffer >> header.OptionalHeader.CheckSum;
		ibBuffer >> header.OptionalHeader.Subsystem;
		ibBuffer >> header.OptionalHeader.DllCharacteristics;
		ibBuffer >> header.OptionalHeader.SizeOfStackReserve;
		ibBuffer >> header.OptionalHeader.SizeOfStackCommit;
		ibBuffer >> header.OptionalHeader.SizeOfHeapReserve;
		ibBuffer >> header.OptionalHeader.SizeOfHeapCommit;
		ibBuffer >> header.OptionalHeader.LoaderFlags;
		ibBuffer >> header.OptionalHeader.NumberOfRvaAndSizes;
	}
	
	template<int x>
	void PeHeaderT<x>::readDataDirectories(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const
	{
		PELIB_IMAGE_DATA_DIRECTORY idd;

		for (unsigned int i=0;i<header.OptionalHeader.NumberOfRvaAndSizes;i++)
		{
			ibBuffer >> idd.VirtualAddress;
			ibBuffer >> idd.Size;
			header.dataDirectories.push_back(idd);
		}
	}
	
	template<int x>
	std::vector<PELIB_IMAGE_SECTION_HEADER> PeHeaderT<x>::readSections(InputBuffer& ibBuffer, PELIB_IMAGE_NT_HEADERS<x>& header) const
	{
		const unsigned int nrSections = header.FileHeader.NumberOfSections;
		PELIB_IMAGE_SECTION_HEADER ishCurr;

		std::vector<PELIB_IMAGE_SECTION_HEADER> vIshdCurr;

		for (unsigned int i=0;i<nrSections;i++)
		{
			ibBuffer.read(reinterpret_cast<char*>(ishCurr.Name), 8);
			ibBuffer >> ishCurr.VirtualSize;
			ibBuffer >> ishCurr.VirtualAddress;
			ibBuffer >> ishCurr.SizeOfRawData;
			ibBuffer >> ishCurr.PointerToRawData;
			ibBuffer >> ishCurr.PointerToRelocations;
			ibBuffer >> ishCurr.PointerToLinenumbers;
			ibBuffer >> ishCurr.NumberOfRelocations;
			ibBuffer >> ishCurr.NumberOfLinenumbers;
			ibBuffer >> ishCurr.Characteristics;
			vIshdCurr.push_back(ishCurr);
		}

		return vIshdCurr;
	}
	
	template<int x>
	int PeHeaderT<x>::read(const unsigned char* pcBuffer, unsigned int uiSize, unsigned int uiOffset)
	{
		if (uiSize < m_inthHeader.size())
		{
			return ERROR_INVALID_FILE;
		}
		
		std::vector<unsigned char> vBuffer(pcBuffer, pcBuffer + m_inthHeader.size());

		InputBuffer ibBuffer(vBuffer);
		PELIB_IMAGE_NT_HEADERS<x> header;
		
		readHeader(ibBuffer, header);
		
		if (uiSize < m_inthHeader.size() + header.OptionalHeader.NumberOfRvaAndSizes * 8 + header.FileHeader.NumberOfSections * 0x28)
		{
			return ERROR_INVALID_FILE;
		}
		
		vBuffer.resize(header.OptionalHeader.NumberOfRvaAndSizes * 8 + header.FileHeader.NumberOfSections * 0x28);
		vBuffer.assign(pcBuffer + m_inthHeader.size(), pcBuffer + m_inthHeader.size() + header.OptionalHeader.NumberOfRvaAndSizes * 8 + header.FileHeader.NumberOfSections * 0x28);
		
		ibBuffer.setBuffer(vBuffer);
		
		readDataDirectories(ibBuffer, header);
		
		m_vIsh = readSections(ibBuffer, header);

		std::swap(m_inthHeader, header);

		m_uiOffset = uiOffset;

		return NO_ERROR;
	}
	
	/**
	* Reads the PE header from a file Note that this function does not verify if a file is actually a MZ file.
	* For this purpose see #PeLib::PeHeaderT<x>::isValid. The only check this function makes is a check to see if
	* the file is large enough to be a PE header. If the data is valid doesn't matter.
	* @param strFilename Name of the file which will be read.
	* @param uiOffset File offset of PE header (see #PeLib::MzHeader::getAddressOfPeHeader).
	**/
	template<int x>
	int PeHeaderT<x>::read(std::string strFilename, unsigned int uiOffset)
	{
		std::ifstream ifFile(strFilename.c_str(), std::ios::binary);
		
		if (!ifFile)
		{
			return ERROR_OPENING_FILE;
		}
		
		// File too small
		if (fileSize(ifFile) < uiOffset + m_inthHeader.size())
		{
			return ERROR_INVALID_FILE;
		}
		
		std::vector<unsigned char> vBuffer(m_inthHeader.size());

		ifFile.seekg(uiOffset, std::ios::beg);
		ifFile.read(reinterpret_cast<char*>(&vBuffer[0]), static_cast<std::streamsize>(vBuffer.size()));

		InputBuffer ibBuffer(vBuffer);
		PELIB_IMAGE_NT_HEADERS<x> header;
		
		readHeader(ibBuffer, header);

		vBuffer.resize(header.OptionalHeader.NumberOfRvaAndSizes * 8 + header.FileHeader.NumberOfSections * 0x28);

		ifFile.read(reinterpret_cast<char*>(&vBuffer[0]), static_cast<std::streamsize>(vBuffer.size()));
		if (!ifFile)
		{
			return ERROR_INVALID_FILE;
		}

		ibBuffer.setBuffer(vBuffer);
		
		readDataDirectories(ibBuffer, header);
		
		// Sections
//		const unsigned int nrSections = header.FileHeader.NumberOfSections;
//		if (fileSize(ifFile) < uiOffset + m_inthHeader.size() + nrSections * PELIB_IMAGE_SECTION_HEADER::size())
//		{
//			return ERROR_INVALID_FILE;
//		}
		
		m_vIsh = readSections(ibBuffer, header);

		std::swap(m_inthHeader, header);

		m_uiOffset = uiOffset;

		ifFile.close();
		
		return NO_ERROR;
	}

	/**
	* Rebuilds the PE header so that it can be written to a file. It's not guaranteed that the
	* header will be valid. If you want to make sure that the header will be valid you
	* must call #PeLib::PeHeaderT<x>::makeValid first.
	* @param vBuffer Buffer where the rebuilt header will be stored.
	**/
	template<int x>
	void PeHeaderT<x>::rebuild(std::vector<byte>& vBuffer) const
	{
		OutputBuffer obBuffer(vBuffer);

		obBuffer << m_inthHeader.Signature;

		obBuffer << m_inthHeader.FileHeader.Machine;
		obBuffer << m_inthHeader.FileHeader.NumberOfSections;
		obBuffer << m_inthHeader.FileHeader.TimeDateStamp;
		obBuffer << m_inthHeader.FileHeader.PointerToSymbolTable;
		obBuffer << m_inthHeader.FileHeader.NumberOfSymbols;
		obBuffer << m_inthHeader.FileHeader.SizeOfOptionalHeader;
		obBuffer << m_inthHeader.FileHeader.Characteristics;
		obBuffer << m_inthHeader.OptionalHeader.Magic;
		obBuffer << m_inthHeader.OptionalHeader.MajorLinkerVersion;
		obBuffer << m_inthHeader.OptionalHeader.MinorLinkerVersion;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfCode;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfInitializedData;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfUninitializedData;
		obBuffer << m_inthHeader.OptionalHeader.AddressOfEntryPoint;
		obBuffer << m_inthHeader.OptionalHeader.BaseOfCode;
		rebuildBaseOfData(obBuffer);
//		obBuffer << m_inthHeader.OptionalHeader.BaseOfData;
		obBuffer << m_inthHeader.OptionalHeader.ImageBase;
		obBuffer << m_inthHeader.OptionalHeader.SectionAlignment;
		obBuffer << m_inthHeader.OptionalHeader.FileAlignment;
		obBuffer << m_inthHeader.OptionalHeader.MajorOperatingSystemVersion;
		obBuffer << m_inthHeader.OptionalHeader.MinorOperatingSystemVersion;
		obBuffer << m_inthHeader.OptionalHeader.MajorImageVersion;
		obBuffer << m_inthHeader.OptionalHeader.MinorImageVersion;
		obBuffer << m_inthHeader.OptionalHeader.MajorSubsystemVersion;
		obBuffer << m_inthHeader.OptionalHeader.MinorSubsystemVersion;
		obBuffer << m_inthHeader.OptionalHeader.Win32VersionValue;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfImage;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfHeaders;
		obBuffer << m_inthHeader.OptionalHeader.CheckSum;
		obBuffer << m_inthHeader.OptionalHeader.Subsystem;
		obBuffer << m_inthHeader.OptionalHeader.DllCharacteristics;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfStackReserve;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfStackCommit;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfHeapReserve;
		obBuffer << m_inthHeader.OptionalHeader.SizeOfHeapCommit;
		obBuffer << m_inthHeader.OptionalHeader.LoaderFlags;
		obBuffer << m_inthHeader.OptionalHeader.NumberOfRvaAndSizes;

		// The 0x10 data directories
		for (unsigned int i=0;i<calcNumberOfRvaAndSizes();i++)
		{
			obBuffer << m_inthHeader.dataDirectories[i].VirtualAddress;
			obBuffer << m_inthHeader.dataDirectories[i].Size;
		}
		
		// The section definitions
		const unsigned int nrSections = calcNumberOfSections();
		for (unsigned int i=0;i<nrSections;i++)
		{
			char temp[9] = {0};
			strcpy(temp, getSectionName(i).c_str());
			obBuffer.add(temp, 8);
			obBuffer << m_vIsh[i].VirtualSize;
			obBuffer << m_vIsh[i].VirtualAddress;
			obBuffer << m_vIsh[i].SizeOfRawData;
			obBuffer << m_vIsh[i].PointerToRawData;
			obBuffer << m_vIsh[i].PointerToRelocations;
			obBuffer << m_vIsh[i].PointerToLinenumbers;
			obBuffer << m_vIsh[i].NumberOfRelocations;
			obBuffer << m_vIsh[i].NumberOfLinenumbers;
			obBuffer << m_vIsh[i].Characteristics;
		}
	}

	/**
	* Converts a relative virtual offset to a file offset.
	* @param dwRva A relative virtual offset.
	* @return A file offset.
	* \todo It's not always 0x1000.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::rvaToOffset(VAR4_8 dwRva) const
	{
		// XXX: Not correct
		if (dwRva < 0x1000) return dwRva;
		
		PeLib::word uiSecnr = getSectionWithRva(dwRva);
		if (uiSecnr == 0xFFFF || dwRva > getVirtualAddress(uiSecnr) + getSizeOfRawData(uiSecnr))
		{
			return std::numeric_limits<VAR4_8>::max();
		}
		
		return getPointerToRawData(uiSecnr) + dwRva - getVirtualAddress(uiSecnr);
	}

	/**
	* Converts a relative virtual offset to a virtual offset.
	* @param dwRva A relative virtual offset.
	* @return A virtual offset.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::rvaToVa(VAR4_8 dwRva) const
	{
		return getImageBase() + dwRva;
	}

	/**
	* Calculates the size of the current PE header. This includes the actual header and the section definitions.
	* @return Size of the current PE header.
	* \todo Better handling of files with less than 0x10 directories.
	**/
	template<int x>
	unsigned int PeHeaderT<x>::size() const
	{
		return m_inthHeader.size() + getNumberOfSections() * PELIB_IMAGE_SECTION_HEADER::size();
	}

	// \todo Not sure if this works.
	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::vaToRva(VAR4_8 dwRva) const
	{
		if (dwRva - getImageBase() < calcStartOfCode()) return dwRva - getImageBase();
		
		if (getSectionWithRva(dwRva - getImageBase()) == 0xFFFF) return -1;
		
		return dwRva - getImageBase();
	}

	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::vaToOffset(VAR4_8 dwRva) const
	{
		return rvaToOffset(dwRva - getImageBase());
	}

	/**
	* Saves the PE header to a file. Note that this only saves the header information, if you have added sections
	* and want to save these to the file you have to call #PeLib::PeHeaderT<x>::saveSections too. This function also
	* does not verify if the PE header is correct. If you want to make sure that the current PE header is valid,
	* call #PeLib::PeHeaderT<x>::isValid and #PeLib::PeHeaderT<x>::makeValid first.
	* @param strFilename Filename of the file the header will be written to.
	* @param uiOffset File offset the header will be written to.
	**/
	template<int x>
	int PeHeaderT<x>::write(std::string strFilename, unsigned int uiOffset) const
	{
		std::fstream ofFile(strFilename.c_str(), std::ios_base::in);
		
		if (!ofFile)
		{
			ofFile.clear();
			ofFile.open(strFilename.c_str(), std::ios_base::out | std::ios_base::binary);
		}
		else
		{
			ofFile.close();
			ofFile.open(strFilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		}
		
		if (!ofFile)
		{
			return ERROR_OPENING_FILE;
		}
		
		ofFile.seekp(uiOffset, std::ios::beg);

		std::vector<unsigned char> vBuffer;

		rebuild(vBuffer);

		ofFile.write(reinterpret_cast<const char*>(&vBuffer[0]), static_cast<unsigned int>(vBuffer.size()));

		ofFile.close();
		
		return NO_ERROR;
	}


	/**
	* Overwrites a section's data.
	* @param wSecnr Number of the section which will be overwritten.
	* @param strFilename Name of the file where the section will be written to.
	* @param wSecnr Number of the section that will be written.
	* @param vBuffer New data of the section.
	**/
	template<int x>
	int PeHeaderT<x>::writeSectionData(const std::string& strFilename, word wSecnr, const std::vector<byte>& vBuffer) const
	{
		std::fstream ofFile(strFilename.c_str(), std::ios_base::in);
		
		if (!ofFile)
		{
			ofFile.clear();
			ofFile.open(strFilename.c_str(), std::ios_base::out | std::ios_base::binary);
		}
		else
		{
			ofFile.close();
			ofFile.open(strFilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		}

		if (!ofFile)
		{
			ofFile.clear();

			return ERROR_OPENING_FILE;
		}

		ofFile.seekp(getPointerToRawData(wSecnr), std::ios::beg);

		ofFile.write(reinterpret_cast<const char*>(&vBuffer[0]), std::min(static_cast<unsigned int>(vBuffer.size()), getSizeOfRawData(wSecnr)));

		ofFile.close();
		
		return NO_ERROR;
	}

	template<int x>
	int PeHeaderT<x>::writeSections(const std::string& strFilename) const
	{
		std::fstream ofFile(strFilename.c_str(), std::ios_base::in);
		
		if (!ofFile)
		{
			ofFile.clear();
			ofFile.open(strFilename.c_str(), std::ios_base::out | std::ios_base::binary);
		}
		else
		{
			ofFile.close();
			ofFile.open(strFilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		}

		if (!ofFile)
		{
			return ERROR_OPENING_FILE;
		}

		unsigned int uiFilesize = fileSize(ofFile);

		for (int i=0;i<calcNumberOfSections();i++)
		{
			if (uiFilesize < getPointerToRawData(i) + getSizeOfRawData(i))
			{
				unsigned int uiToWrite = getPointerToRawData(i) + getSizeOfRawData(i) - uiFilesize;
				std::vector<char> vBuffer(uiToWrite);
				ofFile.seekp(0, std::ios::end);
				ofFile.write(&vBuffer[0], static_cast<unsigned int>(vBuffer.size()));
				uiFilesize = getPointerToRawData(i) + getSizeOfRawData(i);
			}
		}

		ofFile.close();
		
		return NO_ERROR;
	}

	/**
	* Returns the file's Nt signature.
	* @return The Nt signature value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getNtSignature() const
	{
		return m_inthHeader.Signature;
	}

	/**
	* Returns the file's machine.
	* @return The Machine value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMachine() const
	{
		return m_inthHeader.FileHeader.Machine;
	}


	/**
	* Returns the file's number of sections as defined in the header. Note that this value can be different
	* from the number of defined sections (#see PeLib::PeHeaderT<x>::getNumberOfSections).
	* @return The NumberOfSections value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getNumberOfSections() const
	{
		return m_inthHeader.FileHeader.NumberOfSections;
	}

	/** 
	* Returns the file's TimeDateStamp.
	* @return The TimeDateStamp value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getTimeDateStamp() const
	{
		return m_inthHeader.FileHeader.TimeDateStamp;
	}

	/**
	* Returns the relative virtual address of the file's symbol table.
	* @return The PointerToSymbolTable value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getPointerToSymbolTable() const
	{
		return m_inthHeader.FileHeader.PointerToSymbolTable;
	}

	/**
	* Returns the number of symbols of the file's symbol table.
	* @return The NumberOfSymbols value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getNumberOfSymbols() const
	{
		return m_inthHeader.FileHeader.NumberOfSymbols;
	}

	/**
	* Returns the size of optional header of the file.
	* @return The SizeOfOptionalHeader value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getSizeOfOptionalHeader() const
	{
		return m_inthHeader.FileHeader.SizeOfOptionalHeader;
	}

	/**
	* @return The Characteristics value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getCharacteristics() const
	{
		return m_inthHeader.FileHeader.Characteristics;
	}

	/**
	* @return The Magic value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMagic() const
	{
		return m_inthHeader.OptionalHeader.Magic;
	}

	/**
	* @return The MajorLinkerVersion value from the PE header.
	**/
	template<int x>
	byte PeHeaderT<x>::getMajorLinkerVersion() const
	{
		return m_inthHeader.OptionalHeader.MajorLinkerVersion;
	}

	/**
	* @return The MinorLinkerVersion value from the PE header.
	**/
	template<int x>
	byte PeHeaderT<x>::getMinorLinkerVersion() const
	{
		return m_inthHeader.OptionalHeader.MinorLinkerVersion;
	}

	/**
	* @return The SizeOfCode value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfCode() const
	{
		return m_inthHeader.OptionalHeader.SizeOfCode;
	}

	/**
	* @return The SizeOfInitializedData value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfInitializedData() const
	{
		return m_inthHeader.OptionalHeader.SizeOfInitializedData;
	}

	/**
	* @return The SizeOfUninitializedData value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfUninitializedData() const
	{
		return m_inthHeader.OptionalHeader.SizeOfUninitializedData;
	}

	/**
	* @return The AddressOfEntryPoint value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getAddressOfEntryPoint() const
	{
		return m_inthHeader.OptionalHeader.AddressOfEntryPoint;
	}

	/**
	* @return The BaseOfCode value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getBaseOfCode() const
	{
		return m_inthHeader.OptionalHeader.BaseOfCode;
	}

	/**
	* @return The ImageBase value from the PE header.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::getImageBase() const
	{
		return m_inthHeader.OptionalHeader.ImageBase;
	}

	/**
	* @return The SectionAlignment value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSectionAlignment() const
	{
		return m_inthHeader.OptionalHeader.SectionAlignment;
	}

	/**
	* @return The FileAlignment value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getFileAlignment() const
	{
		return m_inthHeader.OptionalHeader.FileAlignment;
	}

	/**
	* @return The MajorOperatingSystemVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMajorOperatingSystemVersion() const
	{
		return m_inthHeader.OptionalHeader.MajorOperatingSystemVersion;
	}

	/**
	* @return The MinorOperatingSystemVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMinorOperatingSystemVersion() const
	{
		return m_inthHeader.OptionalHeader.MinorOperatingSystemVersion;
	}

	/**
	* @return The MajorImageVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMajorImageVersion() const
	{
		return m_inthHeader.OptionalHeader.MajorImageVersion;
	}

	/**
	* @return The MinorImageVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMinorImageVersion() const
	{
		return m_inthHeader.OptionalHeader.MinorImageVersion;
	}

	/**
	* @return The MajorSubsystemVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMajorSubsystemVersion() const
	{
		return m_inthHeader.OptionalHeader.MajorSubsystemVersion;
	}

	/**
	* @return The MinorSubsystemVersion value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getMinorSubsystemVersion() const
	{
		return m_inthHeader.OptionalHeader.MinorSubsystemVersion;
	}

	/**
	* @return The WinVersionValue value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getWin32VersionValue() const
	{
		return m_inthHeader.OptionalHeader.Win32VersionValue;
	}

	/**
	* @return The SizeOfImage value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfImage() const
	{
		return m_inthHeader.OptionalHeader.SizeOfImage;
	}

	/**
	* @return The SizeOfHeaders value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfHeaders() const
	{
		return m_inthHeader.OptionalHeader.SizeOfHeaders;
	}

	/**
	* @return The CheckSums value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getCheckSum() const
	{
		return m_inthHeader.OptionalHeader.CheckSum;
	}

	/**
	* @return The Subsystem value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getSubsystem() const
	{
		return m_inthHeader.OptionalHeader.Subsystem;
	}

	/**
	* @return The DllCharacteristics value from the PE header.
	**/
	template<int x>
	word PeHeaderT<x>::getDllCharacteristics() const
	{
		return m_inthHeader.OptionalHeader.DllCharacteristics;
	}

	/**
	* @return The SizeOfStackReserve value from the PE header.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8 PeHeaderT<x>::getSizeOfStackReserve() const
	{
		return m_inthHeader.OptionalHeader.SizeOfStackReserve;
	}

	/**
	* @return The SizeOfStackCommit value from the PE header.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8  PeHeaderT<x>::getSizeOfStackCommit() const
	{
		return m_inthHeader.OptionalHeader.SizeOfStackCommit;
	}

	/**
	* @return The SizeOfHeapReserve value from the PE header.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8  PeHeaderT<x>::getSizeOfHeapReserve() const
	{
		return m_inthHeader.OptionalHeader.SizeOfHeapReserve;
	}

	/**
	* @return The SizeOfHeapCommit value from the PE header.
	**/
	template<int x>
	typename FieldSizes<x>::VAR4_8  PeHeaderT<x>::getSizeOfHeapCommit() const
	{
		return m_inthHeader.OptionalHeader.SizeOfHeapCommit;
	}

	/**
	* @return The LoaderFlags value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getLoaderFlags() const
	{
		return m_inthHeader.OptionalHeader.LoaderFlags;
	}

	/**
	* @return The NumberOfRvaAndSizes value from the PE header.
	**/
	template<int x>
	dword PeHeaderT<x>::getNumberOfRvaAndSizes() const
	{
		return m_inthHeader.OptionalHeader.NumberOfRvaAndSizes;
	}

	template<int x>
	dword PeHeaderT<x>::calcNumberOfRvaAndSizes() const
	{
		return static_cast<dword>(m_inthHeader.dataDirectories.size());
	}

	/**
	* Returns the relative virtual address of the current file's export directory.
	* @return The Rva of the Export directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddExportRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	/**
	* Returns the size of the current file's export directory.
	* @return The sizeof the Export directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddExportSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}

	/**
	* Returns the relative virtual address of the current file's import directory.
	* @return The Rva of the Import directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddImportRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	}

	/**
	* Returns the size of the current file's import directory.
	* @return The size of the Import directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddImportSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	}

	/**
	* Returns the relative virtual address of the current file's resource directory.
	* @return The Rva of the Resource directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddResourceRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	}

	/**
	* Returns the size of the current file'resource resource directory.
	* @return The size of the Resource directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddResourceSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
	}

	/**
	* Returns the relative virtual address of the current file's exception directory.
	* @return The Rva of the Exception directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddExceptionRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
	}

	/**
	* Returns the size of the current file's exception directory.
	* @return The size of the Exception directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddExceptionSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
	}

	/**
	* Returns the relative virtual address of the current file's security directory.
	* @return The Rva of the Security directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddSecurityRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress;
	}

	/**
	* Returns the size of the current file's security directory.
	* @return The size of the Security directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddSecuritySize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
	}

	/**
	* Returns the relative virtual address of the current file's base reloc directory.
	* @return The Rva of the Base Reloc directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddBaseRelocRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	}

	/**
	* Returns the size of the current file's base reloc directory.
	* @return The size of the Base Reloc directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddBaseRelocSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
	}

	/**
	* Returns the relative virtual address of the current file's debug directory.
	* @return The Rva of the Debug directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddDebugRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	}

	/**
	* Returns the size of the current file's debug directory.
	* @return The size of the Debug directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddDebugSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	}

	/**
	* Returns the relative virtual address of the current file's Architecture directory.
	* @return The Rva of the Architecture directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddArchitectureRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].VirtualAddress;
	}

	/**
	* Returns the size of the current file's Architecture directory.
	* @return The size of the Architecture directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddArchitectureSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].Size;
	}

	/**
	* Returns the relative virtual address of the current file's global ptr directory.
	* @return The Rva of the GlobalPtr directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddGlobalPtrRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress;
	}

	/**
	* Returns the size of the current file's global ptr directory.
	* @return The size of the GlobalPtr directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddGlobalPtrSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size;
	}

	/**
	* Returns the relative virtual address of the current file's TLS directory.
	* @return The Rva of the Tls directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddTlsRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
	}

	/**
	* Returns the size of the current file's TLS directory.
	* @return The size of the Tls directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddTlsSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_TLS].Size;
	}

	/**
	* Returns the relative virtual address of the current file's load config directory.
	* @return The Rva of the LoadConfig directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddLoadConfigRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
	}

	/**
	* Returns the size of the current file's load config directory.
	* @return The size of the LoadConfig directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddLoadConfigSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
	}

	/**
	* Returns the relative virtual address of the current file's bound import directory.
	* @return The Rva of the BoundImport directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddBoundImportRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
	}

	/**
	* Returns the size of the current file's bound import directory.
	* @return The size of the BoundImport directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddBoundImportSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
	}

	/**
	* Returns the relative virtual address of the current file's IAT directory.
	* @return The Rva of the IAT directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddIatRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
	}

	/**
	* Returns the size of the current file's IAT directory.
	* @return The size of the IAT directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddIatSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IAT].Size;
	}

	/**
	* Returns the relative virtual address of the current file's Delay Import directory.
	* @return The Rva of the DelayImport directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddDelayImportRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
	}

	/**
	* Returns the size of the current file's Delay Import directory.
	* @return The size of the DelayImport directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddDelayImportSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size;
	}

	/**
	* Returns the relative virtual address of the current file's COM Descriptor directory.
	* @return The Rva of the COM Descriptor directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddComHeaderRva() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
	}
	
	/**
	* Returns the size of the current file's COM Descriptor directory.
	* @return The Rva of the COM Descriptor directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getIddComHeaderSize() const
	{
		return m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
	}

	/**
	* Returns the relative virtual address of an image directory.
	* @param dwDirectory The identifier of an image directory.
	* @return The Rva of the image directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getImageDataDirectoryRva(dword dwDirectory) const
	{
		return m_inthHeader.dataDirectories[dwDirectory].VirtualAddress;
	}

	template<int x>
	void PeHeaderT<x>::setImageDataDirectoryRva(dword dwDirectory, dword value)
	{
		m_inthHeader.dataDirectories[dwDirectory].VirtualAddress = value;
	}

	/**
	* Returns the size of an image directory.
	* @param dwDirectory The identifier of an image directory.
	* @return The size of the image directory.
	**/
	template<int x>
	dword PeHeaderT<x>::getImageDataDirectorySize(dword dwDirectory) const
	{
		return m_inthHeader.dataDirectories[dwDirectory].Size;
	}

	template<int x>
	void PeHeaderT<x>::setImageDataDirectorySize(dword dwDirectory, dword value)
	{
		m_inthHeader.dataDirectories[dwDirectory].Size = value;
	}

	/**
	* Returns the name of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The name of the section.
	**/
	template<int x>
	std::string PeHeaderT<x>::getSectionName(word wSectionnr) const
	{
		std::string sectionName = "";
		
		for (unsigned int i=0;i<sizeof(m_vIsh[wSectionnr].Name);i++)
		{
			if (m_vIsh[wSectionnr].Name[i]) sectionName += m_vIsh[wSectionnr].Name[i];
		}
		
		return sectionName;
	}
	
	/**
	* Returns the virtual size of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The virtual size of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getVirtualSize(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].VirtualSize;
	}
	
	/**
	* Returns the relative virtual address of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The Rva of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getVirtualAddress(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].VirtualAddress;
	}
	
	/**
	* Returns the size of raw data of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The size of raw data of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getSizeOfRawData(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].SizeOfRawData;
	}

	/**
	* Returns the file offset of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The file offset of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getPointerToRawData(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].PointerToRawData;
	}

	/**
	* Returns the pointer to relocations of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The pointer to relocations of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getPointerToRelocations(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].PointerToRelocations;
	}

	/**
	* Returns the poiner to line numbers of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The pointer to line numbers of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getPointerToLinenumbers(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].PointerToLinenumbers;
	}

	/**
	* Returns the number of relocations of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The number of relocations of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getNumberOfRelocations(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].NumberOfRelocations;
	}

	/**
	* Returns the number of line numbers of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The number of line numbers of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getNumberOfLinenumbers(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].NumberOfLinenumbers;
	}

	/**
	* Returns the characteristics of the section which is specified by the parameter wSectionnr.
	* @param wSectionnr Index of the section.
	* @return The characteristics of the section.
	**/
	template<int x>
	dword PeHeaderT<x>::getCharacteristics(word wSectionnr) const
	{
		return m_vIsh[wSectionnr].Characteristics;
	}

	/**
	* Changes the file's Nt signature.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNtSignature(dword dwValue)
	{
		m_inthHeader.Signature = dwValue;
	}

	/**
	* Changes the file's Machine.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMachine(word wValue)
	{
		m_inthHeader.FileHeader.Machine = wValue;
	}

	/**
	* Changes the number of sections.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNumberOfSections(word wValue)
	{
		m_inthHeader.FileHeader.NumberOfSections = wValue;
	}

	/**
	* Changes the file's TimeDateStamp.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setTimeDateStamp(dword dwValue)
	{
		m_inthHeader.FileHeader.TimeDateStamp = dwValue;
	}

	/**
	* Changes the file's PointerToSymbolTable.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setPointerToSymbolTable(dword dwValue)
	{
		m_inthHeader.FileHeader.PointerToSymbolTable = dwValue;
	}

	/**
	* Changes the file's NumberOfSymbols.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNumberOfSymbols(dword dwValue)
	{
		m_inthHeader.FileHeader.NumberOfSymbols = dwValue;
	}

	/**
	* Changes the file's SizeOfOptionalHeader.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfOptionalHeader(word wValue)
	{
		m_inthHeader.FileHeader.SizeOfOptionalHeader = wValue;
	}

	/**
	* Changes the file's Characteristics.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setCharacteristics(word wValue)
	{
		m_inthHeader.FileHeader.Characteristics = wValue;
	}

	/**
	* Changes the file's Magic.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMagic(word wValue)
	{
		m_inthHeader.OptionalHeader.Magic = wValue;
	}

	/**
	* Changes the file's MajorLinkerVersion.
	* @param bValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMajorLinkerVersion(byte bValue)
	{
		m_inthHeader.OptionalHeader.MajorLinkerVersion = bValue;
	}

	/**
	* Changes the file's MinorLinkerVersion.
	* @param bValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMinorLinkerVersion(byte bValue)
	{
		m_inthHeader.OptionalHeader.MinorLinkerVersion = bValue;
	}

	/**
	* Changes the file's SizeOfCode.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfCode(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfCode = dwValue;
	}

	/**
	* Changes the file's SizeOfInitializedData.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfInitializedData(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfInitializedData = dwValue;
	}

	/**
	* Changes the file's SizeOfUninitializedData.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfUninitializedData(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfUninitializedData = dwValue;
	}

	/**
	* Changes the file's AddressOfEntryPoint.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setAddressOfEntryPoint(dword dwValue)
	{
		m_inthHeader.OptionalHeader.AddressOfEntryPoint = dwValue;
	}

	/**
	* Changes the file's BaseOfCode.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setBaseOfCode(dword dwValue)
	{
		m_inthHeader.OptionalHeader.BaseOfCode = dwValue;
	}

	/**
	* Changes the file's ImageBase.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setImageBase(typename FieldSizes<x>::VAR4_8 dwValue)
	{
		m_inthHeader.OptionalHeader.ImageBase = dwValue;
	}

	/**
	* Changes the file's SectionAlignment.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSectionAlignment(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SectionAlignment = dwValue;
	}

	/**
	* Changes the file's FileAlignment.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setFileAlignment(dword dwValue)
	{
		m_inthHeader.OptionalHeader.FileAlignment = dwValue;
	}

	/**
	* Changes the file's MajorOperatingSystemVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMajorOperatingSystemVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MajorOperatingSystemVersion = wValue;
	}

	/**
	* Changes the file's MinorOperatingSystemVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMinorOperatingSystemVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MinorOperatingSystemVersion = wValue;
	}

	/**
	* Changes the file's MajorImageVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMajorImageVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MajorImageVersion = wValue;
	}

	/**
	* Changes the file's MinorImageVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMinorImageVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MinorImageVersion = wValue;
	}

	/**
	* Changes the file's MajorSubsystemVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMajorSubsystemVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MajorSubsystemVersion = wValue;
	}

	/**
	* Changes the file's MinorSubsystemVersion.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setMinorSubsystemVersion(word wValue)
	{
		m_inthHeader.OptionalHeader.MinorSubsystemVersion = wValue;
	}

	/**
	* Changes the file's Win32VersionValue.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setWin32VersionValue(dword dwValue)
	{
		m_inthHeader.OptionalHeader.Win32VersionValue = dwValue;
	}

	/**
	* Changes the file's SizeOfImage.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfImage(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfImage = dwValue;
	}

	/**
	* Changes the file's SizeOfHeaders.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfHeaders(dword dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfHeaders = dwValue;
	}

	/**
	* Changes the file's CheckSum.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setCheckSum(dword dwValue)
	{
		m_inthHeader.OptionalHeader.CheckSum = dwValue;
	}

	/**
	* Changes the file's Subsystem.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSubsystem(word wValue)
	{
		m_inthHeader.OptionalHeader.Subsystem = wValue;
	}

	/**
	* Changes the file's DllCharacteristics.
	* @param wValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setDllCharacteristics(word wValue)
	{
		m_inthHeader.OptionalHeader.DllCharacteristics = wValue;
	}

	/**
	* Changes the file's SizeOfStackReserve.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfStackReserve(typename FieldSizes<x>::VAR4_8 dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfStackReserve = dwValue;
	}

	/**
	* Changes the file's SizeOfStackCommit.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfStackCommit(typename FieldSizes<x>::VAR4_8 dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfStackCommit = dwValue;
	}

	/**
	* Changes the file's SizeOfHeapReserve.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfHeapReserve(typename FieldSizes<x>::VAR4_8 dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfHeapReserve = dwValue;
	}

	/**
	* Changes the file's SizeOfHeapCommit.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfHeapCommit(typename FieldSizes<x>::VAR4_8 dwValue)
	{
		m_inthHeader.OptionalHeader.SizeOfHeapCommit = dwValue;
	}

	/**
	* Changes the file's LoaderFlags.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setLoaderFlags(dword dwValue)
	{
		m_inthHeader.OptionalHeader.LoaderFlags = dwValue;
	}

	/**
	* Changes the file's NumberOfRvaAndSizes.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNumberOfRvaAndSizes(dword dwValue)
	{
		m_inthHeader.OptionalHeader.NumberOfRvaAndSizes = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddDebugRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddDebugSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddDelayImportRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddDelayImportSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddExceptionRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddExceptionSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddGlobalPtrRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddGlobalPtrSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddIatRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddIatSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IAT].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddLoadConfigRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddLoadConfigSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddResourceRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddResourceSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddSecurityRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddSecuritySize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_SECURITY].Size = dwValue;
	}
	
	template<int x>
	void PeHeaderT<x>::setIddTlsRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddTlsSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_TLS].Size = dwValue;
	}
	
	/**
	* Changes the rva of the file's export directory.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setIddExportRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = dwValue;
	}

	/**
	* Changes the size of the file's export directory.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setIddExportSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_EXPORT].Size = dwValue;
	}

	template<int x>
	void PeHeaderT<x>::setIddBaseRelocRva(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = value;
	}

	template<int x>
	void PeHeaderT<x>::setIddBaseRelocSize(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = value;
	}

	template<int x>
	void PeHeaderT<x>::setIddArchitectureRva(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].VirtualAddress = value;
	}

	template<int x>
	void PeHeaderT<x>::setIddArchitectureSize(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE].Size = value;
	}

	template<int x>
	void PeHeaderT<x>::setIddComHeaderRva(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = value;
	}

	template<int x>
	void PeHeaderT<x>::setIddComHeaderSize(dword value)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = value;
	}

	/**
	* Changes the rva of the file's import directory.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setIddImportRva(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = dwValue;
	}

	/**
	* Changes the size of the file's import directory.
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setIddImportSize(dword dwValue)
	{
		m_inthHeader.dataDirectories[PELIB_IMAGE_DIRECTORY_ENTRY_IMPORT].Size = dwValue;
	}

	/**
	* Changes the name of a section.
	* @param wSectionnr Identifier of the section
	* @param strName New name.
	**/
	template<int x>
	void PeHeaderT<x>::setSectionName(word wSectionnr, std::string strName)
	{
		strncpy(reinterpret_cast<char*>(m_vIsh[wSectionnr].Name), strName.c_str(), sizeof(m_vIsh[wSectionnr].Name));
	}

	/**
	* Changes the virtual size of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setVirtualSize(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].VirtualSize = dwValue;
	}

	/**
	* Changes the virtual address of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setVirtualAddress(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].VirtualAddress = dwValue;
	}

	/**
	* Changes the size of raw data of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setSizeOfRawData(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].SizeOfRawData = dwValue;
	}

	/**
	* Changes the size of raw data of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setPointerToRawData(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].PointerToRawData = dwValue;
	}

	/**
	* Changes the pointer to relocations of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setPointerToRelocations(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].PointerToRelocations = dwValue;
	}

	/**
	* Changes the pointer to line numbers of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setPointerToLinenumbers(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].PointerToLinenumbers = dwValue;
	}

	/**
	* Changes the number of relocations of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNumberOfRelocations(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].NumberOfRelocations = dwValue;
	}

	/**
	* Changes the number of line numbers of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setNumberOfLinenumbers(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].NumberOfLinenumbers = dwValue;
	}

	/**
	* Changes the characteristics of a section.
	* @param wSectionnr Identifier of the section
	* @param dwValue New value.
	**/
	template<int x>
	void PeHeaderT<x>::setCharacteristics(word wSectionnr, dword dwValue)
	{
		m_vIsh[wSectionnr].Characteristics = dwValue;
	}

}

#endif
