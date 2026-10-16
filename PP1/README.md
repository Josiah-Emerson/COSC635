CURRENT: Left some notes in the receive file func, but need to figure out the best way for the ingress of 
data to flow. See all the validation that needs to happen, as well as the tracking of stats. NOTE: have not 
added much for the END portion


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
