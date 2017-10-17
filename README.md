# factorio-combinators
Python script reading blueprints and simulating circuits.

![Blueprint image](https://github.com/akrasuski1/factorio-combinators/blob/master/factorio-blueprint.png)

This code was first written in Python, with around 2600 UPS for the blueprint
shown above (it's an 8-value bidirectional shift register, somewhat similar to stack).

After rewriting it to C++ and optimizing various things, we reach about 
160 000 UPS at the same blueprint.

This is the output of benchmarking code for Python (the one for C++ has a bunch
of additional debug output, but otherwise is the same).
```
Tick 0: 	lamps: . . . . . . . .
Tick 1: 	lamps: # # # # # # # #
Tick 2: 	lamps: . . . . . . . .
Tick 125: 	lamps: # . . . . . . .
Tick 165: 	lamps: . # . . . . . .
Tick 205: 	lamps: # . . . . . . .
Tick 245: 	lamps: . # . . . . . .
Tick 285: 	lamps: . . # . . . . .
Tick 325: 	lamps: . # . . . . . .
Tick 365: 	lamps: . . # . . . . .
Tick 405: 	lamps: . . . # . . . .
Tick 445: 	lamps: . . # . . . . .
Tick 485: 	lamps: . . . # . . . .
Tick 525: 	lamps: . . . . # . . .
Tick 565: 	lamps: . . . # . . . .
Tick 605: 	lamps: . . . . # . . .
Tick 645: 	lamps: . . . . . # . .
Tick 685: 	lamps: . . . . # . . .
Tick 725: 	lamps: . . . . . # . .
Tick 765: 	lamps: . . . . . . # .
Tick 805: 	lamps: . . . . . # . .
Tick 845: 	lamps: . . . . . . # .
Tick 885: 	lamps: . . . . . . . #
Tick 925: 	lamps: . . . . . . # .
Tick 965: 	lamps: . . . . . . . #
Tick 1005: 	lamps: . . . . . . . .
Tick 2999: 	lamps: . . . . . . . .
#entities: 50
UPS: 2616.75546256
```
