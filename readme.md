# Project PNG
Load a file into a PNG

## So what is this?
Basically the goal is to be able to load a file of any size into a PNG file, and then be able to recover it again later.
## Requirements:
This requires the Clang frontend. You may be able to substitute GCC in for Clang in the makefile, but it's untested.
* Linux is the environment this is developed in and will almost certainly work as expected.
* MacOS is untested but shouldn't require much (if any) additional work. 
* Windows may or may not work with a significant amount of effort, but I make no effort to support it. 

Beyond that, there are no requirements.

## How do I use it?
### So far:
You can build it by running `make`, then run it with `./png <input>` where the input is a file.
The following are possible outcomes:
* Non-PNGs will result in an error and program termination (not a crash - expected)
* Valid PNGs will result in details of the leading IHDR chunk being printed out
* Invalid PNGs will most likely be caught and the error reported, but I make no promises.

Once the image is loaded, the program will check the signature at the start of the file. Once this is confirmed, it continues to load all the image chunks into memory, performing checks and logging progress as it goes. Once the image is loaded, the program returns to the leading IHDR block and extracts image data related to dimensions, bit depth and more, which it prints out.

#### TODO:
* Checksum validation - this is important to image integrity checking.
* Storage format design.
* Custom chunk design.
