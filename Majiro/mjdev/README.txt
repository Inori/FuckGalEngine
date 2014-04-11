-------------------------------------------------------------------------------

                                     MJDEV

       an unofficial development toolkit for the Majiro virtual machine

-------------------------------------------------------------------------------
                           PRELIMINARY DOCUMENTATION
-------------------------------------------------------------------------------


mjasm [-d OUTDIR] [-o OUTNAME] [-r RESNAME] FILE

  Assmble FILE into an .mjo bytecode object.  The output filename is constructed 
  automatically, but can be overridden with the -o option.
  
  If the -r option is not used to identify a string resource file, mjasm will 
  look for one with the same name as the input file but with a .sjs extension. 
  If none can be found, any #res<> directives in the source code will expand to 
  dummy values instead.
  

mjdisasm [-lLmsc] [-d OUTDIR] FILE

  Disassemble a .mjo bytecode object.  The output filename is constructed
  automatically by replacing the .mjo suffix with .mjs.
  
  By default, human-readable Japanese strings are placed in a separate resource 
  file with the extension .sjs, so they can be edited independently from the 
  source code, and also so that any future changes to the source code's format 
  will not require tedious manual editing to avoid clobbering translated text. 
  Use the -s option to disable this behaviour.

  Note that the technique used to decide which strings to separate is naive; it 
  separates out all display strings, and any other strings containing Japanese 
  characters.  This latter is necessary to include text displayed in selection 
  screens and the like, but it cannot distinguish such cases from other strings,
  such as filenames containing Japanese text, which should NOT be altered by 
  translators.
  
  The following options are recognised:
    -l: print bytecode offsets next to each instruction
    -L: as -l, but print file offsets instead of bytecode offsets
    -m: include marker commands (no-ops and code 100% predictable from
        context)
    -s: don't put strings into a separate file
    -c: don't actually create a separate file for strings, but generate code as
        though it were being done (use this to avoid clobbering existing 
        resources)
    -d OUTDIR: place output in OUTDIR (which must exist) rather than in the
               current directory
  
  
mjarc -l -[rsu] ARCFILE [FILES]
  
  List contents of the .arc archive ARCFILE.  If FILES are specified, only
  those files will be listed.
  
  By default files are sorted in alphabetical order.  If the "-s" flag is
  passed, they are sorted in ascending order of size; if "-u", they are
  printed in the actual order they come in the archive.  The "-r" option
  causes the sort order to be reversed.

mjarc -x [-d OUTDIR] ARCFILE [FILES]
  
  Extract FILES from the .arc archive ARCFILE.  If FILES is not given, all
  files will be extracted.  Filenames must be exact matches; no wildcards are
  currently supported.

mjarc -a ARCFILE FILES

  Add the given FILES to ARCFILE.
  ARCFILE will be created if it does not already exist.  Any existing files in 
  it will be overwritten without warning if they have the same name as a new 
  file.

  This is very slow on large archives, as the algorithms have been kept naive
  for simplicity's sake.  Bear in mind that in many cases Majiro will use a file
  _outside_ an archive in preference to an archived file: for example, when
  testing changes to a .mjo, you can just put the .mjo in the same folder as
  data.arc and your version will be the one that is used.
  
  
mjcrypt INFILE [OUTFILE]
  
  Encrypt/decrypt a .mjo bytecode object.
  
  If only INFILE is given, the file is modified in place.  If OUTFILE is given, 
  the modified version will be written to it instead.

  Note that you do not need to decrypt object files before disassembly, and you 
  MUST ensure that any object files in the interpreter's folder are kept 
  encrypted.  The decryption function is only provided in case you have some
  need to access raw bytecode.


vaconv INFILE

  Assuming INFILE is a .rc8 or .rct file, converts it to a PNG.
  
vaconv INFILE -f rc8 (or rct)

  Assuming INFILE is a PNG, converts it to a .rc8 or .rct.


Source code is provided in the "src" directory (except for vaconv, which is part 
of RLdev: http://dev.haeleth.net/).  Laugh at how trivial Majiro's file formats 
really are, or curse me for not using your favourite programming language.  As 
you wish, really.  Source code is licenced under the GNU GPL; see src/COPYING 
for details.

Have fun.

-- Haeleth
