(* 
   Utility functions and definitions.
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
   

let id x = x

let buf_size = 4_194_304 (* use 4MB copy buffers *)

let read_str ic len =
  let rv = String.create len in
  let rec loop offs rem =
    if rem = 0 then rv else
      let read = input ic rv offs rem in
      loop (offs + read) (rem - read)
  in
  loop 0 len

let read_cstr ic =
  let buf = Buffer.create 16 in
  let rec loop () =
    let c = input_char ic in
    if c = '\000' then Buffer.contents buf else loop (Buffer.add_char buf c)
  in
  loop ()

let read_int ic =
  let c1 = input_byte ic in
  let c2 = input_byte ic in
  let c3 = input_byte ic in
  let c4 = input_byte ic in
  c1 lor (c2 lsl 8) lor (c3 lsl 16) lor (c4 lsl 24)

let read_int32 ic =
  let c1 = input_byte ic in
  let c2 = input_byte ic in
  let c3 = input_byte ic in
  let c4 = input_byte ic in
  Int32.logor
    (Int32.of_int c1)
    (Int32.logor
      (Int32.shift_left (Int32.of_int c2) 8)
      (Int32.logor
        (Int32.shift_left (Int32.of_int c3) 16)
        (Int32.shift_left (Int32.of_int c4) 24)))

let write_int oc i =
  output_byte oc (i land 0xff);
  output_byte oc ((i lsr 8) land 0xff);
  output_byte oc ((i lsr 16) land 0xff);
  output_byte oc ((i lsr 24) land 0xff)

let write_int32 oc i =
  output_byte oc (Int32.to_int (Int32.logand i 0xffl));
  output_byte oc (Int32.to_int (Int32.logand (Int32.shift_right_logical i 8) 0xffl));
  output_byte oc (Int32.to_int (Int32.logand (Int32.shift_right_logical i 16) 0xffl));
  output_byte oc (Int32.to_int (Int32.logand (Int32.shift_right_logical i 24) 0xffl))

let escape s =
  let b = Buffer.create (String.length s) in
  let rec loop i =
    if i = String.length s 
    then Buffer.contents b
    else match s.[i] with
      | '\'' -> Buffer.add_char b '\\'; Buffer.add_char b s.[i]; loop (i + 1)
      | '\x81'..'\x9f' | '\xe0'..'\xef' | '\xf0'..'\xfc' 
        -> Buffer.add_char b s.[i];
           if match s.[i + 1] with '\x40'..'\x7e' | '\x80'..'\xfc' -> true | _ -> false 
           then (Buffer.add_char b s.[i + 1]; loop (i + 2))
           else loop (i + 1)
      | c -> Buffer.add_char b c; loop (i + 1)
  in
  loop 0

let getopts argopts =
  let cl = List.tl (Array.to_list Sys.argv) in
  let rec loop acc =
    function
      | [] as rem
      | "--" :: rem -> List.rev acc, rem
      | hd :: possarg :: tl when List.mem hd argopts -> loop ((hd ^ "=" ^ possarg) :: acc) tl
      | hd :: tl -> loop (hd :: acc) tl
  in
  let withargs, allfiles = loop [] cl in
  let args, files = List.partition (fun s -> s <> "" && s.[0] = '-') withargs in
  List.map (fun s -> String.sub s 1 (String.length s - 1)) args, files @ allfiles
