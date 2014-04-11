(* 
   Simple .mjs -> .mjo assembler.
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

open Printf
open ExtList
open ExtString

let bcarr = DynArray.create ()
and labels = Hashtbl.create 0
and entrypoints = DynArray.create ()
and offset = ref 0
and resstrs = Hashtbl.create 0

let add_bc x = DynArray.add bcarr (`Code x); offset := !offset + String.length x
and add_pt x = offset := !offset + 4; DynArray.add bcarr (`Pointer (x, !offset))
and add_ep x = DynArray.add entrypoints x
and add_lb x = Hashtbl.add labels x

let get_res i = 
  try
    Hashtbl.find resstrs i
  with Not_found ->
    eprintf "Warning: missing resource string <%04ld>\n" i;
    "(missing string)"

let string_of_i16 i = 
  sprintf "%c%c" 
    (char_of_int (i land 0xff))
    (char_of_int (i lsr 8 land 0xff))

let string_of_i32 i = 
  sprintf "%c%c%c%c" 
    (char_of_int (Int32.to_int (Int32.logand i 0xffl)))
    (char_of_int (Int32.to_int (Int32.logand (Int32.shift_right_logical i 8) 0xffl)))
    (char_of_int (Int32.to_int (Int32.logand (Int32.shift_right_logical i 16) 0xffl)))
    (char_of_int (Int32.to_int (Int32.logand (Int32.shift_right_logical i 24) 0xffl)))

let mjstring s = string_of_i16 (String.length s + 1) ^ s ^ "\000"

let rec process_cmd =
  function
    | `Entrypoint (b, i) -> add_ep (b, i, !offset)
    | `Label s -> add_lb s !offset
    | `Push e -> push e
    | `Exit es -> List.iter push (List.rev es); add_bc "\x2b\x08"
    | `Call (i, j, es)
       -> List.iter push (List.rev es);
          kprintf add_bc "\x10\x08%s%s%s" (string_of_i32 i) (string_of_i32 j) (string_of_i16 (List.length es))
    | `Syscall (i, es)
       -> List.iter push (List.rev es);
          kprintf add_bc "\x35\x08%s%s" (string_of_i32 i) (string_of_i16 (List.length es))
    | `Op ((0x829 | 0x836 as i), s) -> kprintf add_bc "%s%s%s" (string_of_i16 i) (string_of_i16 (String.length s)) s
    | `Op (i, s) -> kprintf add_bc "%s%s" (string_of_i16 i) s
    | `Jump (i, l, es)
       -> List.iter push (List.rev es);
          add_bc (string_of_i16 i);
          add_pt l
    | `Switch ls
       -> kprintf add_bc "\x50\x08%s" (string_of_i16 (List.length ls));
          List.iter add_pt ls
    | `TextCmd (i, es)
       -> List.iter push (List.rev es);
          kprintf add_bc "\x42\x08\x02\x00%s" (string_of_i16 i)
    | `Text (`Res i) -> process_cmd (`Text (`Str (get_res i)))
    | `Text (`Str s)
       -> add_bc
            (String.concat "\x42\x08\x02\x00\x6e\x00"
              (List.map 
                (fun s -> sprintf "\x40\x08%s\x41\x08" (mjstring s))
                (String.nsplit s "\n")))
and push =
  function
    | `Pop -> ()
    | `Int i -> kprintf add_bc "\x00\x08%s" (string_of_i32 i)
    | `Str s -> kprintf add_bc "\x01\x08%s" (mjstring s)
    | `Res i -> push (`Str (get_res i))
    | `Op ((0x829 | 0x836 as i), s) -> kprintf add_bc "%s%s%s" (string_of_i16 i) (string_of_i16 (String.length s)) s
    | `Op (i, s) -> kprintf add_bc "%s%s" (string_of_i16 i) s
    | `Syscall (i, es) 
        -> List.iter push (List.rev es);
           kprintf add_bc "\x34\x08%s%s" (string_of_i32 i) (string_of_i16 (List.length es))

let assemble inname outname resname =
  (* Load resources *)
  if resname <> "" && Sys.file_exists resname then (
    let ic = open_in resname in
    let lexbuf = Lexing.from_channel ic in
    lexbuf.Lexing.lex_curr_p <- { lexbuf.Lexing.lex_curr_p with Lexing.pos_fname = resname };
    MjsLex.read_resfile (Hashtbl.add resstrs) lexbuf;
    close_in ic
  );
  (* Parse *)
  let ic = open_in inname in
  let lexbuf = Lexing.from_channel ic in
  lexbuf.Lexing.lex_curr_p <- { lexbuf.Lexing.lex_curr_p with Lexing.pos_fname = inname };
  let statements = MjsParse.program MjsLex.lex lexbuf in
  close_in ic;
  (* Assemble *)
  DynArray.iter process_cmd statements;
  let entrypoint =
    try
      let offset = DynArray.index_of (fun (x, _, _) -> x) entrypoints in
      let _, _, rv = DynArray.unsafe_get entrypoints offset in
      rv
    with Not_found ->
      prerr_endline "Error: no #entrypoint directive";
      exit 2
  in
  let buf = Buffer.create 0 in
  DynArray.iter
    (function
      | `Code s -> Buffer.add_string buf s
      | `Pointer (id, offs) -> Buffer.add_string buf (string_of_i32 (Int32.of_int (Hashtbl.find labels id - offs))))
    bcarr;
  (* Encrypt *)
  let bytecode = Buffer.contents buf in
  for i = 0 to String.length bytecode - 1 do
    String.unsafe_set bytecode i 
      (char_of_int (int_of_char (String.unsafe_get bytecode i) lxor Array.unsafe_get Key.key (i mod 1024)))
  done;
  (* Output *)
  let oc = open_out_bin outname in
  output_string oc "MajiroObjX1.000\000";
  Util.write_int oc entrypoint;
  Util.write_int oc 0; (* should work if, as I suspect, this is just debug info *)
  Util.write_int oc (DynArray.length entrypoints);
  DynArray.iter (fun (_, id, offset) -> Util.write_int32 oc id; Util.write_int oc offset) entrypoints;
  Util.write_int oc !offset;
  output_string oc bytecode;
  close_out oc

let output_dir = ref "."
and output_name = ref ""
and resource_name = ref ""

let opts =
  function
    | "" 
       -> invalid_arg "-"
    | s when String.length s > 2 && s.[1] = '='
       -> let optref =
            match s.[0] with
              | 'd' -> output_dir
              | 'o' -> output_name
              | 'r' -> resource_name
              | c -> kprintf invalid_arg "-%c" c
          in
          optref := String.slice s ~first:2
    | s
       -> String.iter
            (function
              | '-' -> ()
              | 'h' | '?' -> failwith "usage"
              | c -> kprintf invalid_arg "-%c" c)
            s

let main =
  try match Util.getopts ["-d"; "-o"; "-r"] with
    | args, [file]
       -> List.iter opts args;
          let outname =
            let s = if !output_name = "" then file else !output_name in
            Filename.concat !output_dir (Filename.basename (try Filename.chop_extension s with _ -> s)) ^ ".mjo"
          and resname =
            if !resource_name = "" 
            then (try Filename.chop_extension file with _ -> file) ^ ".sjs"
            else !resource_name
          in
          assemble file outname resname
    | _ -> failwith "usage"
  with 
    Failure "usage" ->
      print_endline "Usage: mjasm [-d OUTDIR] [-o OUTNAME] [-r RESFILE] FILE";
      exit 1
