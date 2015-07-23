/*
* ResourceDirectory.cpp - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#ifndef RESOURCEDIRECTORY_H
#define RESOURCEDIRECTORY_H

#include "PeLibInc.h"

namespace PeLib
{
	class ResourceElement;
	
	/// The class ResourceChild is used to store information about a resource node.
	class ResourceChild
	{
		friend class ResourceElement;
		friend class ResourceDirectory;
		friend class ResourceNode;
		friend class ResourceLeaf;
		
		/// Stores name and offset of a resource node.
		PELIB_IMG_RES_DIR_ENTRY entry;
		/// A pointer to one of the node's child nodes.
		ResourceElement* child;
		
		public:
		  /// Function which compares a resource ID to the node's resource ID.
		  bool equalId(dword wId) const; // EXPORT
		  /// Function which compares a string to the node's resource name.
		  bool equalName(std::string strName) const; // EXPORT
		  /// Predicate that determines if a child is identified by name or by ID.
		  bool isNamedResource() const; // EXPORT
		  /// Used for sorting a node's children.
		  bool operator<(const ResourceChild& rc) const; // EXPORT
		  /// Returns the size of a resource child.
//		unsigned int size() const;

		  /// Standard constructor. Does absolutely nothing.
		  ResourceChild();
		  /// Makes a deep copy of a ResourceChild object.
		  ResourceChild(const ResourceChild& rhs);
		  /// Makes a deep copy of a ResourceChild object.
		  ResourceChild& operator=(const ResourceChild& rhs);
		  /// Deletes a ResourceChild object.
		  ~ResourceChild();
	};
	
	/// Base class for ResourceNode and ResourceLeaf, the elements of the resource tree.
	/// \todo write
	class ResourceElement
	{
		friend class ResourceChild;
		friend class ResourceNode;
		friend class ResourceLeaf;
		
		protected:
		  /// Stores RVA of the resource element in the file.
		  unsigned int uiElementRva;
		
		  /// Reads the next resource element from the InputBuffer.
		  virtual int read(InputBuffer&, unsigned int, unsigned int/*, const std::string&*/) = 0;
		  /// Writes the next resource element into the OutputBuffer.
		  virtual void rebuild(OutputBuffer&, unsigned int&, unsigned int, const std::string&) const = 0;
		  
		public:
		  /// Returns the RVA of the element in the file.
		  unsigned int getElementRva() const; // EXPORT
		  /// Indicates if the resource element is a leaf or a node.
		  virtual bool isLeaf() const = 0; // EXPORT
		  /// Corrects erroneous valeus in the ResourceElement.
		  virtual void makeValid() = 0; // EXPORT
		  /// Returns the size of a resource element.
