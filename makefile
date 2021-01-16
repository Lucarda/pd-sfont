lib.name = fluidsynth~

class.sources = fluidsynth~.c

ldlibs = -lfluidsynth

define forWindows

	datafiles += scripts/localdeps.win.sh scripts/windep.sh

endef

define forDarwin

	datafiles += scripts/localdeps.macos.sh

endef

datafiles = fluidsynth~-help.pd LICENSE.txt README.md
datadirs = sf2

include pd-lib-builder/Makefile.pdlibbuilder


localdep_windows: install
	cd "${installpath}"; ./windep.sh fluidsynth~.dll

localdep_linux: install
	scripts/localdeps.linux.sh "${installpath}/fluidsynth~.pd_linux"
