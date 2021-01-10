lib.name = fluidsynth~

class.sources = fluidsynth~.c

ldlibs = -lfluidsynth

define forWindows

	datafiles += scripts/localdeps.win.sh scripts/windep.sh

endef

define forLinux

ifeq ($(firstword $(subst -, ,$(shell $(CC) -dumpmachine))), i686)
	datafiles += scripts/linuxdep32.sh
	else
	datafiles += scripts/linuxdep64.sh
endif

endef

define forDarwin

	datafiles += scripts/localdeps.macos.sh

endef

datafiles = fluidsynth~-help.pd LICENSE.txt README.md 
datadirs = sf2

include pd-lib-builder/Makefile.pdlibbuilder


windep: install
	cd "${installpath}"; ./windep.sh fluidsynth~.dll 