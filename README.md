* * *

**[fluid~] - soundfont player for Pure Data that uses fluidsynth.**

* * *

### About:

[fluid~] is an external for Pure Data that loads fluidsynth for a fullblown orchestra.

**FluidSynth** is a Real-Time SoundFont Software Synthesizer (see https://www.fluidsynth.org and https://github.com/FluidSynth/fluidsynth).

This repository and distribution carries the version 2.0 and onwards. Current release is version 2.0, which is using fluidsynth 2.1.6 (the latest at the time of the release). Original version (1.0) was released by Frank Barknecht in 04/04/2003.



--------------------------------------------------------------------------

### Licence:

Distributed under the GPLv2+, please check the LICENSE file for details.



--------------------------------------------------------------------------

#### Building [fluid~] for Pd Vanilla:

First you need to install  **FluidSynth** (https://www.fluidsynth.org/) in your system. You can run the makefile after that.

This project relies on the build system called "pd-lib-builder" by Katja Vetter (see: <https://github.com/pure-data/pd-lib-builder>). PdLibBuilder tries to find the Pd source directory at several common locations, but when this fails, you have to specify the path yourself using the pdincludepath variable. Example:

<pre>make pdincludepath=~/pd-0.51-4/src/  (for Windows/MinGW add 'pdbinpath=~/pd-0.51-4/bin/)</pre>

* Installing with pdlibbuilder

Go to the pd-else folder and use "objectsdir" to set a relative path for your build, something like:

<pre>make install objectsdir=../fluid-build</pre>

Then move it to your preferred install folder for Pd and add it to the path.

Cross compiling is also possible with something like this

<pre>make CC=arm-linux-gnueabihf-gcc target.arch=arm7l install objectsdir=../</pre>



#### Further instructions:

#### - macOS

Download and install **FluidSynth** via homebrew (https://brew.sh/).

- "brew install fluidsynth"

After running the makefile, run the "localdeps.macos.sh" script with "fluid~.extension" as the argument. This magical script that copies the dynamic libraries into the external folder and links them correctly.



#### - Windows

Download the 32bit and 64bit **msys2** packages of **FluidSynth**:

`pacman -S mingw32/mingw-w64-i686-fluidsynth`

`pacman -S mingw64/mingw-w64-x86_64-fluidsynth`

and install the **ntldd** package:

`pacman -S mingw32/mingw-w64-i686-ntldd-git`

`pacman -S mingw64/mingw-w64-x86_64-ntldd-git` 

Then you can do with MinGW: 

`make install`

or you can also specify more options with:

`make PDDIR=<path/to/pd directory> PDLIBDIR=<path/to/output> install`

then on the MinGW shell `cd` to your lib installation dir and run the dependencies script:

for 32bit

`````
cd <to/your/lib/installation/dir>
./windep32.sh

`````

for 64bit

`````
cd <to/your/lib/installation/dir>
./windep64.sh

`````



#### - Linux

first build the latest **fluidsynth** (at the time of writing is v 2.1.6)

Download sources from https://github.com/FluidSynth/fluidsynth/releases 

and get the following dependencies (shown for Debian)

`sudo apt install cmake libglib2.0-dev pkgconf`

then `cd` to your fluidsynth sources dir an do:

`````
mkdir build
cd build
cmake ..
sudo make install
`````

then `cd` to the sources of this repo and do

`make install`

after that run the linuxdepXX.sh script that matches the architecture of your build. This script copies fluidsynth dependencies to your fluid~ external folder. Example for 64bit:

`````
cd <to/your/fluid~/installation/dir>
chmod 775 linuxdep64.sh
./linuxdep64.sh
`````

Now the fluidsynth dependencies are copied to your fluid~ external folder.




--------------------------------------------------------------------------

### History and repositories:

The original code and version (1.0) of fluid~ was release by Frank Barknecht in 04/04/2003. This code is available at: <https://sourceforge.net/p/pure-data/svn/HEAD/tree/trunk/externals/footils/fluid/>.

The code was ported to Pd's API in 2016 by Jonathan Wilkes for distribution Purr Data and later modified to include "SMMF" by Albert Gräf in 2020. This is at: https://git.purrdata.net/jwilkes/purr-data/-/tree/master/externals/fluid~

This repository is a fork of Purr Data's code, released in 2021 by Porres as version 2.0, with minor modifications and a port do distribute it for Pd Vanilla. This project's code resides at: https://github.com/porres/pd-fluid



--------------------------------------------------------------------------


### Credits :

Thanks to the authors of fluidsynth (https://github.com/FluidSynth/fluidsynth/blob/master/AUTHORS) and to Larry Troxler, author of the older [iiwu~] external in which fluid~ was based on. thanks to IOhannes Zmölnig for the magical script that copies the dynamic libraries into the external folder and links them correctly. Thanks to Lucas Cordiviola for helping on how to build for windows/Linux and others from the pd-list that also helped.

- *Version:  1.0* (original) by Frank Barknecht [04/04/2003],  

- Ported from Flext/C++ to plain C/pdlibbuilder by Jonathan Wilkes for Purr Data [2016]

- Added SMMF mode and various other little improvements by Albert Gräf for Purr Data [2020]

- Revised by Alexandre Torres Porres and ported to distribute for Pd Vanilla as *Version:  2.0* [01/2021] 

  

--------------------------------------------------------------------------




