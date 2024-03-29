lib.name = sfont~

class.sources = sfont~.c

ldlibs = -lfluidsynth

datafiles = sfont~-help.pd sfont~-meta.pd LICENSE.txt README.md
datadirs = sf

# Makefile based on pd-lib-builder by Katja Vetter, see: https://github.com/pure-data/pd-lib-builder

PDLIBBUILDER_DIR=pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))

localdep_linux: install
	scripts/localdeps.linux.sh "${installpath}/sfont~.${extension}"

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/sfont~.${extension}"

localdep_macos: install
	scripts/localdeps.macos.sh "${installpath}/sfont~.${extension}"
