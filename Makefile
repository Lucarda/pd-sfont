lib.name = sfont~

class.sources = sfont~.c

ldlibs = -lfluidsynth

datafiles = sfont~-help.pd sfont~-meta.pd LICENSE.txt README.md
datadirs = sf

# This Makefile is based on the Makefile from pd-lib-builder written by
# Katja Vetter. You can get it from:
# https://github.com/pure-data/pd-lib-builder

PDLIBBUILDER_DIR=pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))

localdep_linux: install
	scripts/localdeps.linux.sh "${installpath}/fluidsynth~.${extension}"

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/fluidsynth~.dll"

localdep_macos: install
	scripts/localdeps.macos.sh "${installpath}/fluidsynth~.${extension}"
