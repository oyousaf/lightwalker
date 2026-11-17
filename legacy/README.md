# Legacy: the 2014 Flash games

The original games behind the Unreal Engine 5 remake, written in ActionScript 2 with Flash CS6 in 2014. They're kept as they were, apart from the extracted script.

| Folder | Game | Becomes |
|---|---|---|
| [lightwalker/](lightwalker/) | **Lightwalker**, a single-screen arena shooter inspired by Robotron. Arrow keys move and WASD fires lasers in eight directions. Wave 1 has 15 shaytaan chasers; wave 2 has 30 chasers and 5 dajjal wanderers. Three lives, 100 points a kill. | Arena mode |
| [green-ball/](green-ball/) | **Green Ball**, a side-scrolling jumper and Lightwalker's first concept (see `lightwalker/initialplans.jpg`). | The Path mode |
| [block-breaker/](block-breaker/) | **Block-breaker**, a brick breaker built from a tutorial. | The Shield mode |

- `lightwalker/LightWalker.as` is the game's ActionScript, extracted from the `.fla` so it can be read without Flash.
- `lightwalker/art/` holds the original sprite PSDs for Lightwalker, the shaytaan and the dajjal.
- The `.swf` files play in a browser through [Ruffle](https://ruffle.rs).
- The loose sound files from 2014 aren't included, because their licences are unclear. The remake uses new sounds.

## Known bugs, fixed in the remake

- The left-firing laser checks the vertical position against the left edge, so it flies off screen.
- The dajjal's random direction always resolves to "down".
- Enemy spawning only accepts points outside the safe zone on both axes, so enemies start in the corners, or at the top-left if no spot is found.
- Respawning keeps the last random spot even when every attempt was too close to an enemy.
- The dajjal timer handle overwrites the chaser timer handle.
