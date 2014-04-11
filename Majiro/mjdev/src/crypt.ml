(* 
   Simple program to encrypt/decrypt Majiro bytecode files.
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

let do_crypt fname outfile =
  let ic = open_in_bin fname in
  let decrypting, length, offset = Mjo.header ic in
  seek_in ic 16;
  let oldheader = Util.read_str ic (offset - 16) in
  let crypted = Mjo.crypt ic in
  close_in ic;
  let oc = open_out_bin outfile in
  output_string oc (if decrypting then "MjPlainBytecode\000" else "MajiroObjX1.000\000");
  output_string oc oldheader;
  output_string oc crypted;
  close_out oc

let main = 
  match Sys.argv with
    | [| _; filename |] -> do_crypt filename filename
    | [| _; filename; outfile |] -> do_crypt filename outfile
    | _ -> print_endline "Usage: mjcrypt INFILE [OUTFILE]"
