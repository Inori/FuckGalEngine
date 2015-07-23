/*
* PeFile.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef PEFILE_H
#define PEFILE_H

#include "PeLibInc.h"
#include "MzHeader.h"
#include "PeHeader.h"
#include "ImportDirectory.h"
#include "ExportDirectory.h"
#include "BoundImportDirectory.h"
#include "ResourceDirectory.h"
#include "RelocationsDirectory.h"
#include "ComHeaderDirectory.h"
#include "IatDirectory.h"
#include "DebugDirectory.h"
#include "TlsDirectory.h"

namespace PeLib
{
	class PeFile32;
	class PeFile64;
	
	/**
	* Visitor base class for PeFiles.
	**/
	class PeFileVisitor
	{
		public:
		  virtual void callback(PeFile32 &file){}
		  virtual void callback(PeFile64 &file){}
		  virtual ~PeFileVisitor(){}
	};

	/**
	* Traits class that's used to decide of what type the PeHeader in a PeFile is.
	**/
	template<int>
	struct PeFile_Traits;

	template<>
	struct PeFile_Traits<32>
	{
		typedef PeHeader32 PeHeader32_64;
	};
	
	template<>
	struct PeFile_Traits<64>
	{
		typedef PeHeader64 PeHeader32_64;
	};
	
	/**
	* This class represents the common structures of PE and PE+ files.
	**/
	class PeFile
	{
		protected:
		  std::string m_filename; ///< Name of the current file.
		  MzHeader m_mzh; ///< MZ header of the current file.
		  ExportDirectory m_expdir; ///< Export directory of the current file.
		  BoundImportDirectory m_boundimpdir; ///< BoundImportDirectory of the current file.
		  ResourceDirectory m_resdir; ///< ResourceDirectory of the current file.
		  RelocationsDirectory m_relocs; ///< Relocations directory of the current file.
		  ComHeaderDirectory m_comdesc; ///< COM+ descriptor directory of the current file.
		  IatDirectory m_iat; ///< Import address table of the current file.
		  DebugDirectory m_debugdir;
		public:
		  virtual ~PeFile();
		  
		  /// Returns the name of the current file.
		  virtual std::string getFileName() const = 0; // EXPORT
		  /// Changes the name of the current file.
		  virtual void setFileName(std::string strFilename) = 0; // EXPORT
		  
		  virtual void visit(PeFileVisitor &v) = 0;
		  
		  /// Reads the MZ header of the current file from disc.
		  virtual int readMzHeader() = 0; // EXPORT
		  /// Reads the export directory of the current file from disc.
		  virtual int readExportDirectory()  = 0; // EXPORT
		  /// Reads the PE header of the current file from disc.
		  virtual int readPeHeader()  = 0; // EXPORT
		  /// Reads the import directory of the current file from disc.
		  virtual int readImportDirectory()  = 0; // EXPORT
		  /// Reads the bound import directory of the current file from disc.
		  virtual int readBoundImportDirectory()  = 0; // EXPORT
		  /// Reads the resource directory of the current file from disc.
		  virtual int readResourceDirectory()  = 0; // EXPORT
		  /// Reads the relocations directory of the current file from disc.
		  virtual int readRelocationsDirectory()  = 0; // EXPORT
		  /// Reads the COM+ descriptor directory of the current file from disc.
		  virtual int readComHeaderDirectory()  = 0; // EXPORT
		  /// Reads the IAT directory of the current file from disc.
		  virtual int readIatDirectory()  = 0; // EXPORT
		  /// Reads the Debug directory of the current file.		  
		  virtual int readDebugDirectory()  = 0; // EXPORT
		  virtual int readTlsDirectory()  = 0; // EXPORT
		  
		  virtual unsigned int getBits() const = 0;

		  /// Accessor function for the MZ header.
		  const MzHeader& mzHeader() const;
		  /// Accessor function for the MZ header.
		  MzHeader& mzHeader(); // EXPORT

		  /// Accessor function for the export directory.
		  const ExportDirectory& expDir() const;
		  /// Accessor function for the export directory.
		  ExportDirectory& expDir(); // EXPORT
		  
		  /// Accessor function for the bound import directory.
		  const BoundImportDirectory& boundImpDir() const;
		  /// Accessor function for the bound import directory.
		  BoundImportDirectory& boundImpDir(); // EXPORT
		  
		  /// Accessor function for the resource directory.
		  const ResourceDirectory& resDir() const;
		  /// Accessor function for the resource directory.
		  ResourceDirectory& resDir(); // EXPORT

		  /// Accessor function for the relocations directory.
		  const RelocationsDirectory& relocDir() const;
		  /// Accessor function for the relocations directory.
		  RelocationsDirectory& relocDir(); // EXPORT

		  /// Accessor function for the COM+ descriptor directory.
		  const ComHeaderDirectory& comDir() const;
		  /// Accessor function for the COM+ descriptor directory.
		  ComHeaderDirectory& comDir(); // EXPORT

		  /// Accessor function for the IAT directory.
		  const IatDirectory& iatDir() const;
		  /// Accessor function for the IAT directory.
		  IatDirectory& iatDir(); // EXPORT
		  
		  /// Accessor function for the debug directory.
		  const DebugDirectory& debugDir() const;
		  /// Accessor function for the debug directory.
		  DebugDirectory& debugDir(); // EXPORT
		  
	};
	
	/**
	* This class implements the common structures of PE and PE+ files.
	**/
	template<int bits>
	class PeFileT : public PeFile
	{
		typedef typename PeFile_Traits<bits>::PeHeader32_64 PeHeader32_64;
		
		private:
		  PeHeader32_64 m_peh; ///< PE header of the current file.
		  ImportDirectory<bits> m_impdir; ///< Import directory of the current file.
		  TlsDirectory<bits> m_tlsdir;
		
		public:
		  /// Default constructor which exists only for the sake of allowing to construct files without filenames.
		  PeFileT();
		  
		  virtual ~PeFileT() {}
		  
		  /// Initializes a PeFile with a filename
		  explicit PeFileT(const std::string& strFilename);
		  
		  /// Returns the name of the current file.
		  std::string getFileName() const;
		  /// Changes the name of the current file.
		  void setFileName(std::string strFilename);

		  /// Reads the MZ header of the current file from disc.
		  int readMzHeader() ;
		  /// Reads the export directory of the current file from disc.
		  int readExportDirectory() ;
		  /// Reads the PE header of the current file from disc.
		  int readPeHeader() ;
		  /// Reads the import directory of the current file from disc.
		  int readImportDirectory() ;
		  /// Reads the bound import directory of the current file from disc.
		  int readBoundImportDirectory() ;
		  /// Reads the resource directory of the current file from disc.
		  int readResourceDirectory() ;
		  /// Reads the relocations directory of the current file from disc.
		  int readRelocationsDirectory() ;
		  /// Reads the COM+ descriptor directory of the current file from disc.
		  int readComHeaderDirectory() ;
		  /// Reads the IAT directory of the current file from disc.
		  int readIatDirectory() ;
		  /// Reads the Debug directory of the current file.		  
		  int readDebugDirectory() ;
		  int readTlsDirectory() ;
		  
		  unsigned int getBits() const
		  {
			  return bits;
		  }
		  
		  /// Accessor function for the PE header.
		  const PeHeader32_64& peHeader() const;
		  /// Accessor function for the PE header.
		  PeHeader32_64& peHeader();

		  /// Accessor function for the import directory.
		  const ImportDirectory<bits>& impDir() const;
		  /// Accessor function for the import directory.
		  ImportDirectory<bits>& impDir();
		  
		  const TlsDirectory<bits>& tlsDir() const;
		  TlsDirectory<bits>& tlsDir();
	};

	/**
	* This class is the main class for handling PE files.
	**/
	class PeFile32 : public PeFileT<32>
	{
		public:
		  /// Default constructor which exists only for the sake of allowing to construct files without filenames.
		  PeFile32();

		  /// Initializes a PeFile with a filename
		  explicit PeFile32(const std::string& strFlename);
		  virtual void visit(PeFileVisitor &v) { v.callback( *this ); }
	};
	
	/**
	* This class is the main class for handling PE+ files.
	**/
	class PeFile64 : public PeFileT<64>
	{
		public:
		  /// Default constructor which exists only for the sake of allowing to construct files without filenames.
		  PeFile64();

		  /// Initializes a PeFile with a filename
		  explicit PeFile64(const std::string& strFlename);
		  virtual void visit(PeFileVisitor &v) { v.callback( *this ); }
	};
	
	//typedef PeFileT<32> PeFile32;
	//typedef PeFileT<64> PeFile64;
	
	/**
	* @param strFilename Name of the current file.
	**/
	template<int bits>
	PeFileT<bits>::PeFileT(const std::string& strFilename)
	{
 		m_filename = strFilename;
 	}

	template<int bits>
	PeFileT<bits>::PeFileT()
	{
	}

	template<int bits>
	int PeFileT<bits>::readPeHeader() 
	{
		return peHeader().read(getFileName(), mzHeader().getAddressOfPeHeader());
	}

	
	template<int bits>
	int PeFileT<bits>::readImportDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 2
			&& peHeader().getIddImportRva()
			&& peHeader().getIddImportSize())
		{
			return impDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddImportRva())), peHeader().getIddImportSize(), peHeader());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	/**
	* @return A reference to the file's PE header.
	**/
	template<int bits>
	const typename PeFile_Traits<bits>::PeHeader32_64& PeFileT<bits>::peHeader() const
	{
		return m_peh;
	}

	/**
	* @return A reference to the file's PE header.
	**/
	template<int bits>
	typename PeFile_Traits<bits>::PeHeader32_64& PeFileT<bits>::peHeader()
	{
		return m_peh;
	}

	/**
	* @return A reference to the file's import directory.
	**/
	template<int bits>
	const ImportDirectory<bits>& PeFileT<bits>::impDir() const
	{
		return m_impdir;
	}

	/**
	* @return A reference to the file's import directory.
	**/
	template<int bits>
	ImportDirectory<bits>& PeFileT<bits>::impDir()
	{
		return m_impdir;
	}

	template<int bits>
	const TlsDirectory<bits>& PeFileT<bits>::tlsDir() const
	{
		return m_tlsdir;
	}

	template<int bits>
	TlsDirectory<bits>& PeFileT<bits>::tlsDir()
	{
		return m_tlsdir;
	}

	/**
	* @return Filename of the current file.
	**/
	template<int bits>
	std::string PeFileT<bits>::getFileName() const
	{
		return m_filename;
	}

	/**
	* @param strFilename New filename.
	**/
	template<int bits>
	void PeFileT<bits>::setFileName(std::string strFilename)
	{
		m_filename = strFilename;
	}

	template<int bits>
	int PeFileT<bits>::readMzHeader() 
	{
		return mzHeader().read(getFileName());
	}
	
	template<int bits>
	int PeFileT<bits>::readExportDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 1
			&& peHeader().getIddExportRva() && peHeader().getIddExportSize())
		{
			return expDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddExportRva())), peHeader().getIddExportSize(), peHeader());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	
	template<int bits>
	int PeFileT<bits>::readBoundImportDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 12
			&& peHeader().getIddBoundImportRva() && peHeader().getIddBoundImportSize())
		{
			return boundImpDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddBoundImportRva())), peHeader().getIddBoundImportSize());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readResourceDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 3
			&& peHeader().getIddResourceRva() && peHeader().getIddResourceSize())
		{
			return resDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddResourceRva())), peHeader().getIddResourceSize(), peHeader().getIddResourceRva());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readRelocationsDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 6
			&& peHeader().getIddBaseRelocRva() && peHeader().getIddBaseRelocSize())
		{
			return relocDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddBaseRelocRva())), peHeader().getIddBaseRelocSize());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readComHeaderDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 15
			&& peHeader().getIddComHeaderRva() && peHeader().getIddComHeaderSize())
		{
			return comDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddComHeaderRva())), peHeader().getIddComHeaderSize());
		}
			std::cout << peHeader().getIddComHeaderRva() << std::endl;
			std::exit(0);
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readIatDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 13
			&& peHeader().getIddIatRva() && peHeader().getIddIatSize())
		{
			return iatDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddIatRva())), peHeader().getIddIatSize());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readDebugDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 7
			&& peHeader().getIddDebugRva() && peHeader().getIddDebugSize())
		{
			return debugDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddDebugRva())), peHeader().getIddDebugSize());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

	template<int bits>
	int PeFileT<bits>::readTlsDirectory() 
	{
		if (peHeader().calcNumberOfRvaAndSizes() >= 10
			&& peHeader().getIddTlsRva() && peHeader().getIddTlsSize())
		{
			return tlsDir().read(getFileName(), static_cast<unsigned int>(peHeader().rvaToOffset(peHeader().getIddTlsRva())), peHeader().getIddTlsSize());
		}
		return ERROR_DIRECTORY_DOES_NOT_EXIST;
	}

}

#endif
