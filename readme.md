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
You can build it by running `make`
Then, run it with `./png <input>` where the input is a file.
* Non-PNGs will result in an error and program termination (not a crash - expected)
* Valid PNGs will result in details of the leading IHDR chunk being printed out
* Invalid PNGs may or may not be caught, this is todo

### *More to come.*
