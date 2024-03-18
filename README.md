# Tetrade
A homebrew Tetris clone for the original Playstation. 

### Modes
Includes two modes:

**Marathon Mode:** Complete lines, and score points. Game gets faster as more lines are cleared. Up to two players can play a game simultaneously.

**Versus Mode:** Play head-to-head. Clearing lines adds lines to the opponents matrix.

### Random System
The game offers two ways that the next game piece or tetrimino is selected (selectable in the options menu).

**Random bag:** A unique set of each of the 7 tetriminoes are shuffled together, making up the next tetriminoes given to the player.

**Pure Random:** Each tetrimino is randomly selected.

## Screenshots

### Title Screen
![Title](/screenshots/scr_0.png)
### Marathon Mode
![Marathon Mode](/screenshots/scr_1.png)
### Versus Mode
![Two Player](/screenshots/scr_2.png)
![Two Player Winner](/screenshots/scr_3.png)


## Controls

### Game:

**D-Pad Left:** Move left \
**D-Pad Right:** Move Right \
**D-Pad Up:** Hard drop \
**D-Pad down:** Soft drop

**Cross:** Rotate Counterclockwise \
**Circle:** Rotate Clockwise

**R1, L1, Square:** Hold

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

Build directory will contain the .CUE, .BIN, and .EXE files. I have tested this game on real hardware (SCPH-7501) and should fully work as it does in emulators.


## Credits:

Thanks to Lameguy64, spicyjpeg for Psn00SbDK and all the examples.

#### Music by:
Sara Garrard (sonatina.itch.io)

#### Sound Effcts by:
Kenney Vleugels (www.kenney.nl) \
FilmCow
