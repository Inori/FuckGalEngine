require"pack"

local byte = string.byte
local char = string.char
local bpack = string.pack
local bunpack = string.unpack

local BASEOFFSET = 0x3395aa
local INDEXOFFSET = 0x7a8f02

local item = {}
local offset = {}
local length = {}
local entry

local fsrc, fdst

local function Readchar()
	local a = fsrc:read(2)
	if not a then return nil end
	local _, x = bunpack(a, ">H")
	return x
end

local function Getoffset()
	fsrc:seek("set",INDEXOFFSET)
	_, entry = bunpack(fsrc:read(4), "I")
	for i=1, entry do
		_, offset[i] = bunpack(fsrc:read(4), "I")
	end
	table.insert(offset, INDEXOFFSET)
end

local function Getlength()
	for i=1, entry do
		length[i] = offset[i+1] - offset[i] - 2
	end
end

function hex(s)
	s=string.gsub(s,"(.)",function (x) return bpack("H",string.format("%02d",string.byte(x)) )end)
	return s
end

local function Main()
	fsrc = assert(io.open(arg[1], "rb"))
	fdst = assert(io.open(arg[2], "wb"))

	Getoffset()
	Getlength()

	local s=""
	local sp = false
	local dv = string.rep(bpack(">H", 0x2300), 4)
	local dk = bpack(">H", 0x2000)
	local de = bpack(">H", 0x0d00) .. bpack(">H", 0x0a00)

	for i=1, entry do
		fsrc:seek("set", BASEOFFSET+offset[i])
		for j=1, length[i]/2 do
			if not sp then
				local a = Readchar()
				if a == 0 then break end
				if a == 0x0700 then
					a = Readchar()
					if a == 0x0600 then s = s .. bpack(">H", 0xce25) -- "¡ò"
					elseif a == 0x0800 then s = s .. bpack(">H", 0xb225); sp = true-- "¡ø"
					elseif a == 0x0100 then s = s .. bpack(">H", 0x0526); sp = true-- "¡ï"
					elseif a == 0x0400 then s = s .. bpack(">H", 0xc625) -- "¡ô"
					elseif a == 0x0900 then s = s .. bpack(">H", 0xb325) -- "¡÷"
					else s = s .. bpack(">H", a)

					end
				elseif a == 0x0a00 then s = s .. bpack(">H", 0x0a00)
				else s = s .. bpack(">H", a) end

			else
				local a = Readchar()
				if a == 0x0700 then
					a = Readchar()
					if a == 0x0600 then s = s .. bpack(">H", 0xce25) -- "¡ò"
					elseif a == 0x0800 then s = s .. bpack(">H", 0xb225); sp = true-- "¡ø"
					elseif a == 0x0100 then s = s .. bpack(">H", 0x0526); sp = true-- "¡ï"
					elseif a == 0x0400 then s = s .. bpack(">H", 0xc625) -- "¡ô"
					elseif a == 0x0900 then s = s .. bpack(">H", 0xb325);sp = false -- "¡÷"
					else s = s .. bpack(">H", a)
					end
				elseif a == 0x0a00 then s = s .. bpack(">H", 0xbc25) -- "¨‹"
				elseif a == 0x0000 then s = s .. bpack(">H", 0xbd25);sp = false -- "¨Œ"
				else s = s .. bpack(">H", a)
				end
			end
		end

		item[i] = dv .. dk .. hex(i) ..dk .. dv .. de .. s .. de .. de
		s = ""
	end

	fdst:write("\255\254")

	for i=1, #item do
		assert(fdst:write(item[i]))
	end
	print("OK!")

end

Main()
