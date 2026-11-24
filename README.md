# LightWalker

A neon arena shooter in Unreal Engine 5, remaking a Flash game I made in 2014. Lightwalker holds the centre of a dark arena while waves of shaytaan close in and dajjal drift around the edges, and fights back with bolts of light.

The originals are in [legacy/](legacy/).

## Controls

| | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | WASD or arrow keys | Left stick or d-pad |
| Fire where you face | Space or Left Ctrl | RT, RB or A |
| Aim and fire | Hold left mouse button (fires at the cursor) | Right stick |
| Pause | Esc or P | Start |
| Play again | Enter or Space | A or Start |

## Status

Finished: the arena (15 waves, power-ups, two animated heroes, music and menus), The Path (a run-and-jump stage after wave 5), The Shield (a beam-deflecting wall breaker after wave 10), the boss, and an endless arena with a leaderboard kept on your computer. Next up is play-testing the packaged Windows build.

## Building

Requires Unreal Engine 5.8 and Visual Studio with the "Game development with C++" workload. Right-click `LightWalker.uproject` → Generate Visual Studio project files, then open the project; the editor compiles the C++ module on first launch.
