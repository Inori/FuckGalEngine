(* 
   Process Majiro archive files.
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

type header =
  { code: int32;
    offset: int;
    length: int;
    filename: string; }

(*
  Header format:
  
    00  char[16]  "MajiroArcV1.000\000"
    10  int       file count
    14  int       offset of filenames
    18  int       offset of data
    
    1c  rec[file count + 1] 
      00  char[4] ???
      04  int     offset of data
      
      for last record, 00 is 00 00 00 00 and 04 is the offset of EOF
  
    Followed by filenames (packed, null-terminated array) and data.
*)
let archeader fname =
  let ic = open_in_bin fname in
  if Util.read_str ic 16 <> "MajiroArcV1.000\000" then invalid_arg "archeader";
  let file_count = Util.read_int ic in
  seek_in ic (pos_in ic + 8);
  let index = Array.init (file_count + 1) (fun _ -> let i = Util.read_int32 ic in i, Util.read_int ic) in
  let rec loop acc =
    function
      | [] -> assert false 
      | [_] -> acc
      | (code, offset) :: ((_, next) :: _ as rest)
       -> let curr =
            { code = code; 
              offset = offset; 
              length = next - offset;
              filename = Util.read_cstr ic }
          in 
          loop (curr :: acc) rest
  in
  let rv = List.rev (loop [] (Array.to_list index)) in
  close_in ic;
  rv

let copy_to_channel =
  let buf = String.create Util.buf_size in
  fun ic oc offset length ->
    seek_in ic offset;
    let rec loop rem =
      if rem > 0 then
        let read = input ic buf 0 (min rem Util.buf_size) in
        output oc buf 0 read;
        loop (rem - read)
    in
    loop length
      
let split_archive outdir fname index =
  let ic = open_in_bin fname in
  List.iter
    (fun entry ->
      let oc = open_out_bin (Filename.concat outdir entry.filename) in
      copy_to_channel ic oc entry.offset entry.length;
      close_out oc)
    index;
  close_in ic

let read_arc action fname files =
  let fname, files, lcodes = if fname = "-c" then List.hd files, List.tl files, true else fname, files, false in
  let index = 
    (if files = [] then Util.id else List.filter (fun { filename = fn } -> List.mem fn files)) 
      (archeader fname)
  in
  match action with
    | `X outdir -> split_archive outdir fname index
    | `L cmp -> List.iter 
                (fun entry -> 
                  printf "%-18s " entry.filename;
                  if lcodes then printf " %08lx " entry.code;
                  printf "%8.2f kb\n" (float entry.length /. 1024.))
                (List.sort ~cmp index)

let write_arc fname addfiles =
  let index, ic = 
    if Sys.file_exists fname 
    then let i = archeader fname in i, open_in_bin fname
    else [], stdin
  in
  let tmpfile = 
    let dir = Filename.dirname fname and base = Filename.basename fname in
    let rec loop i =
      let s = kprintf (Filename.concat dir) "~mjarc#%s#%03d.tmp" base i in
      if Sys.file_exists s then loop (i + 1) else s
    in loop 0
  in
  let addbasenames = List.map Filename.basename addfiles in
  let keep = List.filter (fun { filename = fn } -> not (List.mem fn addbasenames)) index in
  let names = List.map (fun { filename = fn } -> fn) keep @ addbasenames in
  let file_count, ftab_length = List.fold_left (fun (c, l) elt -> c + 1, l + String.length elt + 1) (0, 0) names in
  let ftab_offset = 28 + (file_count + 1) * 8 in
  let oc = open_out_bin tmpfile in
  begin try
    output_string oc "MajiroArcV1.000\000";
    Util.write_int oc file_count;
    Util.write_int oc ftab_offset;
    Util.write_int oc (ftab_offset + ftab_length);
    seek_out oc ftab_offset;
    List.iter (fun s -> output_string oc s; output_char oc '\000') names;
    let kept_entries =
      List.map
        (fun entry ->
          let offset = pos_out oc in
          copy_to_channel ic oc entry.offset entry.length;
          entry.code, offset)
        keep
    in
    let added_entries =
      List.map
        (fun s -> 
          let offset = pos_out oc in
          let ic = open_in_bin s in
          copy_to_channel ic oc 0 (in_channel_length ic);
          0x7ea1eaf5l (* TODO: figure out what this number is and calculate it *), offset)
        addfiles
    in
    let all_entries = kept_entries @ added_entries @ [0l, pos_out oc] in
    seek_out oc 28;
    List.iter 
      (fun (code, offset) -> 
        Util.write_int32 oc code;
        Util.write_int oc offset)
      all_entries
  with e ->
    (try close_out oc; with _ -> ());
    (try Sys.remove tmpfile with _ -> ());
    raise e
  end;
  close_out oc;
  close_in ic;
  begin try
    Sys.remove fname;
  with e ->
    if not (Sys.file_exists fname) then print_endline "Oops, you've lost your data due to the following error:";
    raise e
  end;
  Sys.rename tmpfile fname

let main = 
  try 
    let args, files = Util.getopts ["-d"] in
    if files = [] then failwith "usage";
    let action = ref `None 
    and sort = ref `Name 
    and reverse = ref false
    and outdir = ref "." in
    List.iter
     (function
       | "" -> invalid_arg "-"
       | s when String.length s > 2 && String.sub s 0 2 = "d="
          -> outdir := String.sub s 2 (String.length s - 2)
       | s
          -> String.iter
               (function
                 | '-' -> ()
                 | 'h' | '?' -> failwith "usage"
                 | 'a' -> action := `Write
                 | 'x' -> action := `Extract
                 | 'l' -> action := `List
                 | 's' -> sort := `Size
                 | 'u' -> sort := `None
                 | 'r' -> reverse := true
                 | c -> kprintf invalid_arg "-%c" c)
               s)
      args;
    (match !action with
      | `None -> failwith "usage"
      | `Write -> write_arc
      | `Extract -> read_arc (`X !outdir)
      | `List -> read_arc (`L
                   (match !sort, !reverse with
                     | `None, false -> (fun _ _ -> 0)
                     | `None, true  -> (fun _ _ -> 1)
                     | `Name, false -> (fun { filename = fa } { filename = fb } -> String.compare fa fb)
                     | `Name, true  -> (fun { filename = fa } { filename = fb } -> String.compare fb fa)
                     | `Size, false -> (fun { length = la } { length = lb } -> compare la lb)
                     | `Size, true  -> (fun { length = la } { length = lb } -> compare lb la))))
      (List.hd files) (List.tl files)
  with
    | Invalid_argument s ->
        eprintf "Invalid option `%s'\nUsage: mjarc -a/-x/-l [-rsu] [-d OUTDIR] ARCHIVE [FILES]" s;
        exit 1
    | Failure "usage" ->
        prerr_endline "Usage: mjarc -a/-x/-l [-rsu] [-d OUTDIR] ARCHIVE [FILES]";
        exit 1
