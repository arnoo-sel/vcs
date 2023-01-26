# VCS User's Manual

This directory contains the source for the VCS user's manual, in the [dokki](https://github.com/leikareipa/dokki/) format.

A pre-built HTML version of the manual is hosted [here](https://www.tarpeeksihyvaesoft.com/vcs/docs/user-manual/).

**Note!** The dokki format is work in progress, so the processes described here for dealing with it are preliminary.

## Building the manual

As noted above, the VCS user's manual has been made using dokki, a framework for creating HTML documentation with Markdown. The manual needs to be built before it can be viewed properly.

To build the manual:

1. Clone the [dokki repository](https://github.com/leikareipa/dokki/) and locate the **build-final.js** file in it.
2. Execute `$ node build-final.js index.intermediate.html ../index.html` from the [src/](./src/) directory.

The above steps should produce an **index.html** file, which along with the [img/](./img/) and [js/](./js/) directories constitutes the built manual.

3. As a temporary finalizing step in the build process, search and replace instances of `src="../` with `src="./` in the generated **index.html** file. This step will be obsoleted as the build tools mature.

### Hosting the dependencies

By default, the manual fetches certain dependencies from Tarpeeksi Hyvae Soft's server. The required dependencies are the dokki distributable and [Font Awesome](https://fontawesome.com/) 5. Optional dependencies include Tarpeeksi Hyvae Soft's feedback system.

To host the required dependencies yourself:

1. Obtain the dokki distributable from the [dokki repository](https://github.com/leikareipa/dokki/).
2. Obtain a distributable (or CDN link) for Font Awesome 5 from [their website](https://fontawesome.com/).
3. Modify [index.intermediate.html](./index.intermediate.html) to point to where you plan on hosting the dependencies.
4. Build the manual by following the instructions in [Building the manual](#building-the-manual).
