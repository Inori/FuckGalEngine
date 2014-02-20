--The file must be encoded by UTF-8
--提取psp版dies irae文本
--This is a quick-and-dirty script, don't mind some details
--script by 福音,Thanks to dwing

local ffi = require("ffi")
local C = ffi.C
local ffichars = ffi.typeof("uint8_t[?]")
local ffistr = ffi.string

local INDEXOFFSET = 0x31c450
local BASEOFFSET = 0x36a32e
local ENTRY = (BASEOFFSET-INDEXOFFSET+2) / 8 - 1

local byte = string.byte
local char = string.char

local fsrc, fdst
local iteam = {}
local offset = {}
local length = {}

ffi.cdef[[ int __stdcall MultiByteToWideChar(int cp, int flag, const char* src, int srclen, char* wdst, int wdstlen);
int __stdcall WideCharToMultiByte(int cp, int flag, const char* wsrc, int wsrclen, char* dst, int dstlen, const char* defchar, int* defused);]]

local function mb2wc(src, cp)
	local srclen = #src
	local dst = ffichars(srclen * 2)
	return ffistr(dst, C.MultiByteToWideChar(cp or 65001, 0, src, srclen, dst, srclen) * 2)
end

local function wc2mb(src, cp)
	local srclen = #src / 2
	local dstlen = srclen * 3
	local dst = ffichars(dstlen)
	return ffistr(dst, C.WideCharToMultiByte(cp or 65001, 0, src, srclen, dst, dstlen, nil, nil))
end

local function SjisToUTF8Str(s)
	return wc2mb(mb2wc(s, 932), 65001)
end

local function Readchar()
	s = fsrc:read(1)
	if not s or #s < 1 then return 0 end
	return byte(s)
end

local function GetCharType(c)
	if c == 0x07 then return 1 end
	if (c >= 0x20 and c <= 0x7e) or (c >= 0xa1 and c <= 0xdf) then return 2 end
	if (c >= 0x81 and c <= 0x9f) or (c >= 0xe0 and c <= 0xfc) then return 3 end
	if (c == 0x00 or  c == 0x0a or  c == 0x0d) then return 2 end
	return 0
end

local function IsSjisSecondChar(c)
	return c and c >= 0x40 and c <= 0xfc and c ~= 0x7f
end

local function read3byte()
	local a, b, c = fsrc:read(3):byte(1, 3)
	return a + b * 0x100 + c * 0x10000
end

local function GetIndex()
	fsrc:seek("set", INDEXOFFSET+1)

	for i=1, ENTRY do
	fsrc:seek(cur, 1)
	offset[i] = read3byte()
	fsrc:seek(cur, 1)
	length[i] = read3byte()
	end
end


local function Close()
	fsrc:close()
	fdst:close()
end


function Main()

	fsrc = assert(io.open(arg[1], "rb"))
	fdst = assert(io.open(arg[2], "wb"))

	GetIndex()

	local s=""
	local sp = false
	local haswidechar = false
	local j

	for i=1, ENTRY do
		fsrc:seek("set", BASEOFFSET+offset[i])

		for j=1, length[i]  do
			if not sp then
				local a = Readchar()
				if a == 0 then break end
				local tp = GetCharType(a)
				if tp == 1 then
					a = Readchar()
					if a == 0x08 then s = s .. char(0x81, 0xa3); sp = true     -- "▲"
					elseif a == 0x09 then s = s .. char(0x81, 0xa2)            --"△"
					elseif a == 0x06 then s = s .. char(0x81, 0x9d);           --"◎"
					elseif a == 0x01 then s = s .. char(0x81, 0x9a); sp = true --"★"
					elseif a == 0x04 then s = s .. char(0x81, 0x9f)            --"◆"
					else s = s .. char(a)
					end

				elseif tp == 2 then
					if a == 0x0a then s = s .. "\n"
					elseif a == 0x0d then s = s .. "\r"
					else s = s .. char(a) end
					if a >= 0x80 then haswidechar = true end
				elseif tp == 3 then
					s = s .. char(a)
					a = Readchar()
					if IsSjisSecondChar(a) then
						s = s .. char(a)
						haswidechar = true
					else
						haswidechar = false
					end
				end
			else
				local a = Readchar()
				local tp = GetCharType(a)
				if tp == 1 then
					a = Readchar()
					if a == 0x08 then s = s .. char(0x81, 0xa3); sp = true     -- "▲"
					elseif a == 0x09 then s = s .. char(0x81, 0xa2)            --"△"
					elseif a == 0x06 then s = s .. char(0x81, 0x9d)             --"◎"
					elseif a == 0x01 then s = s .. char(0x81, 0x9a); sp = true --"★"
					elseif a == 0x04 then s = s .. char(0x81, 0x9f)            --"◆"
					else s = s .. char(a)
					end
				elseif tp == 2 then
					if a == 0x0a then s = s .. char(0x81, 0xa5)                 -- "▼"
					elseif a == 0x0d then s = s .. "\r"
					elseif a == 0x00 then s = s .. char(0x81, 0xa4);sp = false --"▽"
					else s = s .. char(a) end
					if a >= 0x80 then haswidechar = true end
				elseif tp == 3 then
					s = s .. char(a)
					a = Readchar()
					if IsSjisSecondChar(a) then
						s = s .. char(a)
						haswidechar = true
					else
						haswidechar = false
					end
				end
			end
		end
--		if haswidechar then
		iteam[#iteam + 1] = string.format("#### %d ####\r\n%s\r\n\r\n", i, SjisToUTF8Str(s))
		s = ""
--		end
	end

	fdst:write("\239\187\191")
	for i=1, #iteam do
		assert(fdst:write(iteam[i]))
	end


	Close();
end

Main()
