/* 
   Parser for .mjs files (Kepagoesque).
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
*/

%{
open Printf
open Parsing
%}

%token EOF
%token COMMA LSQU RSQU LPAR RPAR LCUR RCUR LTN GTN
%token<string> BYTES STR LABEL
%token<int32> INT
%token DRES POP
%token<bool> DFUNCTION 
%token<int> OP JMP TEXTCMD
%token GOTO JNE SWITCH
%token PUSH CALL SYSCALL EXIT
%token<string> IDENT

%start program
%type<MjsAst.statement DynArray.t> program

%%

program: statements EOF { $1 }

statements: /* empty */  { DynArray.create () }
  | statements statement { DynArray.add $1 $2; $1 }

statement:
  | DFUNCTION INT { `Entrypoint ($1, $2) }
  | LABEL       { `Label $1 }
  | CALL LTN INT COMMA INT GTN params { `Call ($3, $5, $7) }
  | EXIT params { `Exit $2 }
  | PUSH expr   { `Push $2 }
  | syscall     { `Syscall $1 }
  | resstr      { `Text (`Res $1) }  
  | STR         { `Text (`Str $1) }
  | unknownop   { `Op $1 }
  | GOTO LABEL  { `Jump (0x82c, $2, []) }
  | JMP params LABEL { `Jump ($1, $3, $2) }
  | SWITCH LCUR labels RCUR { `Switch $3 }
  | TEXTCMD params { `TextCmd ($1, $2) }
  | IDENT { eprintf "Undeclared identifier `%s'\n" $1; exit 2 }

labels: LABEL          { $1 :: [] }
  | LABEL COMMA labels { $1 :: $3 }

params: /* empty */ { [] }
  | LPAR RPAR { [] }
  | LPAR param_list RPAR { $2 }

param_list:    expr       { $1 :: [] }
  | expr COMMA param_list { $1 :: $3 }

expr:
  | POP       { `Pop }
  | INT       { `Int $1 }
  | STR       { `Str $1 }
  | resstr    { `Res $1 }
  | syscall   { `Syscall $1 }
  | unknownop { `Op $1 }
  | IDENT { eprintf "Undeclared identifier `%s'\n" $1; exit 2 }

syscall: 
  SYSCALL LTN INT GTN params 
    { $3, $5 }

resstr: 
  DRES LTN INT GTN 
    { $3 }

unknownop:
  | OP           { $1, "" }
  | OP LSQU RSQU { $1, "" }
  | OP BYTES     { $1, $2 }