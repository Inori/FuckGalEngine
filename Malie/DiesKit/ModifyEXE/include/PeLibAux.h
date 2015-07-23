/*
* PeLibAux.cpp - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef PELIBAUX_H
#define PELIBAUX_H

//#include "PeLibInc.h"
//#include "PeHeader.h"
#include "buffer/OutputBuffer.h"
#include "buffer/InputBuffer.h"
//#include "buffer/ResTree.h"
#include <numeric>
#include <limits>

namespace PeLib
{
	enum errorCodes
	{
		NO_ERROR = 0,
		ERROR_OPENING_FILE = -1,
		ERROR_INVALID_FILE = -2,
		ERROR_TOO_MANY_SECTIONS = -3,
		ERROR_NOT_ENOUGH_SPACE = -4,
		ERROR_NO_FILE_ALIGNMENT = -5,
		ERROR_NO_SECTION_ALIGNMENT = -6,
		ERROR_ENTRY_NOT_FOUND = -7,
		ERROR_DUPLICATE_ENTRY = -8,
		ERROR_DIRECTORY_DOES_NOT_EXIST = -9
	};

	class PeFile;
	
// It's necessary to make sure that a byte has 8 bits and that the platform has a 8 bit type,
// a 16bit type and a bit type. That's because binary PE files are pretty picky about their
// structure.

	#if CHAR_BIT == 8
		#if UCHAR_MAX == 255
			typedef unsigned char byte;
	//		typedef std::bitset<8> byte;
		#else
			#error You need to change some typedefs (Code: 8). Please read the PeLib documentation.
		#endif
	
		#if USHRT_MAX == 65535U
			typedef unsigned short word;
	//		typedef std::bitset<16> word;
		#else
			#error You need to change some typedefs (Code: 16). Please read the PeLib documentation.
		#endif
	
		#if UINT_MAX == 4294967295UL
			typedef unsigned int dword;
	//		typedef std::bitset<32> dword;
		#else
			#error You need to change some typedefs (Code: 32). Please read the PeLib documentation.
		#endif

		typedef unsigned long long qword;
	
//		#if ULLONG_MAX == 18446744073709551615
//			typedef unsigned long long qword;
//		#else
//			#error You need to change some typedefs (Code: 32). Please read the PeLib documentation.
//		#endif
	#else
		#error You need to change some typedefs. Please read the PeLib documentation.
	#endif


/*	enum bits {BITS_BYTE = 8, BITS_WORD = 16, BITS_DWORD = 32};

	template<bits value>
	class DataType
	{
		private:
		  std::bitset<value> bsValue;
		  unsigned long ulValue;
	  
		public:
		  void operator=(unsigned long ulValue)
		  {
			bsValue = ulValue;
		  }

		  operator unsigned long() const
		  {
			return bsValue.to_ulong();
		  }
		  
		  const int operator&()
		  {
		  	ulValue = bsValue;
		  	return ulValue;	
		  }

	};

	typedef DataType<BITS_BYTE> byte;
	typedef DataType<BITS_WORD> word;
	typedef DataType<BITS_DWORD> dword;
*/

	enum {PEFILE32 = 32,
		  PEFILE64 = 64,
		  PEFILE_UNKNOWN = 0};

	enum {BoundImportDirectoryId = 1,
	      ComHeaderDirectoryId,
	      ExportDirectoryId,
	      IatDirectoryId,
	      ImportDirectoryId,
	      MzHeaderId,
	      PeHeaderId,
	      RelocationsId,
	      PeFileId,
	      ResourceDirectoryId,
	      DebugDirectoryId,
	      TlsDirectoryId
	};

	const word PELIB_IMAGE_DOS_SIGNATURE = 0x5A4D;
	
	const dword PELIB_IMAGE_NT_SIGNATURE = 0x00004550;

	template<int bits>
	struct PELIB_IMAGE_ORDINAL_FLAGS;
	
	template<>
	struct PELIB_IMAGE_ORDINAL_FLAGS<32>
	{
		static const dword IMAGE_ORDINAL_FLAG = 0x80000000;
	};
	
	template<>
	struct PELIB_IMAGE_ORDINAL_FLAGS<64>
	{
		static const qword IMAGE_ORDINAL_FLAG;
	};

	const unsigned long PELIB_IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;

	const unsigned long PELIB_IMAGE_RESOURCE_NAME_IS_STRING = 0x80000000;

	const unsigned long PELIB_IMAGE_RESOURCE_DATA_IS_DIRECTORY = 0x80000000;

	enum
	{
		PELIB_IMAGE_DIRECTORY_ENTRY_EXPORT,		// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_IMPORT,		// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE,		// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_EXCEPTION,
		PELIB_IMAGE_DIRECTORY_ENTRY_SECURITY,
		PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC,	// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_DEBUG,
		PELIB_IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,
		PELIB_IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
		PELIB_IMAGE_DIRECTORY_ENTRY_TLS,
		PELIB_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
		PELIB_IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,	// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_IAT,		// OK
		PELIB_IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
		PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
	};

	enum
	{
		PELIB_IMAGE_SCN_TYPE_NO_PAD		= 0x00000008,
		PELIB_IMAGE_SCN_CNT_CODE		   = 0x00000020,
		PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA       = 0x00000040,
		PELIB_IMAGE_SCN_CNT_UNINITIALIZED_DATA     = 0x00000080,
		PELIB_IMAGE_SCN_LNK_OTHER		  = 0x00000100,
		PELIB_IMAGE_SCN_LNK_INFO		   = 0x00000200,
		PELIB_IMAGE_SCN_LNK_REMOVE		 = 0x00000800,
		PELIB_IMAGE_SCN_LNK_COMDAT		 = 0x00001000,
		PELIB_IMAGE_SCN_NO_DEFER_SPEC_EXC	  = 0x00004000,
		PELIB_IMAGE_SCN_GPREL		      = 0x00008000,
		PELIB_IMAGE_SCN_MEM_FARDATA		= 0x00008000,
		PELIB_IMAGE_SCN_MEM_PURGEABLE	      = 0x00020000,
		PELIB_IMAGE_SCN_MEM_16BIT		  = 0x00020000,
		PELIB_IMAGE_SCN_MEM_LOCKED		 = 0x00040000,
		PELIB_IMAGE_SCN_MEM_PRELOAD		= 0x00080000,
		PELIB_IMAGE_SCN_ALIGN_1BYTES	       = 0x00100000,
		PELIB_IMAGE_SCN_ALIGN_2BYTES	       = 0x00200000,
		PELIB_IMAGE_SCN_ALIGN_4BYTES	       = 0x00300000,
		PELIB_IMAGE_SCN_ALIGN_8BYTES	       = 0x00400000,
		PELIB_IMAGE_SCN_ALIGN_16BYTES	      = 0x00500000,
		PELIB_IMAGE_SCN_ALIGN_BYTES		= 0x00600000,
		PELIB_IMAGE_SCN_ALIGN_64BYTES	      = 0x00700000,
		PELIB_IMAGE_SCN_ALIGN_128BYTES	     = 0x00800000,
		PELIB_IMAGE_SCN_ALIGN_256BYTES	     = 0x00900000,
		PELIB_IMAGE_SCN_ALIGN_512BYTES	     = 0x00A00000,
		PELIB_IMAGE_SCN_ALIGN_1024BYTES	    = 0x00B00000,
		PELIB_IMAGE_SCN_ALIGN_2048BYTES	    = 0x00C00000,
		PELIB_IMAGE_SCN_ALIGN_4096BYTES	    = 0x00D00000,
		PELIB_IMAGE_SCN_ALIGN_8192BYTES	    = 0x00E00000,
		PELIB_IMAGE_SCN_LNK_NRELOC_OVFL	    = 0x01000000,
		PELIB_IMAGE_SCN_MEM_DISCARDABLE	    = 0x02000000,
		PELIB_IMAGE_SCN_MEM_NOT_CACHED	     = 0x04000000,
		PELIB_IMAGE_SCN_MEM_NOT_PAGED	      = 0x08000000,
		PELIB_IMAGE_SCN_MEM_SHARED		 = 0x10000000,
		PELIB_IMAGE_SCN_MEM_EXECUTE		= 0x20000000,
		PELIB_IMAGE_SCN_MEM_READ		   = 0x40000000,
		PELIB_IMAGE_SCN_MEM_WRITE		  = 0x80000000
	};

	enum
	{
		PELIB_IMAGE_FILE_MACHINE_UNKNOWN	   = 0,
		PELIB_IMAGE_FILE_MACHINE_I386	      = 0x014c,
		PELIB_IMAGE_FILE_MACHINE_R3000	     = 0x0162,
		PELIB_IMAGE_FILE_MACHINE_R4000	     = 0x0166,
		PELIB_IMAGE_FILE_MACHINE_R10000	    = 0x0168,
		PELIB_IMAGE_FILE_MACHINE_WCEMIPSV2	 = 0x0169,
		PELIB_IMAGE_FILE_MACHINE_ALPHA	     = 0x0184,
		PELIB_IMAGE_FILE_MACHINE_POWERPC	   = 0x01F0,
		PELIB_IMAGE_FILE_MACHINE_SH3	       = 0x01a2,
		PELIB_IMAGE_FILE_MACHINE_SH3E	      = 0x01a4,
		PELIB_IMAGE_FILE_MACHINE_SH4	       = 0x01a6,
		PELIB_IMAGE_FILE_MACHINE_ARM	       = 0x01c0,
		PELIB_IMAGE_FILE_MACHINE_THUMB	     = 0x01c2,
		PELIB_IMAGE_FILE_MACHINE_IA64	      = 0x0200,
		PELIB_IMAGE_FILE_MACHINE_MIPS16	    = 0x0266,
		PELIB_IMAGE_FILE_MACHINE_MIPSFPU	   = 0x0366,
		PELIB_IMAGE_FILE_MACHINE_MIPSFPU16	 = 0x0466,
		PELIB_IMAGE_FILE_MACHINE_ALPHA64	   = 0x0284,
		PELIB_IMAGE_FILE_MACHINE_AXP64	     = PELIB_IMAGE_FILE_MACHINE_ALPHA64
	};

	enum
	{
		PELIB_IMAGE_FILE_RELOCS_STRIPPED	   = 0x0001,
		PELIB_IMAGE_FILE_EXECUTABLE_IMAGE	  = 0x0002,
		PELIB_IMAGE_FILE_LINE_NUMS_STRIPPED	= 0x0004,
		PELIB_IMAGE_FILE_LOCAL_SYMS_STRIPPED       = 0x0008,
		PELIB_IMAGE_FILE_AGGRESIVE_WS_TRIM	 = 0x0010,
		PELIB_IMAGE_FILE_LARGE_ADDRESS_AWARE       = 0x0020,
		PELIB_IMAGE_FILE_BYTES_REVERSED_LO	 = 0x0080,
		PELIB_IMAGE_FILE_32BIT_MACHINE	     = 0x0100,
		PELIB_IMAGE_FILE_DEBUG_STRIPPED	    = 0x0200,
		PELIB_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   = 0x0400,
		PELIB_IMAGE_FILE_NET_RUN_FROM_SWAP	 = 0x0800,
		PELIB_IMAGE_FILE_SYSTEM		    = 0x1000,
		PELIB_IMAGE_FILE_DLL		       = 0x2000,
		PELIB_IMAGE_FILE_UP_SYSTEM_ONLY	    = 0x4000,
		PELIB_IMAGE_FILE_BYTES_REVERSED_HI	 = 0x8000
	};

	enum
	{
		PELIB_IMAGE_NT_OPTIONAL_HDR32_MAGIC      = 0x10b,
		PELIB_IMAGE_NT_OPTIONAL_HDR64_MAGIC      = 0x20b,
		PELIB_IMAGE_ROM_OPTIONAL_HDR_MAGIC       = 0x107
	};

	enum
	{
		PELIB_IMAGE_SUBSYSTEM_UNKNOWN	      = 0,
		PELIB_IMAGE_SUBSYSTEM_NATIVE	       = 1,
		PELIB_IMAGE_SUBSYSTEM_WINDOWS_GUI	  = 2,
		PELIB_IMAGE_SUBSYSTEM_WINDOWS_CUI	  = 3,
		PELIB_IMAGE_SUBSYSTEM_OS2_CUI	      = 5,
		PELIB_IMAGE_SUBSYSTEM_POSIX_CUI	    = 7,
		PELIB_IMAGE_SUBSYSTEM_NATIVE_WINDOWS       = 8,
		PELIB_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI       = 9
	};

	enum
	{
		PELIB_RT_CURSOR = 1,		// 1
		PELIB_RT_BITMAP,			// 2
		PELIB_RT_ICON,			// 3
		PELIB_RT_MENU,			// 4
		PELIB_RT_DIALOG,			// 5
		PELIB_RT_STRING,			// 6
		PELIB_RT_FONTDIR,			// 7
		PELIB_RT_FONT,			// 8
		PELIB_RT_ACCELERATOR,		// 9
		PELIB_RT_RCDATA,			// 10
		PELIB_RT_MESSAGETABLE,	// 11
		PELIB_RT_GROUP_CURSOR,	// 12
		PELIB_RT_GROUP_ICON = 14,	// 14
		PELIB_RT_VERSION = 16,
		PELIB_RT_DLGINCLUDE,
		PELIB_RT_PLUGPLAY = 19,
		PELIB_RT_VXD,
		PELIB_RT_ANICURSOR,
		PELIB_RT_ANIICON,
		PELIB_RT_HTML,
		PELIB_RT_MANIFEST
	};
	
	template<typename T>
	unsigned int accumulate(unsigned int size, const T& v)
	{
		return size + v.size();
	}

	
	struct PELIB_IMAGE_DOS_HEADER
	{
		word   e_magic;
		word   e_cblp;
		word   e_cp;
		word   e_crlc;
		word   e_cparhdr;
		word   e_minalloc;
		word   e_maxalloc;
		word   e_ss;
		word   e_sp;
		word   e_csum;
		word   e_ip;
		word   e_cs;
		word   e_lfarlc;
		word   e_ovno;
		word   e_res[4];
		word   e_oemid;
		word   e_oeminfo;
		word   e_res2[10];
		dword   e_lfanew;
		
		PELIB_IMAGE_DOS_HEADER();
		
		static inline unsigned int size() {return 64;}
	};

	struct PELIB_IMAGE_FILE_HEADER
	{
		word	Machine;
		word    NumberOfSections;
		dword   TimeDateStamp;
		dword   PointerToSymbolTable;
		dword   NumberOfSymbols;
		word    SizeOfOptionalHeader;
		word    Characteristics;
		
		PELIB_IMAGE_FILE_HEADER()
		{
			Machine = 0;
			NumberOfSections = 0;
			TimeDateStamp = 0;
			PointerToSymbolTable = 0;
			NumberOfSymbols = 0;
			SizeOfOptionalHeader = 0;
			Characteristics = 0;
		}
		
		static inline unsigned int size() {return 20;}
	};

	struct PELIB_IMAGE_DATA_DIRECTORY
	{
		dword   VirtualAddress;
		dword   Size;
		
		PELIB_IMAGE_DATA_DIRECTORY()
		{
			VirtualAddress = 0;
			Size = 0;
		}
		
		static inline unsigned int size() {return 8;}
	};

	template<int>
	struct FieldSizes;
	
	template<>
	struct FieldSizes<32>
	{
		typedef dword VAR4_8;
	};
	
	template<>
	struct FieldSizes<64>
	{
		typedef qword VAR4_8;
	};
	
	template<int x>
	struct PELIB_IMAGE_OPTIONAL_HEADER_BASE
	{
		typedef typename FieldSizes<x>::VAR4_8 VAR4_8;
		
		word    Magic;
		byte    MajorLinkerVersion;
		byte    MinorLinkerVersion;
		dword   SizeOfCode;
		dword   SizeOfInitializedData;
		dword   SizeOfUninitializedData;
		dword   AddressOfEntryPoint;
		dword   BaseOfCode;
		dword   BaseOfData;
		VAR4_8  ImageBase;
		dword   SectionAlignment;
		dword   FileAlignment;
		word    MajorOperatingSystemVersion;
		word    MinorOperatingSystemVersion;
		word    MajorImageVersion;
		word    MinorImageVersion;
		word    MajorSubsystemVersion;
		word    MinorSubsystemVersion;
		dword   Win32VersionValue;
		dword   SizeOfImage;
		dword   SizeOfHeaders;
		dword   CheckSum;
		word    Subsystem;
		word    DllCharacteristics;
		VAR4_8  SizeOfStackReserve;
		VAR4_8  SizeOfStackCommit;
		VAR4_8  SizeOfHeapReserve;
		VAR4_8  SizeOfHeapCommit;
		dword   LoaderFlags;
		dword   NumberOfRvaAndSizes;
//		PELIB_IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
		
		PELIB_IMAGE_OPTIONAL_HEADER_BASE();
	};
	
	template<int x>
	PELIB_IMAGE_OPTIONAL_HEADER_BASE<x>::PELIB_IMAGE_OPTIONAL_HEADER_BASE()
	{
		Magic = 0;
		MajorLinkerVersion = 0;
		MinorLinkerVersion = 0;
		SizeOfCode = 0;
		SizeOfInitializedData = 0;
		SizeOfUninitializedData = 0;
		AddressOfEntryPoint = 0;
		BaseOfCode = 0;
//		BaseOfData = 0;
		ImageBase = 0;
		SectionAlignment = 0;
		FileAlignment = 0;
		MajorOperatingSystemVersion = 0;
		MinorOperatingSystemVersion = 0;
		MajorImageVersion = 0;
		MinorImageVersion = 0;
		MajorSubsystemVersion = 0;
		MinorSubsystemVersion = 0;
		Win32VersionValue = 0;
		SizeOfImage = 0;
		SizeOfHeaders = 0;
		CheckSum = 0;
		Subsystem = 0;
		DllCharacteristics = 0;
		SizeOfStackReserve = 0;
		SizeOfStackCommit = 0;
		SizeOfHeapReserve = 0;
		SizeOfHeapCommit = 0;
		LoaderFlags = 0;
		NumberOfRvaAndSizes = 0;
	}

	template<int>
	struct PELIB_IMAGE_OPTIONAL_HEADER;
	
	template<>
	struct PELIB_IMAGE_OPTIONAL_HEADER<32> : public PELIB_IMAGE_OPTIONAL_HEADER_BASE<32>
	{
		dword  BaseOfData;
		
		static inline unsigned int size() {return 224 - 0x10 * 8;}
	};
	
	template<>
	struct PELIB_IMAGE_OPTIONAL_HEADER<64> : public PELIB_IMAGE_OPTIONAL_HEADER_BASE<64>
	{
		static inline unsigned int size() {return 240 - 0x10 * 8;}
	};
	
	template<int x>
	struct PELIB_IMAGE_NT_HEADERS
	{
		dword Signature;
		PELIB_IMAGE_FILE_HEADER FileHeader;
		PELIB_IMAGE_OPTIONAL_HEADER<x> OptionalHeader;
		std::vector<PELIB_IMAGE_DATA_DIRECTORY> dataDirectories;
		
		unsigned int size() const
		{
			return sizeof(dword)
				+ PELIB_IMAGE_FILE_HEADER::size()
				+ PELIB_IMAGE_OPTIONAL_HEADER<x>::size()
				+ static_cast<unsigned int>(dataDirectories.size()) * PELIB_IMAGE_DATA_DIRECTORY::size();
		}
		
		PELIB_IMAGE_NT_HEADERS()
		{
			Signature = 0;
		}
	};

	const unsigned int PELIB_IMAGE_SIZEOF_SHORT_NAME  = 8;

	struct PELIB_IMAGE_SECTION_HEADER
	{
		byte Name[PELIB_IMAGE_SIZEOF_SHORT_NAME];
		dword	VirtualSize;
		dword   VirtualAddress;
		dword   SizeOfRawData;
		dword   PointerToRawData;
		dword   PointerToRelocations;
		dword   PointerToLinenumbers;
		word    NumberOfRelocations;
		word    NumberOfLinenumbers;
		dword   Characteristics;
		
		PELIB_IMAGE_SECTION_HEADER()
		{
			VirtualSize = 0;
			VirtualAddress = 0;
			SizeOfRawData = 0;
			PointerToRawData = 0;
			PointerToRelocations = 0;
			PointerToLinenumbers = 0;
			NumberOfRelocations = 0;
			NumberOfLinenumbers = 0;
			Characteristics = 0;
		}
		
		static inline unsigned int size() {return 40;}
		bool biggerFileOffset(const PELIB_IMAGE_SECTION_HEADER& ish) const;
		bool biggerVirtualAddress(const PELIB_IMAGE_SECTION_HEADER& ish) const;
	};

	template<int bits>
	struct PELIB_IMAGE_THUNK_DATA
	{
		typename FieldSizes<bits>::VAR4_8 Ordinal;

		PELIB_IMAGE_THUNK_DATA()
		{
			Ordinal = 0;
		}
		
		static inline unsigned int size() {return 4;}
	};

	struct PELIB_IMAGE_IMPORT_DESCRIPTOR
	{
		dword   OriginalFirstThunk;
		dword   TimeDateStamp;
		dword   ForwarderChain;
		dword   Name;
		dword   FirstThunk;
		
		PELIB_IMAGE_IMPORT_DESCRIPTOR()
		{
			OriginalFirstThunk = 0;
			TimeDateStamp = 0;
			ForwarderChain = 0;
			Name = 0;
			FirstThunk = 0;
		}
		
		static inline unsigned int size() {return 20;}
	};
	
	struct PELIB_IMAGE_EXPORT_DIRECTORY
	{
		dword   Characteristics;
		dword   TimeDateStamp;
		word    MajorVersion;
		word    MinorVersion;
		dword   Name;
		dword   Base;
		dword   NumberOfFunctions;
		dword   NumberOfNames;
		dword   AddressOfFunctions;
		dword   AddressOfNames;
		dword   AddressOfNameOrdinals;
		
		PELIB_IMAGE_EXPORT_DIRECTORY()
		{
			Characteristics = 0;
			TimeDateStamp = 0;
			MajorVersion = 0;
			MinorVersion = 0;
			Name = 0;
			Base = 0;
			NumberOfFunctions = 0;
			NumberOfNames = 0;
			AddressOfFunctions = 0;
			NumberOfNames = 0;
			AddressOfNameOrdinals = 0;
		}
		
		static inline unsigned int size() {return 40;}
	};
	
	struct PELIB_IMAGE_BOUND_IMPORT_DESCRIPTOR
	{
		dword   TimeDateStamp;
		word    OffsetModuleName;
		word    NumberOfModuleForwarderRefs;
		
		PELIB_IMAGE_BOUND_IMPORT_DESCRIPTOR()
		{
			TimeDateStamp = 0;
			OffsetModuleName = 0;
			NumberOfModuleForwarderRefs = 0;
		}

		static unsigned int size()
		{
			return 8;
		}
	};
	
	// Stores all necessary information about a BoundImport field.
	struct PELIB_IMAGE_BOUND_DIRECTORY
	{
		PELIB_IMAGE_BOUND_IMPORT_DESCRIPTOR ibdDescriptor; ///< Information about the imported file.
		std::string strModuleName; ///< Name of the imported file.
		std::vector<PELIB_IMAGE_BOUND_DIRECTORY> moduleForwarders;

		// Will be used in std::find_if
		// Passing by-reference not possible (see C++ Standard Core Language Defect Reports, Revision 29, Issue 106)
		/// Compares the passed filename to the struct's filename.
		bool equal(const std::string strModuleName) const;
		
		unsigned int size() const;
	};

	struct PELIB_EXP_FUNC_INFORMATION
	{
		dword addroffunc;
		dword addrofname;
		word ordinal;
		std::string funcname;
		
		PELIB_EXP_FUNC_INFORMATION();

		bool equal(const std::string strFunctionName) const;
		inline unsigned int size() const
		{
			unsigned int uiSize = 4;
			if (addroffunc) uiSize += 2;// + 4;
			if (!funcname.empty()) uiSize += 4 + (unsigned int)funcname.size() + 1;
			return uiSize;
		}
	};

	struct PELIB_IMAGE_RESOURCE_DIRECTORY
	{
		dword Characteristics;
		dword TimeDateStamp;
		word MajorVersion;
		word MinorVersion;
		word NumberOfNamedEntries;
		word NumberOfIdEntries;
		
		PELIB_IMAGE_RESOURCE_DIRECTORY();

		static inline unsigned int size() {return 16;}
	};

	struct PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY
	{
		dword Name;
		dword OffsetToData;
		PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY();
		static inline unsigned int size() {return 8;}
	};

	const unsigned int PELIB_IMAGE_SIZEOF_BASE_RELOCATION = 8;

	struct PELIB_IMG_RES_DIR_ENTRY
	{
		PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY irde;
		std::string wstrName;

		bool operator<(const PELIB_IMG_RES_DIR_ENTRY& first) const;

	};
	
	struct PELIB_IMAGE_BASE_RELOCATION
	{
		dword VirtualAddress;
		dword SizeOfBlock;
		
		PELIB_IMAGE_BASE_RELOCATION();
		static inline unsigned int size() {return 8;}
	};

	struct PELIB_IMAGE_COR20_HEADER
	{
		dword cb;
		word MajorRuntimeVersion;
		word MinorRuntimeVersion;
		PELIB_IMAGE_DATA_DIRECTORY MetaData;
		dword Flags;
		dword EntryPointToken;
		PELIB_IMAGE_DATA_DIRECTORY Resources;
		PELIB_IMAGE_DATA_DIRECTORY StrongNameSignature;
		PELIB_IMAGE_DATA_DIRECTORY CodeManagerTable;
		PELIB_IMAGE_DATA_DIRECTORY VTableFixups;
		PELIB_IMAGE_DATA_DIRECTORY ExportAddressTableJumps;
		PELIB_IMAGE_DATA_DIRECTORY ManagedNativeHeader;
		
		PELIB_IMAGE_COR20_HEADER();
		static inline unsigned int size() {return 72;}
	};

	// Used to store a file's export table.
	struct PELIB_IMAGE_EXP_DIRECTORY
	{
		/// The IMAGE_EXPORTED_DIRECTORY of a file's export table.
		PELIB_IMAGE_EXPORT_DIRECTORY ied;
		/// The original filename of current file.
		std::string name;
		std::vector<PELIB_EXP_FUNC_INFORMATION> functions;
		inline unsigned int size() const
		{
			return PELIB_IMAGE_EXPORT_DIRECTORY::size() + name.size() + 1 + 
			std::accumulate(functions.begin(), functions.end(), 0, accumulate<PELIB_EXP_FUNC_INFORMATION>);
		}
	};

	// Used for parsing a file's import table. It combines the function name, the hint
	// and the IMAGE_THUNK_DATA of an imported function.
	template<int bits>
	struct PELIB_THUNK_DATA
	{
		/// The IMAGE_THUNK_DATA struct of an imported function.
		PELIB_IMAGE_THUNK_DATA<bits> itd;
		/// The hint of an imported function.
		word hint;
		/// The function name of an imported function.
		std::string fname;
		
		bool equalHint(word wHint) const
		{
			return hint == wHint;
//			return itd.Ordinal == (wHint | IMAGE_ORDINAL_FLAGS<bits>::IMAGE_ORDINAL_FLAG);
		}
		
		bool equalFunctionName(std::string strFunctionName) const
		{
			return isEqualNc(fname, strFunctionName);
		}
		
		unsigned int size() const {return PELIB_IMAGE_THUNK_DATA<bits>::size() + fname.size() + 1 + sizeof(hint);}
	};

	// Used to store a file's import table. Every struct of this sort
	// can store import information of one DLL.
	template<int bits>
	struct PELIB_IMAGE_IMPORT_DIRECTORY
	{
		/// The IMAGE_IMPORT_DESCRIPTOR of an imported DLL.
		PELIB_IMAGE_IMPORT_DESCRIPTOR impdesc;
		/// The name of an imported DLL.
		std::string name;
		/// All original first thunk values of an imported DLL.
		std::vector<PELIB_THUNK_DATA<bits> > originalfirstthunk;
		/// All first thunk value of an imported DLL.
		std::vector<PELIB_THUNK_DATA<bits> > firstthunk;
		
//		bool operator==(std::string strFilename) const;
		inline unsigned int size() const
		{
			return PELIB_IMAGE_IMPORT_DESCRIPTOR::size() + name.size() + 1 + // descriptor + dllname
			std::accumulate(originalfirstthunk.begin(), originalfirstthunk.end(), 0, accumulate<PELIB_THUNK_DATA<bits> >) + // thunks (PeLib uses only one thunk)
			PELIB_IMAGE_THUNK_DATA<bits>::size(); // zero-termination
		}
		
		bool operator==(std::string strFilename) const
		{
			return isEqualNc(this->name, strFilename);
		}
	};

    struct PELIB_IMAGE_RESOURCE_DATA_ENTRY
    {
	    dword OffsetToData;
	    dword Size;
	    dword CodePage;
	    dword Reserved;

		static inline unsigned int size() {return 16;}
    };

    struct PELIB_IMAGE_RESOURCE_DATA
    {
	    PELIB_IMAGE_RESOURCE_DATA_ENTRY irdEntry;
	    std::vector<byte> vData;
    };

	struct IMG_BASE_RELOC
    {
		PELIB_IMAGE_BASE_RELOCATION ibrRelocation;
		std::vector<word> vRelocData;
	};
	
	struct PELIB_IMAGE_DEBUG_DIRECTORY
	{
		dword Characteristics;
		dword TimeDateStamp;
		word MajorVersion;
		word MinorVersion;
		dword Type;
		dword SizeOfData;
		dword AddressOfRawData;
		dword PointerToRawData;
		
		static unsigned int size() {return 28;}
	};
	
	struct PELIB_IMG_DEBUG_DIRECTORY
	{
		PELIB_IMAGE_DEBUG_DIRECTORY idd;
		std::vector<byte> data;
	};

	template<int bits>
	struct PELIB_IMAGE_TLS_DIRECTORY_BASE
	{
	    typename FieldSizes<bits>::VAR4_8 StartAddressOfRawData;
	    typename FieldSizes<bits>::VAR4_8 EndAddressOfRawData;
	    typename FieldSizes<bits>::VAR4_8 AddressOfIndex;
	    typename FieldSizes<bits>::VAR4_8 AddressOfCallBacks;
	    dword SizeOfZeroFill;
	    dword Characteristics;
	};    
	
	template<int bits>
	struct PELIB_IMAGE_TLS_DIRECTORY;// : public PELIB_IMAGE_TLS_DIRECTORY_BASE<bits>
	
	template<>
	struct PELIB_IMAGE_TLS_DIRECTORY<32> : public PELIB_IMAGE_TLS_DIRECTORY_BASE<32>
	{
//		enum {size = 24};
	    static unsigned int size(){return 24;}
	};

	template<>
	struct PELIB_IMAGE_TLS_DIRECTORY<64> : public PELIB_IMAGE_TLS_DIRECTORY_BASE<64>
	{
//		enum {size = 40};
	    static unsigned int size(){return 40;}
	};

	unsigned int fileSize(const std::string& filename);
	unsigned int fileSize(std::ifstream& file);
	unsigned int fileSize(std::ofstream& file);
	unsigned int fileSize(std::fstream& file);
	bool isEqualNc(const std::string& s1, const std::string& s2);
	unsigned int alignOffset(unsigned int uiOffset, unsigned int uiAlignment);
	
	/// Determines if a file is a 32bit or 64bit PE file.
	unsigned int getFileType(const std::string strFilename);
	
	/// Opens a PE file.
	PeFile* openPeFile(const std::string& strFilename);

  /*  enum MzHeader_Field {e_magic, e_cblp, e_cp, e_crlc, e_cparhdr, e_minalloc, e_maxalloc,
                        e_ss, e_sp, e_csum, e_ip, e_cs, e_lfarlc, e_ovno, e_res, e_oemid,
                        e_oeminfo, e_res2, e_lfanew};
    enum PeHeader_Field {NtSignature, Machine, NumberOfSections, TimeDateStamp, PointerToSymbolTable,
                        NumberOfSymbols, SizeOfOptionalHeader, Characteristics, Magic,
                        MajorLinkerVersion, MinorLinkerVersion, SizeOfCode, SizeOfInitializedData,
                        SizeOfUninitializedData, AddressOfEntryPoint, BaseOfCode, BaseOfData, ImageBase,
                        SectionAlignment, FileAlignment, MajorOperatingSystemVersion, MinorOperatingSystemVersion,
                        MajorImageVersion, MinorImageVersion, MajorSubsystemVersion, MinorSubsystemVersion,
                        Win32VersionValue, SizeOfImage, SizeOfHeaders, CheckSum, Subsystem, DllCharacteristics,
                        SizeOfStackReserve, SizeOfStackCommit, SizeOfHeapReserve, SizeOfHeapCommit,
                        LoaderFlags, NumberOfRvaAndSizes, DataDirectoryRva, DataDirectorySize};
    enum Section_Field {SectionName, VirtualSize, VirtualAddress, SizeOfRawData, PointerToRawData, PointerToRelocations,
                        PointerToLinenumbers, NumberOfRelocations, NumberOfLinenumbers, SectionCharacteristics};
*/
}

#endif
