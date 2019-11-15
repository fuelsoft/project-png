# Project PNG
Load a file into a PNG

## What is this?
Basically the goal is to be able to load a file of any size into a PNG file, and then be able to recover it again later.
## Requirements:
This requires the Clang frontend. You may be able to substitute GCC in for Clang in the makefile, but it's untested.
* Linux is the environment this is developed in and will almost certainly work as expected.
* MacOS is untested but shouldn't require much (if any) additional work. 
* Windows may or may not work with a significant amount of effort, but I make no effort to support it. 

Beyond that, there are no requirements.

## How do I use it?
### Usage:
There are 3 key modes to the program:
* *Analysis mode* runs the program in a non-destructive way - it doesn't modify anything. Use this to test if a PNG has a file packed within itself already.
* *Insertion mode* will take a provided file and pack it into a provided PNG file.
* *Extraction mode* will (if possible) restore a copy of the inserted file.

You can build it by running `make`, then run it with `./png [flags] <input> [<target> <output>]`
* `flags` are:
	* `-h`: Display Help
	* `-d`: Print Details/Run Verbose
	* `-a`: Analysis Mode [default]
	* `-i`: Insertion Mode
	* `-e`: Extraction Mode

All operations require a base PNG to work with:
* `input` is the PNG file you wish to work with.

Insertion also requires two additional files:
* `target` is another file (it could also be a PNG, it doesn't matter) that you wish to work with.
* `output` is the name of a new PNG file to write the result to.

To make it clear, `target` will be inserted into `input` and outputted as `output`.

### Results:
The following are possible outcomes for analysis mode:
* Non-PNGs will result in an error and program termination (not a crash - expected).
* Valid PNGs will result in details of various image statistics being printed out.
* Invalid PNGs will most likely be caught and the error reported, but I make no promises. There are a *lot* of edge cases.

The following are possible outcomes for insertion mode:
* Non-PNGs will result in an error and program termination.
* Valid PNGs will result in the target file being inserted and written to disk as the filename provided.
* Invalid PNGs will most likely be caught and the error reported. Again, I make no promises.

The following are possible outcomes for extraction mode:
* Non-PNGs will result in an error and program termination.
* Valid PNGs will result in the inserted image being written out to disk **under its original filename**.
* Invalid PNGs may be caught and the error reported, but this mode only does basic error checking.

*Here's a fun activity: `./png -i <your png> png.cpp test.png`, then look at a hexdump of `test.png`.*
*Alternatively, you can also try `./png -i <your png> <your png> test.png` to place the PNG within itself*

#### TODO:
* Data extraction
* Re-write problem/weak code
