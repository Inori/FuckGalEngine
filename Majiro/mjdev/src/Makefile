# Makefile for MjDev
# Copyright (c) 2005 Haeleth
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA  02111-1307, USA.

OCAMLC = ocamlopt
#OCAMLC = ocamlc -g

EXTLIB = $(shell which ocamlfind >/dev/null 2>&1 && ocamlfind query -i-format extlib) extlib.cmxa

TARGETS = mjarc mjcrypt mjdisasm mjasm

.PHONY: all $(TARGETS)
all: $(TARGETS)

../mjasm.exe: util.ml key.ml mjsAst.ml mjsParse.mli mjsParse.ml mjsLex.ml asm.ml
	$(OCAMLC) $(EXTLIB) $^ -o $@

../mjdisasm.exe: util.ml key.ml mjo.ml disasm.ml
	$(OCAMLC) $(EXTLIB) $^ -o $@

../mjarc.exe: util.ml arc.ml
	$(OCAMLC) $(EXTLIB) $^ -o $@

../mjcrypt.exe: util.ml key.ml mjo.ml crypt.ml
	$(OCAMLC) $(EXTLIB) $^ -o $@

%.ml %.mli: %.mly
	ocamlyacc $<

%.ml: %.mll
	ocamllex $<

GENERATED = mjsParse.mli mjsParse.ml mjsLex.ml

.PHONY: clean
clean:
	$(RM) *.cmx *.cmi *.cmo *.o $(GENERATED)

define win32target
.PHONY: $(1)
$(1): $$(addprefix ../,$$(addsuffix .exe,$(1)))
endef
$(foreach t,$(TARGETS),$(eval $(call win32target,$t)))