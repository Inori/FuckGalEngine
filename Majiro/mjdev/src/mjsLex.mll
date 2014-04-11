(* 
   Lexer for .mjs files (Kepagoesque) and accompanying resource files.
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

{ (* Caml header *)

open ExtString
open Printf
open MjsParse
open Lexing

let _raw_bytes s =
  let s = if String.length s mod 2 == 1 then "0" ^ s else s
  and buf = "0x__" in
  String.init 
    (String.length s / 2) 
    (fun i -> String.unsafe_blit s (i * 2) buf 2 2; char_of_int (int_of_string buf))  

let _identifier =
  let keywords = Hashtbl.create 0 in
  Array.iter (fun (k, v) -> Hashtbl.add keywords k v) [| 
    "#entrypoint", DFUNCTION true;
    "#function",   DFUNCTION false;
    "#res",        DRES;
    "call",        CALL; 
    "exit",        EXIT;
    "goto",        GOTO;
    "jne",         JMP 0x831;
    "pop",         POP;
    "push",        PUSH;
    "switch",      SWITCH;
    "syscall",     SYSCALL;
    "text_control_66", TEXTCMD 0x66;
    "br",              TEXTCMD 0x6e;
    "text_control_6f", TEXTCMD 0x6f;
    "pause",           TEXTCMD 0x70;
    "text_control_72", TEXTCMD 0x72;
    "text_control_73", TEXTCMD 0x73;
    "cls",             TEXTCMD 0x77;
  |];
  fun s ->
    try
      Hashtbl.find keywords s
    with Not_found ->
      try
        if String.length s = 5 && String.starts_with s "op" then 
          OP (int_of_string ("0x" ^ String.slice s ~first:~-3))
        else if String.length s = 6 && String.starts_with s "jmp" then
          JMP (int_of_string ("0x" ^ String.slice s ~first:~-3))
        else
          raise Not_found
      with Not_found ->
        IDENT s

} (* End of Caml header *)

let sjs1 = ['\x81'-'\x9f' '\xe0'-'\xef' '\xf0'-'\xfc']
let sjs2 = ['\x40'-'\x7e' '\x80'-'\xfc']
let int = ['0'-'9']
let hex = ['0'-'9' 'A'-'F' 'a'-'f']
let alpha = ['A'-'Z' 'a'-'z' '_']
let alnum = (alpha | int)
let sp = [' ' '\t' '\r']

rule comment spos =
  parse
    | eof { eprintf "Error (%s line %d): unterminated comment" spos.pos_fname spos.pos_lnum; exit 2 }
    | "-}" {}
    | [^ '-' '\n']+ | '-' { comment spos lexbuf }
    | '\n' { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 };
             comment spos lexbuf }

and eol =
  parse
    | [^ '\n']* '\n' { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 } }
    | _ | eof        {}

and lex = 
  parse
    | eof { EOF }
    | sp+ { lex lexbuf }
    | ("//" [^ '\n']*)? "\n" 
      { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 };
        lex lexbuf }
    | "{-" { comment lexbuf.lex_curr_p lexbuf;
             lex lexbuf }

    | ',' { COMMA }
    | '[' { LSQU } | ']' { RSQU } | '(' { LPAR } | ')' { RPAR }
    | '{' { LCUR } | '}' { RCUR } | '<' { LTN }  | '>' { GTN }

    | "[#" (hex+ as raw) "]" { BYTES (_raw_bytes raw) }
    | '$' (hex+ as num) { INT (kprintf Int32.of_string "0x%s" num) }
    | '-'? int+ { INT (Int32.of_string (lexeme lexbuf)) }
    | "'" | "\"" { STR (fst (string lexbuf.lex_curr_p (Buffer.create 8) (lexeme_char lexbuf 0) lexbuf)) }
    | (alpha | '#') alnum* { _identifier (lexeme lexbuf) }
    | '@' (alnum+ as id) { LABEL id }

and string spos buf delim =
  parse
    | "'" | "\"" | "<"
      { let l = lexeme_char lexbuf 0 in
        if l == delim 
        then (Buffer.contents buf, false)
        else (Buffer.add_char buf l; string spos buf delim lexbuf) }

    | "{-"
      { if delim = '<' then comment lexbuf.lex_curr_p lexbuf else Buffer.add_string buf "{-";
        string spos buf delim lexbuf }

    | "//"
      { if delim = '<' then eol lexbuf else Buffer.add_string buf "//";
        string spos buf delim lexbuf }

    | sp* '\n' sp* 
      { Buffer.add_char buf ' ';
        lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 };
        string spos buf delim lexbuf }
    | "\\" sp* '\n' sp*
      { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 };
        string spos buf delim lexbuf }

    | "\\n" { Buffer.add_char buf '\n'; string spos buf delim lexbuf }
    | "\\_" { Buffer.add_char buf ' '; string spos buf delim lexbuf }
    | "\\"_ { Buffer.add_char buf (lexeme_char lexbuf 1); string spos buf delim lexbuf }

    | sp+ | ([^ '\'' '\"' '<' '{' '/' '\\' ' ' '\t' '\n']#sjs1)+ | sjs1 sjs2 | "{" | "/"
        { Buffer.add_string buf (lexeme lexbuf); string spos buf delim lexbuf }
    
    | "\\"? eof 
        { if delim = '<'
          then (Buffer.contents buf, true)
          else (eprintf "Error (%s line %d): unterminated string" spos.pos_fname spos.pos_lnum; exit 2) }

and read_resfile add =
  parse
    | eof {}
    | sp+ { read_resfile add lexbuf }
    | ("//" [^ '\n']*)? "\n" 
      { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 };
        read_resfile add lexbuf }
    | "{-" { comment lexbuf.lex_curr_p lexbuf; read_resfile add lexbuf }
    | "<" { read_resstrs add lexbuf }
    | _ { eprintf "Error (%s line %d): illegal character `%c'\n" lexbuf.lex_curr_p.pos_fname lexbuf.lex_curr_p.pos_lnum (lexeme_char lexbuf 0);
          exit 2 }

and read_resstrs add =
  parse
    | sp* (int+ as i | '$' (hex+ as h))? sp* ">"
      { let key = 
          match i, h with
            | Some i, None -> Int32.of_string i
            | None, Some h -> kprintf Int32.of_string "0x%s" h
            | _ -> eprintf "Error (%s line %d): anonymous resstrings are not implemented" lexbuf.lex_curr_p.pos_fname lexbuf.lex_curr_p.pos_lnum;
                   exit 2
        in
        let str, last = resstring lexbuf in
        add key str;
        if not last then read_resstrs add lexbuf }
    | _ { eprintf "Error (%s line %d): illegal character `%c'\n" lexbuf.lex_curr_p.pos_fname lexbuf.lex_curr_p.pos_lnum (lexeme_char lexbuf 0);
          exit 2 }

and resstring =
  parse
    | sp* 
      { string lexbuf.lex_curr_p (Buffer.create 8) '<' lexbuf }
    | sp* '\n' 
      { lexbuf.lex_curr_p <- { lexbuf.lex_curr_p with pos_lnum = lexbuf.lex_curr_p.pos_lnum + 1 }; resstring lexbuf }
