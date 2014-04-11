
;IUnknown 接口定义

externdef IID_IUnknown:IID
LPUNKNOWN typedef DWORD
LPPUNKNOWN typedef ptr LPUNKNOWN

IUnknown_QueryInterfaceProto typedef proto :dword,:dword,:dword
IUnknown_AddRefProto typedef proto :dword
IUnknown_ReleaseProto typedef proto :dword
IUnknown_QueryInterface typedef ptr IUnknown_QueryInterfaceProto
IUnknown_AddRef typedef ptr IUnknown_AddRefProto
IUnknown_Release typedef ptr IUnknown_ReleaseProto

IUnknown struct dword
	QueryInterface IUnknown_QueryInterface ?
	AddRef IUnknown_AddRef ?
	Release IUnknown_Release ?
IUnknown ends

;IMalloc 接口定义
externdef IID_IMalloc:IID
LPMALLOC typedef dword
LPPMALLOC typedef ptr LPMALLOC

IMalloc_AllocProto typedef proto :dword,:dword
IMalloc_ReallocProto typedef proto :dword,:dword,:dword
IMalloc_FreeProto typedef proto :dword,:dword
IMalloc_GetSizeProto typedef proto :dword,:dword
IMalloc_DidAllocProto typedef proto :dword,:dword
IMalloc_HeapMinimizeProto typedef proto :dword,:dword

IMalloc_Alloc typedef ptr IMalloc_AllocProto
IMalloc_Realloc typedef ptr IMalloc_ReallocProto
IMalloc_Free typedef ptr IMalloc_FreeProto
IMalloc_GetSize typedef ptr IMalloc_GetSizeProto
IMalloc_DidAlloc typedef ptr IMalloc_DidAllocProto
IMalloc_HeapMinimize typedef ptr IMalloc_AllocProto
IMalloc_Alloc typedef ptr IMalloc_AllocProto

IMalloc struct dword
	QueryInterface IUnknown_QueryInterface ?
	AddRef IUnknown_AddRef ?
	Release IUnknown_Release ?
	Alloc IMalloc_Alloc ?
	Realloc IMalloc_Realloc ?
	Free IMalloc_Free ?
	GetSize IMalloc_GetSize ?
	DidAlloc IMalloc_DidAlloc ?
	HeapMinimize IMalloc_HeapMinimize ?
IMalloc ends

;IStream接口定义
externdef IID_IStream:IID
LPSTREAM typedef dword
LPPSTREAM typedef ptr LPSTREAM

IStream struct dword
	QueryInterface IUnknown_QueryInterface ?
	AddRef IUnknown_AddRef ?
	Release IUnknown_Release ?
	Read		dd		?
	Write		dd		?
	Seek		dd		?
	SetSize		dd		?
	CopyTo		dd		?
	Commit		dd		?
	Revert		dd		?
	LockRegion	dd		?
	UnlockRegion	dd		?
	Stat			dd		?
	Clone		dd		?
IStream ends

;IPicture接口定义

.data
	IID_IPicture db 80h,9,0f8h,7bh,32h,0bfh,1ah,10h,8bh,0bbh,0,0aah,0,30h,0ch,0abh
;externdef IID_IPicture:IID
LPPICTURE	typedef	dword
LPPPICTURE	typedef ptr LPPICTURE

IPicture_get_WidthProto typedef proto :dword,:dword
IPicture_get_HeightProto typedef proto :dword,:dword
IPicture_RenderProto typedef proto :dword,:dword,:dword,:dword,:dword,:dword,:dword,:dword,:dword,:dword,:dword

IPicture_get_Width typedef ptr IPicture_get_WidthProto
IPicture_get_Height typedef ptr IPicture_get_HeightProto
IPicture_Render typedef ptr IPicture_RenderProto

IPicture struct dword
	QueryInterface IUnknown_QueryInterface ?
	AddRef IUnknown_AddRef ?
	Release IUnknown_Release ?
	get_Handle		dd		?
	get_hPal			dd		?
	get_Type			dd		?
	get_Width		IPicture_get_Width		?
	get_Height		IPicture_get_Height	?
	Render			IPicture_Render		?
	set_hPal			dd		?
	get_CurDC		dd		?
	SelectPicture		dd		?
	get_KeepOriginalFormat	dd		?
	put_KeepOriginalFormat	dd		?
	PictureChanged	dd		?
	SaveAsFile		dd		?
	get_Attributes		dd		?
IPicture ends