//		  virtual unsigned int size() const = 0;
		  /// Necessary virtual destructor.
		  virtual ~ResourceElement() {}
	};
	
	/// ResourceLeafs represent the leafs of the resource tree: The actual resources.
	class ResourceLeaf : public ResourceElement
	{
		friend class ResourceChild;
		friend class ResourceDirectory;
		template<typename T> friend struct fixNumberOfEntries;
		
		private:
		  /// The resource data.
		  std::vector<byte> m_data;
		  /// PeLib equivalent of the Win32 structure IMAGE_RESOURCE_DATA_ENTRY
		  PELIB_IMAGE_RESOURCE_DATA_ENTRY entry;
		  
		protected:
		  int read(InputBuffer& inpBuffer, unsigned int uiOffset, unsigned int rva/*, const std::string&*/);
		  /// Writes the next resource leaf into the OutputBuffer.
		  void rebuild(OutputBuffer&, unsigned int& uiOffset, unsigned int uiRva, const std::string&) const;
		
		public:
		  /// Indicates if the resource element is a leaf or a node.
		  bool isLeaf() const; // EXPORT
		  /// Corrects erroneous valeus in the ResourceLeaf.
		  void makeValid(); // EXPORT
		  /// Reads the next resource leaf from the InputBuffer.
		  /// Returns the size of a resource lead.
//		  unsigned int size() const;

		  /// Returns the resource data of this resource leaf.
		  std::vector<byte> getData() const; // EXPORT
		  /// Sets the resource data of this resource leaf.
		  void setData(const std::vector<byte>& vData); // EXPORT

		  /// Returns the OffsetToData value of this resource leaf.
		  dword getOffsetToData() const; // EXPORT
		  /// Returns the Size value of this resource leaf.
		  dword getSize() const; // EXPORT
		  /// Returns the CodePage value of this resource leaf.
		  dword getCodePage() const; // EXPORT
		  /// Returns the Reserved value of this resource leaf.
		  dword getReserved() const; // EXPORT
		  
		  /// Sets the OffsetToData value of this resource leaf.
		  void setOffsetToData(dword dwValue); // EXPORT
		  /// Sets the Size value of this resource leaf.
		  void setSize(dword dwValue); // EXPORT
		  /// Sets the CodePage value of this resource leaf.
		  void setCodePage(dword dwValue); // EXPORT
		  /// Sets the Reserved value of this resource leaf.
		  void setReserved(dword dwValue); // EXPORT
	};
	
	/// ResourceNodes represent the nodes in the resource tree.
	class ResourceNode : public ResourceElement
	{
		friend class ResourceChild;
		friend class ResourceDirectory;
		template<typename T> friend struct fixNumberOfEntries;
		
		/// The node's children.
		std::vector<ResourceChild> children;
		/// The node's header. Equivalent to IMAGE_RESOURCE_DIRECTORY from the Win32 API.
		PELIB_IMAGE_RESOURCE_DIRECTORY header;
		
		protected:
		  /// Reads the next resource node.
		  int read(InputBuffer& inpBuffer, unsigned int uiOffset, unsigned int rva/*, const std::string&*/);
		  /// Writes the next resource node into the OutputBuffer.
		  void rebuild(OutputBuffer&, unsigned int& uiOffset, unsigned int uiRva, const std::string&) const;
		  
		public:
		  /// Indicates if the resource element is a leaf or a node.
		  bool isLeaf() const; // EXPORT
		  /// Corrects erroneous valeus in the ResourceNode.
		  void makeValid(); // EXPORT
		  
		  /// Returns the node's number of children.
		  unsigned int getNumberOfChildren() const; // EXPORT
		  /// Adds another child to node.
		  void addChild(); // EXPORT
		  /// Returns a node's child.
		  ResourceElement* getChild(unsigned int uiIndex); // EXPORT
		  /// Removes a node's child.
		  void removeChild(unsigned int uiIndex); // EXPORT
		  
		  /// Returns the name of one of the node's children.
		  std::string getChildName(unsigned int uiIndex) const; // EXPORT
		  /// Returns the Name value of one of the node's children.
		  dword getOffsetToChildName(unsigned int uiIndex) const; // EXPORT
		  /// Returns the OffsetToData value of one of the node's children.
		  dword getOffsetToChildData(unsigned int uiIndex) const; // EXPORT
		  
		  /// Sets the name of one of the node's children.
		  void setChildName(unsigned int uiIndex, const std::string& strNewName); // EXPORT
		  /// Sets the Name value of one of the node's children.
		  void setOffsetToChildName(unsigned int uiIndex, dword dwNewOffset); // EXPORT
		  /// Sets the OffsetToData value of one of the node's children.
		  void setOffsetToChildData(unsigned int uiIndex, dword dwNewOffset); // EXPORT
		  
		  /// Returns the node's Characteristics value.
		  dword getCharacteristics() const; // EXPORT
		  /// Returns the node's TimeDateStamp value.
		  dword getTimeDateStamp() const; // EXPORT
		  /// Returns the node's MajorVersion value.
		  word getMajorVersion() const; // EXPORT
		  /// Returns the node's MinorVersion value.
		  word getMinorVersion() const; // EXPORT
		  /// Returns the node's NumberOfNamedEntries value.
		  word getNumberOfNamedEntries() const; // EXPORT
		  /// Returns the node's NumberOfIdEntries value.
		  word getNumberOfIdEntries() const; // EXPORT
		  
		  /// Sets the node's Characteristics value.
		  void setCharacteristics(dword value); // EXPORT
		  /// Sets the node's TimeDateStamp value.
		  void setTimeDateStamp(dword value); // EXPORT
		  /// Sets the node's MajorVersion value.
		  void setMajorVersion(word value); // EXPORT
		  /// Sets the node's MinorVersion value.
		  void setMinorVersion(word value); // EXPORT
		  /// Sets the node's NumberOfNamedEntries value.
		  void setNumberOfNamedEntries(word value); // EXPORT
		  /// Sets the node's NumberOfIdEntries value.
		  void setNumberOfIdEntries(word value); // EXPORT
		  
		  /// Returns the size of a resource node.
//		unsigned int size() const;
	};

	/// Auxiliary functor which is used to search through the resource tree.
	/**
	* Traits class for the template functions of ResourceDirectory.
	* It's used to find out which function to use when searching for resource nodes or resource leafs
	* in a node's children vector.
	**/
	template<typename T>
	struct ResComparer
	{
		/// Pointer to a member function of ResourceChild
		typedef bool(ResourceChild::*CompFunc)(T) const;
		 
		/// Return 0 for all unspecialized versions of ResComparer.
		static CompFunc comp();
	};

	/// Auxiliary functor which is used to search through the resource tree.
	/**
	* ResComparer<dword> is used when a resource element is searched for by ID.
	**/
	template<>
	struct ResComparer<dword>
	{
		/// Pointer to a member function of ResourceChild
		typedef bool(ResourceChild::*CompFunc)(dword) const;
		 
		/// Return the address of the ResourceChild member function that compares the ids of resource elements.
		static CompFunc comp()
		{
			return &ResourceChild::equalId;
		}
	};

	/// Auxiliary functor which is used to search through the resource tree.
	/**
	* This specializd version of ResComparer is used when a resource element is searched for by name.
	**/
	template<>
	struct ResComparer<std::string>
	{
		/// Pointer to a member function of ResourceChild
		typedef bool(ResourceChild::*CompFunc)(std::string) const;
		 
		/// Return the address of the ResourceChild member function that compares the names of resource elements.
		static CompFunc comp()
		{
			return &ResourceChild::equalName;
		}
	};

	/// Unspecialized function that's used as base template for the specialized versions below.
	template<typename T>
	struct fixNumberOfEntries
	{
		/// Fixes a resource node's header.
		static void fix(ResourceNode*);
	};
		  
	/// Fixes NumberOfIdEntries value of a node.
	template<>
	struct fixNumberOfEntries<dword>
	{
		/// Fixes a resource node's NumberOfIdEntries value.
		static void fix(ResourceNode* node)
		{
			node->header.NumberOfIdEntries = (unsigned int)node->children.size() - std::count_if(node->children.begin(), node->children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource));
		}
	};

	/// Fixes NumberOfNamedEntries value of a node.
	template<>
	struct fixNumberOfEntries<std::string>
	{
		/// Fixes a resource node's NumberOfNamedEntries value.
		static void fix(ResourceNode* node)
		{
			node->header.NumberOfNamedEntries = static_cast<PeLib::word>(std::count_if(node->children.begin(), node->children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource)));
		}
	};
	
	/// Class that represents the resource directory of a PE file.	  
	/**
	* The class ResourceDirectory represents the resource directory of a PE file. This class is fundamentally
	* different from the other classes of the PeLib library due to the structure of the ResourceDirectory.
	* For once, it's possible to manipulate the ResourceDirectory through a set of "high level" functions and
	* and through a set of "low level" functions. The "high level" functions are the functions inside the
	* ResourceDirectory class with the exception of getRoot.<br><br>
	* getRoot on the other hand is the first "low level" function. Use it to retrieve the root node of the
	* resource tree. Then you can traverse through the tree and manipulate individual nodes and leafs
	* directly using the functions provided by the classes ResourceNode and ResourceLeaf.<br><br>
	* There's another difference between the ResourceDirectory class and the other PeLib classes, which is
	* once again caused by the special structure of the PE resource directory. The nodes of the resource
	* tree must be in a certain order. Manipulating the resource tree does not directly sort the nodes
	* correctly as this would cause more trouble than it fixes. That means it's your responsibility to 
	* fix the resource tree after manipulating it. PeLib makes the job easy for you, just call the
	* ResourceDirectory::makeValid function.<br><br>
	* You might also wonder why there's no size() function in this class. I did not forget it. It's just
	* that it's impossible to calculate the size of the resource directory without rebuilding it. So why
	* should PeLib do this if you can do it just as easily by calling rebuild() and then checking the length
	* of the returned vector.<br><br>
	* There are also different ways to serialize (rebuild) the resource tree as it's not a fixed structure
	* that can easily be minimized like most other PE directories.<br><br>
	* This means it's entirely possible that the resource tree you read from a file differs from the one
	* PeLib creates. This might cause a minor issue. The original resource tree might be smaller (due to
	* different padding) so it's crucial that you check if there's enough space in the original resource
	* directory before you write the rebuilt resource directory back to the file.
	**/
	class ResourceDirectory
	{
		private:
		  /// The root node of the resource directory.
		  ResourceNode m_rnRoot;

		  // Prepare for some crazy syntax below to make Digital Mars happy.
		  
		  /// Retrieves an iterator to a specified resource child.
		  template<typename S, typename T>
		  std::vector<ResourceChild>::const_iterator locateResourceT(S restypeid, T resid) const;
		  
		  /// Retrieves an iterator to a specified resource child.
		  template<typename S, typename T>
		  std::vector<ResourceChild>::iterator locateResourceT(S restypeid, T resid);
		  
		  /// Adds a new resource.
		  template<typename S, typename T>
		  int addResourceT(S restypeid, T resid, ResourceChild& rc);
		  
		  /// Removes new resource.
		  template<typename S, typename T>
		  int removeResourceT(S restypeid, T resid);
		  
		  /// Returns the data of a resource.
		  template<typename S, typename T>
		  int getResourceDataT(S restypeid, T resid, std::vector<byte>& data) const;
		  
		  /// Sets the data of a resource.
		  template<typename S, typename T>
		  int setResourceDataT(S restypeid, T resid, std::vector<byte>& data);
		  
		  /// Returns the ID of a resource.
		  template<typename S, typename T>
		  dword getResourceIdT(S restypeid, T resid) const;
		  
		  /// Sets the ID of a resource.
		  template<typename S, typename T>
		  int setResourceIdT(S restypeid, T resid, dword dwNewResId);
		  
		  /// Returns the name of a resource.
		  template<typename S, typename T>
		  std::string getResourceNameT(S restypeid, T resid) const;
		  
		  /// Sets the name of a resource.
		  template<typename S, typename T>
		  int setResourceNameT(S restypeid, T resid, std::string strNewResName);
		  
		public:
		  ResourceNode* getRoot();
		  /// Corrects a erroneous resource directory.
		  void makeValid();
		  /// Reads the resource directory from a file.
		  int read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize, unsigned int uiResDirRva);
		  /// Rebuilds the resource directory.
		  void rebuild(std::vector<byte>& vBuffer, unsigned int uiRva) const;
		  /// Returns the size of the rebuilt resource directory.
