(* 
   Disassemble Majiro bytecode files to a Kepagoesque form.
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
module ISet = Set.Make (Int32)
module IMap = Map.Make (Int32)

let include_markers = ref false (* 83a is source line numbers, 841 is apparently save points *)
and include_offsets = ref `None
and separate_strings = ref `Yes
and output_dir = ref Filename.current_dir_name

class resstr_handler =
  object (self)
    method enabled = true
    val mutable count = 0
    method private last = sprintf "#res<%04d>" count
  end

let resstrs fname =
  match !separate_strings with
    | `Yes ->
    (* Object to handle resource string output *)
    object (self)
      inherit resstr_handler
      val mutable chan = None
      method private write s =
        let oc = match chan with Some c -> c | None -> let c = open_out fname in chan <- Some c; c in
        let pre, post =
          if s = "" 
          then "", "" 
          else (if s.[0] = ' ' then "\\" else ""), (if s.[String.length s - 1] = ' ' then "\\" else "")
        in
        fprintf oc "<%04d> %s%s%s\n" count pre s post

      method add_whole s =
        count <- count + 1;
        self#write s;
        self#last

      val buf = Buffer.create 0
      method add_part s =
        if Buffer.length buf == 0 
        then (count <- count + 1; Buffer.add_string buf s; self#last)
        else (Buffer.add_string buf s; "")
      method finalise () =
        if Buffer.length buf > 0 then self#write (Buffer.contents buf);
        Buffer.clear buf

      method close () =
        self#finalise ();
        Option.may close_out chan
    end
  | `Suppress ->
    (* Stub object for when we don't want to clobber the resource file *)
    object
      inherit resstr_handler as self
      val mutable finalised = true
      method add_whole _ = count <- count + 1; self#last
      method add_part _ = if finalised then (count <- count + 1; finalised <- false; self#last) else ""
      method finalise () = finalised <- true
      method close () = ()
    end
  | `No ->
    (* Stub object for when resstrs aren't wanted at all *)
    object
      method enabled = false
      method add_whole _ = ""
      method add_part _ = ""
      method finalise () = ()
      method close () = ()
    end
  
let read_mjstring ic =
  let len = IO.read_ui16 ic in
  let dat = IO.read_string ic in
  assert (len = String.length dat + 1);
  dat

let read_bytes ic n = "#" ^
  ExtString.String.replace_chars
    (fun c -> sprintf "%02x" (int_of_char c)) 
    (IO.really_nread ic n)

let contains_sjs =
  (* a naive test *)
  ExtString.String.fold_left 
    (fun maybe -> function '\x81'..'\x9f' | '\xe0'..'\xef' | '\xf0'..'\xfc' -> true | _ -> maybe)
    false

(* return [ `Nothing | `Text of string | `Stack of string * int * bool | `Cmd of string * int * int32 list ] *)
let read_cmd ic pos startpos =
  let cmd s = `Cmd (s, 0, [])
  and cmdi i s = `Cmd (s, i, [])
  and stack s = `Stack (s, 0, false)
  and stacki i s = `Stack (s, i, false)
  and read_ptr () = let p = IO.read_real_i32 ic in Int32.add (Int32.of_int (pos ())) p
  in
  match IO.read_ui16 ic with
    | 0x100 | 0x101 | 0x108 | 0x109 | 0x110 | 0x118 | 0x119 | 0x11a | 0x120 | 0x121 | 0x130 | 0x138 | 0x140 
    | 0x142 | 0x148 | 0x150 | 0x158 | 0x15a | 0x160 | 0x162 | 0x170 | 0x178 | 0x180 | 0x188 | 0x190 | 0x1a0 
    | 0x1a1
      as op -> kprintf cmd "op%3x" op (* are these arithmetic of some sort? *)
    | 0x1b0 | 0x1b1 | 0x1b2 | 0x1b3 | 0x1b5 | 0x1b8 | 0x1c0 | 0x1d0 | 0x1d2 | 0x1d8 | 0x1d9 | 0x270 | 0x272 
    | 0x290 | 0x2c0
      as op -> kprintf cmd "op%3x[%s]" op (read_bytes ic 8)
    | 0x800 -> kprintf stack "%ld" (IO.read_real_i32 ic)
    | 0x801 -> let s = read_mjstring ic in
               if contains_sjs s
               then `Stack (s, 0, true)
               else kprintf stack "'%s'" (Util.escape s)
    | 0x802 -> kprintf stack "op802[%s]" (read_bytes ic 8)
    | 0x803 -> kprintf cmd "op803[%s]" (read_bytes ic 4)
    | 0x80f -> kprintf cmd "op80f[%s]" (read_bytes ic 10)
    | 0x810 -> let a = IO.read_real_i32 ic in let b = IO.read_real_i32 ic in
               let argc = IO.read_i16 ic in
               kprintf (cmdi argc) "call<$%08lx, %ld>" a b
    | 0x829 -> let cnt = IO.read_i16 ic in kprintf cmd "op829[%s]" (if cnt == 0 then "" else read_bytes ic cnt)
    | 0x82b -> cmdi 1 "exit"
    | 0x82c -> `Cmd ("goto", 0, [read_ptr ()])
    | 0x82d -> `Cmd ("jmp82d", 0, [read_ptr ()])
    | 0x82e -> `Cmd ("jmp82e", 0, [read_ptr ()])
    | 0x82f -> cmd "op82f"
    | 0x830 -> kprintf cmd "op830[%s]" (read_bytes ic 4)
    | 0x831 -> `Cmd ("jne", 1, [read_ptr ()])
    | 0x832 -> `Cmd ("jmp832", 1, [read_ptr ()])
    | 0x834 -> let a = IO.read_real_i32 ic in
               let argc = IO.read_i16 ic in
               kprintf (stacki argc) "syscall<$%08lx>" a
    | 0x835 -> let a = IO.read_real_i32 ic in
               let argc = IO.read_i16 ic in
               kprintf (cmdi argc) "syscall<$%08lx>" a
    | 0x836 -> let cnt = IO.read_i16 ic in kprintf cmd "op836[%s]" (if cnt == 0 then "" else read_bytes ic cnt)
    | 0x837 -> kprintf cmd "op837[%s]" (read_bytes ic 8)
    | 0x838 -> `Cmd ("jmp838", 1, [read_ptr ()])
    | 0x839 -> `Cmd ("jmp839", 1, [read_ptr ()])
    | 0x83a -> let n = IO.read_i16 ic in if !include_markers then kprintf cmd "{-line %d-}" n else `Nothing
    | 0x83b -> `Cmd ("jmp83b", 0, [read_ptr ()])
    | 0x83d -> `Cmd ("jmp83d", 0, [read_ptr ()])
    | 0x83e -> cmd "op83e"
    | 0x83f -> cmd "op83f"
    | 0x840 -> `Text (read_mjstring ic)
    | 0x841 -> if !include_markers then cmd "op841" else `Nothing
    | 0x842 
       -> let sc = IO.read_i16 ic in
          if sc <> 2 then kprintf failwith "unknown subcode %d to op842 at 0x%06x" sc startpos;
          begin match IO.read_i16 ic with
            | 0x66 -> cmdi 5 "text_control_66"
            | 0x6e -> if !separate_strings <> `No then `Text "\\n" else cmd "br"
            | 0x6f -> cmd "text_control_6f"
            | 0x70 -> cmd "pause"
            | 0x72 -> cmd "text_control_72"
            | 0x73 -> cmdi 1 "text_control_73"
            | 0x77 -> cmd "cls"
            | op -> kprintf failwith "unknown command text_control_%02x at 0x%06x" op startpos
          end    
    | 0x843 -> `Cmd ("jmp843", 0, [read_ptr ()])
    | 0x844 -> cmd "op844" (* target for jmp83b, jmp83d? *)
    | 0x847 -> kprintf cmd "op837[%s]" (read_bytes ic 4)
    | 0x850 -> let c = IO.read_i16 ic in `Cmd ("switch", 0, List.init c (fun _ -> read_ptr ()))
    | op -> kprintf failwith "unknown command op%03x at 0x%06x" op startpos

let process fname =
  let ic = open_in_bin fname in
  let encrypted, _, offset = Mjo.header ic in
  
  (* Get entrypoint details *)
  let entrypoints =
    seek_in ic 0x10;
    let primary = Util.read_int ic in      
    seek_in ic 0x18;
    let record_count = Util.read_int ic 
    and q = Hashtbl.create 0 in
    for i = 0 to record_count - 1 do
      let code = Util.read_int32 ic in
      let offs = Util.read_int ic in
      Hashtbl.add q offs (code, offs == primary)
    done;
    q
  in
  (* Get generic channel for bytecode *)
  let bc, pos =
    IO.pos_in
     (if encrypted then
        IO.input_string (Mjo.crypt ic)
      else
       (seek_in ic offset;
        IO.input_channel ic))
  in
  (* Read commands *)
  let cmds = Queue.create () 
  and targets = ref ISet.empty
  in
  try while true do
    let cpos = pos () in
    let cmd = read_cmd bc pos (cpos + offset) in
    Queue.add (cpos, cmd) cmds;
    match cmd with
      | `Cmd (_, _, l) -> List.iter (fun i -> targets := ISet.add i !targets) l
      | _ -> ()
  done with IO.No_more_input | IO.Overflow _ ->
    IO.close_in bc;
    
    (* Order labels *)
    let _, labels = ISet.fold (fun pos (acc, map) -> acc + 1, IMap.add pos acc map) !targets (1, IMap.empty) in
 
    (* Open output channels *)
    let oc, res =
      let basename = Filename.basename fname in
      let noext = try Filename.chop_extension basename with _ -> basename in
      open_out (Filename.concat !output_dir (noext ^ ".mjs")), resstrs (Filename.concat !output_dir (noext ^ ".sjs"))
    in    
    (* Output code *)
    let do_lbl pos =
      let p32 = Int32.of_int pos in
      if IMap.mem p32 labels then (
        fprintf oc "\n  @%d\n" (IMap.find p32 labels);
        res#finalise ();
        targets := ISet.remove p32 !targets;
      )
    and do_pos pos = 
      match !include_offsets with
        | `None -> ()
        | `Bytecode -> fprintf oc "{-%04x-} " pos
        | `File -> fprintf oc "{-%04x-} " (pos + offset)
    in
    let stack = Stack.create () in
    let pop_all () =
      if not (Stack.is_empty stack) then (
        res#finalise ();
        Stack.iter
          (fun (pos, elt) ->
            do_lbl pos;
            do_pos pos;
            fprintf oc "push %s\n" elt)
          stack;
        Stack.clear stack
      )
    in
    let pop_n ?(clear = true) n =
      let skip = ref None in
      let rv =
        String.concat ", "
          (List.init n
            (fun _ -> 
              if !skip <> None || Stack.is_empty stack then "pop" else
                let pos, elt = Stack.pop stack in
                if !skip = None && IMap.mem (Int32.of_int pos) labels then skip := Some pos;
                elt))
      in
      if clear || !skip <> None then (
        pop_all ();
        Option.may (fun p -> do_lbl p) !skip;
      );
      rv
    in
    let process (pos, cmd) =
      if Hashtbl.mem entrypoints pos then (
        let code, is_entry = Hashtbl.find entrypoints pos in
        Hashtbl.remove entrypoints pos;
        fprintf oc "\n#%s $%08lx\n" (if is_entry then "entrypoint" else "function") code;
        res#finalise ()
      );
      match cmd with
        | `Nothing -> pop_all (); do_lbl pos
        | `Stack (s, n, b)
           -> res#finalise ();
              let s = 
                if n != 0 then 
                  sprintf "%s(%s)" s (pop_n n ~clear:false) 
                else if b then 
                  if res#enabled
                  then res#add_whole s
                  else sprintf "'%s'" (Util.escape s) 
                else s
              in
              Stack.push (pos, s) stack
        | `Text s
           -> pop_all (); do_lbl pos; do_pos pos;
              if res#enabled then
                let r = res#add_part s in
                if r <> "" then fprintf oc "%s\n" r else ()
              else
                fprintf oc "'%s'\n" (Util.escape s)
        | `Cmd (s, n, l) 
           -> let args = if n != 0 then sprintf " (%s)" (pop_n n) else (pop_all (); "") in
              do_lbl pos; do_pos pos;
              res#finalise ();
              output_string oc s;
              output_string oc args;
              match l with
                | [] -> output_char oc '\n'
                | [lb] -> fprintf oc " @%d\n" (IMap.find lb labels)
                | lbls -> fprintf oc " { %s }\n" 
                            (String.concat ", " (List.map (fun i -> sprintf "@%d" (IMap.find i labels)) lbls))
    in
    Queue.iter process cmds;
    pop_all ();
    
    (* Clean up *)
    if not (ISet.is_empty !targets) then 
      eprintf "Warning: labels not generated: %s\n" 
        (String.concat ", " (List.map (fun p -> sprintf "@%d" (IMap.find p labels)) (ISet.elements !targets)));
    if Hashtbl.length entrypoints != 0 then 
      eprintf "Warning: entrypoints not generated: %s\n" 
        (String.concat ", " (Hashtbl.fold (fun _ (v, _) acc -> acc @ [sprintf "$%08lx" v]) entrypoints []));
    close_out oc;
    res#close ()

let opts =
  function
    | "" 
       -> invalid_arg "-"
    | s when String.length s > 2 && String.sub s 0 2 = "d="
       -> output_dir := String.sub s 2 (String.length s - 2)
    | s
       -> String.iter
            (function
              | '-' -> ()
              | 'h' | '?' -> failwith "usage"
              | 'l' -> include_offsets := `Bytecode
              | 'L' -> include_offsets := `File
              | 'm' -> include_markers := true
              | 's' -> separate_strings := `No
              | 'c' -> separate_strings := `Suppress
              | c -> kprintf invalid_arg "-%c" c)
            s

let main =
  try match Util.getopts ["-d"] with
    | args, [file]
       -> List.iter opts args;
          process file
    | _ -> failwith "usage"
  with 
    Failure "usage" ->
      print_endline "Usage: mjdisasm [-hlLms] [-d OUTDIR] FILE";
      exit 1
