lib.name = fluidsynth~

class.sources = fluidsynth~.c

ldlibs = -lfluidsynth

define forWindows

	datafiles += scripts/localdeps.win.sh scripts/windep.sh

endef

define forLinux

	datafiles += scripts/localdeps.linux.sh

endef

define forDarwin

	datafiles += scripts/localdeps.macos.sh

endef

datafiles = fluidsynth~-help.pd LICENSE.txt README.md
datadirs = sf2

include pd-lib-builder/Makefile.pdlibbuilder


windep: install
	cd "${installpath}"; ./windep.sh fluidsynth~.dll

lindep: install
	cd "${installpath}"; /bin/sh localdeps.linux.sh fluidsynth~.pd_linux