//		  unsigned int size() const;
		  /// Writes the resource directory to a file.
		  int write(const std::string& strFilename, unsigned int uiOffset, unsigned int uiRva) const;

		  /// Adds a new resource type.
		  int addResourceType(dword dwResTypeId);
		  /// Adds a new resource type.
		  int addResourceType(const std::string& strResTypeName);
		  
		  /// Removes a resource type and all of it's resources.
		  int removeResourceType(dword dwResTypeId);
		  /// Removes a resource type and all of it's resources.
		  int removeResourceType(const std::string& strResTypeName);
		  
		  /// Removes a resource type and all of it's resources.
		  int removeResourceTypeByIndex(unsigned int uiIndex);
		  
		  /// Adds a new resource.
		  int addResource(dword dwResTypeId, dword dwResId);
		  /// Adds a new resource.
		  int addResource(dword dwResTypeId, const std::string& strResName);
		  /// Adds a new resource.
		  int addResource(const std::string& strResTypeName, dword dwResId);
		  /// Adds a new resource.
		  int addResource(const std::string& strResTypeName, const std::string& strResName);

		  /// Removes a resource.
		  int removeResource(dword dwResTypeId, dword dwResId);
		  /// Removes a resource.
		  int removeResource(dword dwResTypeId, const std::string& strResName);
		  /// Removes a resource.
		  int removeResource(const std::string& strResTypeName, dword dwResId);
		  /// Removes a resource.
		  int removeResource(const std::string& strResTypeName, const std::string& strResName);

		  /// Returns the number of resource types.
		  unsigned int getNumberOfResourceTypes() const;
		  
		  /// Returns the ID of a resource type.
		  dword getResourceTypeIdByIndex(unsigned int uiIndex) const;
		  /// Returns the name of a resource type.
		  std::string getResourceTypeNameByIndex(unsigned int uiIndex) const;
		  
		  /// Converts a resource type ID to an index.
		  int resourceTypeIdToIndex(dword dwResTypeId) const;
		  /// Converts a resource type name to an index.
		  int resourceTypeNameToIndex(const std::string& strResTypeName) const;
		  
		  /// Returns the number of resources of a certain resource type.
		  unsigned int getNumberOfResources(dword dwId) const;
		  /// Returns the number of resources of a certain resource type.
		  unsigned int getNumberOfResources(const std::string& strResTypeName) const;
		  
		  /// Returns the number of resources of a certain resource type.
		  unsigned int getNumberOfResourcesByIndex(unsigned int uiIndex) const;
		  
		  /// Returns the data of a certain resource.
		  void getResourceData(dword dwResTypeId, dword dwResId, std::vector<byte>& data) const;
		  /// Returns the data of a certain resource.
		  void getResourceData(dword dwResTypeId, const std::string& strResName, std::vector<byte>& data) const;
		  /// Returns the data of a certain resource.
		  void getResourceData(const std::string& strResTypeName, dword dwResId, std::vector<byte>& data) const;
		  /// Returns the data of a certain resource.
		  void getResourceData(const std::string& strResTypeName, const std::string& strResName, std::vector<byte>& data) const;
		  
		  /// Returns the data of a certain resource.
		  void getResourceDataByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, std::vector<byte>& data) const;
		  
		  /// Sets the data of a certain resource.
		  void setResourceData(dword dwResTypeId, dword dwResId, std::vector<byte>& data);
		  /// Sets the data of a certain resource.
		  void setResourceData(dword dwResTypeId, const std::string& strResName, std::vector<byte>& data);
		  /// Sets the data of a certain resource.
		  void setResourceData(const std::string& strResTypeName, dword dwResId, std::vector<byte>& data);
		  /// Sets the data of a certain resource.
		  void setResourceData(const std::string& strResTypeName, const std::string& strResName, std::vector<byte>& data);
		  
		  /// Sets the data of a certain resource.
		  void setResourceDataByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, std::vector<byte>& data);

		  /// Returns the ID of a certain resource.
		  dword getResourceId(dword dwResTypeId, const std::string& strResName) const;
		  /// Returns the ID of a certain resource.
		  dword getResourceId(const std::string& strResTypeName, const std::string& strResName) const;
		  
		  /// Returns the ID of a certain resource.
		  dword getResourceIdByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex) const;
		  
		  /// Sets the ID of a certain resource.
		  void setResourceId(dword dwResTypeId, dword dwResId, dword dwNewResId);
		  /// Sets the ID of a certain resource.
		  void setResourceId(dword dwResTypeId, const std::string& strResName, dword dwNewResId);
		  /// Sets the ID of a certain resource.
		  void setResourceId(const std::string& strResTypeName, dword dwResId, dword dwNewResId);
		  /// Sets the ID of a certain resource.
		  void setResourceId(const std::string& strResTypeName, const std::string& strResName, dword dwNewResId);
		  
		  /// Sets the ID of a certain resource.
		  void setResourceIdByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, dword dwNewResId);
		  
		  /// Returns the name of a certain resource.
		  std::string getResourceName(dword dwResTypeId, dword dwResId) const;
		  /// Returns the name of a certain resource.
		  std::string getResourceName(const std::string& strResTypeName, dword dwResId) const;
		  
		  /// Returns the name of a certain resource.
		  std::string getResourceNameByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex) const;
		  
		  /// Sets the name of a certain resource.
		  void setResourceName(dword dwResTypeId, dword dwResId, const std::string& strNewResName);
		  /// Sets the name of a certain resource.
		  void setResourceName(dword dwResTypeId, const std::string& strResName, const std::string& strNewResName);
		  /// Sets the name of a certain resource.
		  void setResourceName(const std::string& strResTypeName, dword dwResId, const std::string& strNewResName);
		  /// Sets the name of a certain resource.
		  void setResourceName(const std::string& strResTypeName, const std::string& strResName, const std::string& strNewResName);
		  
		  /// Sets the name of a certain resource.
		  void setResourceNameByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, const std::string& strNewResName);
	};
	
	/**
	* Looks through the entire resource tree and returns a const_iterator to the resource specified
	* by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @return A const_iterator to the specified resource.
	**/
	template<typename S, typename T>
	std::vector<ResourceChild>::const_iterator ResourceDirectory::locateResourceT(S restypeid, T resid) const
	{
		typedef bool(ResourceChild::*CompFunc1)(S) const;
		typedef bool(ResourceChild::*CompFunc2)(T) const;
		
		CompFunc1 comp1 = ResComparer<S>::comp();
		CompFunc2 comp2 = ResComparer<T>::comp();

		std::vector<ResourceChild>::const_iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(comp1), restypeid));
		if (Iter == m_rnRoot.children.end())
		{
			return Iter;
		}
		
		ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
		std::vector<ResourceChild>::const_iterator ResIter = std::find_if(currNode->children.begin(), currNode->children.end(), std::bind2nd(std::mem_fun_ref(comp2), resid));
		if (ResIter == currNode->children.end())
		{
			return ResIter;
		}
			
		return ResIter;
	}
	
	/**
	* Looks through the entire resource tree and returns an iterator to the resource specified
	* by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @return An iterator to the specified resource.
	**/
	template<typename S, typename T>
	std::vector<ResourceChild>::iterator ResourceDirectory::locateResourceT(S restypeid, T resid)
	{
		typedef bool(ResourceChild::*CompFunc1)(S) const;
		typedef bool(ResourceChild::*CompFunc2)(T) const;
		
		CompFunc1 comp1 = ResComparer<S>::comp();
		CompFunc2 comp2 = ResComparer<T>::comp();

		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(comp1), restypeid));
		if (Iter == m_rnRoot.children.end())
		{
			return Iter;
		}
		
		ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
		std::vector<ResourceChild>::iterator ResIter = std::find_if(currNode->children.begin(), currNode->children.end(), std::bind2nd(std::mem_fun_ref(comp2), resid));
		if (ResIter == currNode->children.end())
		{
			return ResIter;
		}
			
		return ResIter;
	}
	
	/**
	* Adds a new resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @param rc ResourceChild that will be added.
	**/
	template<typename S, typename T>
	int ResourceDirectory::addResourceT(S restypeid, T resid, ResourceChild& rc)
	{
		typedef bool(ResourceChild::*CompFunc1)(S) const;
		typedef bool(ResourceChild::*CompFunc2)(T) const;
		
		CompFunc1 comp1 = ResComparer<S>::comp();
		CompFunc2 comp2 = ResComparer<T>::comp();

		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(comp1), restypeid));
		if (Iter == m_rnRoot.children.end())
		{
			return 1;
			// throw Exceptions::ResourceTypeDoesNotExist(ResourceDirectoryId, __LINE__);
		}
		
		ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
		std::vector<ResourceChild>::iterator ResIter = std::find_if(currNode->children.begin(), currNode->children.end(), std::bind2nd(std::mem_fun_ref(comp2), resid));
		if (ResIter != currNode->children.end())
		{
			return 1;
//			throw Exceptions::EntryAlreadyExists(ResourceDirectoryId, __LINE__);
		}
				
		rc.child = new ResourceNode;
		ResourceChild rlnew;
		rlnew.child = new ResourceLeaf;
		ResourceNode* currNode2 = static_cast<ResourceNode*>(rc.child);
		currNode2->children.push_back(rlnew);
		currNode->children.push_back(rc);
		
		fixNumberOfEntries<T>::fix(currNode);
		fixNumberOfEntries<T>::fix(currNode2);
		
		return 0;
	}
	
	/**
	* Removes a resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	**/
	template<typename S, typename T>
	int ResourceDirectory::removeResourceT(S restypeid, T resid)
	{
		typedef bool(ResourceChild::*CompFunc1)(S) const;
		typedef bool(ResourceChild::*CompFunc2)(T) const;
		
		CompFunc1 comp1 = ResComparer<S>::comp();
		CompFunc2 comp2 = ResComparer<T>::comp();
		
		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(comp1), restypeid));
		if (Iter == m_rnRoot.children.end())
		{
			return 1;
			//throw Exceptions::ResourceTypeDoesNotExist(ResourceDirectoryId, __LINE__);
		}
		
		ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
		std::vector<ResourceChild>::iterator ResIter = std::find_if(currNode->children.begin(), currNode->children.end(), std::bind2nd(std::mem_fun_ref(comp2), resid));
		if (ResIter == currNode->children.end())
		{
			return 1;
			// throw Exceptions::InvalidName(ResourceDirectoryId, __LINE__);
		}
			
		currNode->children.erase(ResIter);
		
		fixNumberOfEntries<T>::fix(currNode);
		
		return 0;
	}
	
	/**
	* Returns the data of a resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @param data The data of the resource will be written into this vector.
	**/
	template<typename S, typename T>
	int ResourceDirectory::getResourceDataT(S restypeid, T resid, std::vector<byte>& data) const
	{
		std::vector<ResourceChild>::const_iterator ResIter = locateResourceT(restypeid, resid);
		ResourceNode* currNode = static_cast<ResourceNode*>(ResIter->child);
		ResourceLeaf* currLeaf = static_cast<ResourceLeaf*>(currNode->children[0].child);
		data.assign(currLeaf->m_data.begin(), currLeaf->m_data.end());
		
		return 0;
	}
	
	/**
	* Sets the data of a resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @param data The new data of the resource is taken from this vector.
	**/
	template<typename S, typename T>
	int ResourceDirectory::setResourceDataT(S restypeid, T resid, std::vector<byte>& data)
	{
		std::vector<ResourceChild>::iterator ResIter = locateResourceT(restypeid, resid);
		ResourceNode* currNode = static_cast<ResourceNode*>(ResIter->child);
		ResourceLeaf* currLeaf = static_cast<ResourceLeaf*>(currNode->children[0].child);
		currLeaf->m_data.assign(data.begin(), data.end());
		
		return 0;
	}
	
	/**
	* Returns the id of a resource, resource type and ID are specified by the parameters.
	* Note: Calling this function with resid == the ID of the resource makes no sense at all.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @return The ID of the specified resource.
	**/
	template<typename S, typename T>
	dword ResourceDirectory::getResourceIdT(S restypeid, T resid) const
	{
		std::vector<ResourceChild>::const_iterator ResIter = locateResourceT(restypeid, resid);
		return ResIter->entry.irde.Name;
	}
	
	/**
	* Sets the id of a resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @param dwNewResId New ID of the resource.
	**/
	template<typename S, typename T>
	int ResourceDirectory::setResourceIdT(S restypeid, T resid, dword dwNewResId)
	{
		std::vector<ResourceChild>::iterator ResIter = locateResourceT(restypeid, resid);
		ResIter->entry.irde.Name = dwNewResId;
		return 0;
	}
		  
	/**
	* Returns the name of a resource, resource type and ID are specified by the parameters.
	* Note: Calling this function with resid == the name of the resource makes no sense at all.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @return The name of the specified resource.
	**/
	template<typename S, typename T>
	std::string ResourceDirectory::getResourceNameT(S restypeid, T resid) const
	{
		std::vector<ResourceChild>::const_iterator ResIter = locateResourceT(restypeid, resid);
		return ResIter->entry.wstrName;
	}
		  
	/**
	* Sets the name of a resource, resource type and ID are specified by the parameters.
	* @param restypeid Identifier of the resource type (either ID or name).
	* @param resid Identifier of the resource (either ID or name).
	* @param strNewResName The new name of the resource.
	**/
	template<typename S, typename T>
	int ResourceDirectory::setResourceNameT(S restypeid, T resid, std::string strNewResName)
	{
		std::vector<ResourceChild>::iterator ResIter = locateResourceT(restypeid, resid);
		ResIter->entry.wstrName = strNewResName;
		
		return 0;
	}
}

#endif
