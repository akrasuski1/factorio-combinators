# factorio-combinators
Python script reading blueprints and simulating circuits.

![Blueprint image](https://github.com/akrasuski1/factorio-combinators/blob/master/factorio-blueprint.png)

Benchmark output for reasonable sized blueprint shown above (it's an 8-value bidirectional shift register, a.k.a. stack):
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
