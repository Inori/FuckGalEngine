(* 
   Process Majiro bytecode files, including encryption.
   Copyright (c) 2005 Haeleth.
   
   This program is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation; either version 2 of the License, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   details.

   You should have received a copy of the GNU General Public License along with
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple
   Place - Suite 330, Boston, MA  02111-1307, USA.
*)

open Key

(*
  MJO header format:
  
  00  char[16]  "MajiroObjX1.000\000"
  10  int       offset of ?default entrypoint (seems always to be the last in the file)
  14  int       ?lines in source code (= the int16 after the last 0x83a opcode)
  18  int       entrypoint count
  
  1c  entrypoint records
    00  int     function name hash?
    04  int     offset of entrypoint
  
  __  int       data length
      
  followed by data ("encrypted" with cyclic XOR against Key.key)
*)
let header ic =
  let encrypted =
    match Util.read_str ic 16 with
      | "MajiroObjX1.000\000" -> true
      | "MjPlainBytecode\000" -> false
      | _ -> invalid_arg "mjoheader"
  in
  (* For now, just return data offset. *)
  seek_in ic (pos_in ic + 8);
  let record_count = Util.read_int ic in
  seek_in ic (0x1c + record_count * 8);
  let length = Util.read_int ic in
  encrypted, length, 0x20 + record_count * 8

let crypt ic =
  seek_in ic 0;
  let _, length, offset = header ic in
  let result = String.create length
  and buffer = String.create Util.buf_size in
  seek_in ic offset;
  let rec loop pos rem =
    if rem == 0 then
      result
    else
      let read = input ic buffer 0 (min rem Util.buf_size) in
      for i = 0 to read - 1 do
        let j = pos + i in
        result.[j] <- char_of_int (int_of_char buffer.[i] lxor Key.key.(j mod 1024))
      done;
      loop (pos + read) (rem - read)
  in
  loop 0 length
