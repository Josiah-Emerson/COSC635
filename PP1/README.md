CURRENT: MAIN IMPLEMENTATION IS DONE RAHHH
TODO: Test between laptop and PC
TODO: Make sure we are outputting the correct stats for both send/recv
TODO: Make output prettier
TODO: Take command line arguments
TODO: Test with a big file over network


TODO: Possible implementation could depend on versions. One version TransferID is just an ID and in another 
one it is a hash of the file and thus more reliable
TODO: Test/sim for out of order packets (i.e. send a packet after END)
TODO: I know there is a TODO in the pp1 sender class about the const value for max payload. Update that to be the val in the defs.h

To compile:
```
cmake -B build
cmake --build build
```

executables will be placed in the build/bin directory
