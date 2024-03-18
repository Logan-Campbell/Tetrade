# Tetrade
A homebrwew Tetris clone for the original Playstation. 

### Modes
Includes two modes:

**Marathon Mode:** Complete lines, and score points. Game gets faster as more lines are cleared. Up to two players can play a game simultaneously.

**Versus Mode:** Play head-to-head. Clearing lines adds lines to the opponents matrix.

### Random System
The game offers two ways that the next game piece or tetrimino is selected (selectable in the options menu).

**Random bag:** A unique set of each of the 7 tetriminoes are shuffled together, making up the next tetriminoes given to the player.

**Pure Random:** Each tetrimino is randomly selected.


## Controls

### Game:

**D-Pad Left:** Move left \
**D-Pad Right:** Move Right \
**D-Pad Up:** Hard drop \
**D-Pad down:** Soft drop

**Cross:** Rotate Counterclockwise \
**Circle:** Rotate Clockwise

**R1,L1,Square:** Hold

### Menus:

**Cross:**  Confirm \
**Circle:** Cancel/Back \
**D-Pad Left:** Decrease \
**D-Pad Right:** Increase

## To Build:

[Install PSn00bSDK](
https://github.com/Lameguy64/PSn00bSDK)

[Install guide](https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/installation.md)

Run both commands in the root of the project folder:

```cmake --preset default .```\
```cmake --build ./build```

build directory will contain .CUE .BIN .EXE files.


## Credits:

Thanks to Lameguy64, spicyjpeg for Psn00SDK and examples.

#### Music by:
Sara Garrard sonatina.itch.io

#### Sound Effcts by:
Kenney Vleugels (www.kenney.nl) \
FilmCow