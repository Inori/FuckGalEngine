(*
   AST handling for .mjs files (Kepagoesque).
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

type statement =
  [
  | `Entrypoint of bool * int32
  | `Label of string
  | `Push of expr
  | `Exit of expr list
  | `Call of int32 * int32 * expr list
  | `Syscall of int32 * expr list
  | `Jump of int * string * expr list
  | `Switch of string list
  | `Op of int * string
  | `Text of [ `Str of string | `Res of int32 ] 
  | `TextCmd of int * expr list
  ]

and expr =
  [
  | `Pop
  | `Int of int32
  | `Str of string
  | `Res of int32
  | `Syscall of int32 * expr list
  | `Op of int * string
  ] 


(* if debugging *)
(*
open Printf

let rec string_of_expr =
  function
    | `Pop -> "pop"
    | `Int i -> Int32.to_string i
    | `Str s -> sprintf "'%s'" (Util.escape s)
    | `Res i -> sprintf "#res<%04ld>" i
    | `Syscall (i, es) -> sprintf "syscall<$%08lx>(%s)" i (string_of_exprs es)
    | `Op (i, bs) -> sprintf "op%03x%s" i (if bs = "" then "" else sprintf "[#%s]" (ExtString.String.replace_chars (fun c -> sprintf "%02x" (int_of_char c)) bs))

and string_of_exprs l = String.concat ", " (List.map string_of_expr l)

and string_of_statement : statement -> string =
  function
    | `Entrypoint (b, i) -> sprintf "#%s $%08lx" (if b then "entrypoint" else "function") i
    | `Label l -> "@" ^ l
    | `Push e -> "push " ^ string_of_expr e
    | `Exit es -> sprintf "exit (%s)" (string_of_exprs es)
    | `Call (i, j, es) -> sprintf "call<$%08lx, %ld> (%s)" i j (string_of_exprs es)
    | `Syscall (i, es) -> sprintf "syscall<$%08lx> (%s)" i (string_of_exprs es)
    | `Jump (i, l, es) -> sprintf "jmp%03x (%s) @%s" i (string_of_exprs es) l
    | `Switch ss -> sprintf "switch { %s }" (String.concat ", " (List.map ((^) "@") ss))
    | `Op (i, bs) -> sprintf "op%03x%s" i (if bs = "" then "" else sprintf "[#%s]" (ExtString.String.replace_chars (fun c -> sprintf "%02x" (int_of_char c)) bs))
    | `Text t -> string_of_expr (t :> expr)
    | `TextCmd (i, es) -> sprintf "text_control_%02x (%s)" i (string_of_exprs es)

let print_statements = DynArray.iter (fun smt -> print_endline (string_of_statement smt))
*)
