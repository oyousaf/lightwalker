# Menu theme: FL Studio session sheet

An original tune in the style of early-90s 16-bit platformers (Mega Drive FM, Amiga): bouncy FM slap bass, off-beat stabs, a 16th-note arpeggio, a hooky lead and punchy drums. It's a stand-in until a more modern theme replaces it.

- **Tempo:** 132 BPM, 4/4
- **Key:** A minor, lifting to C major
- **Length:** 28 bars, about 51 s
  - Bars 1–4 are the intro (plays once).
  - Bars 5–28 are the loop (A, A, B). The MIDI file has "Loop start" and "Loop end" markers.
- **Rough preview:** `MenuTheme_Preview.wav` uses crude built-in synths. It's only there to judge the tune.

The MIDI comes from `Tools/music/compose_menu_theme.py`. If you change the notes there and re-run it, the files update.

## 1. Set up

1. Start a new FL Studio project and set the tempo to **132**.
2. Go to **File → Import → MIDI file** and choose `MenuTheme_AllParts.mid`.
   - Tick "Start new project" (or not, if you've already set the tempo) and keep "Separate tracks".
   - FL creates one channel and pattern per part.
3. Replace each channel's default instrument with the one in the table below.
   - Right-click the channel, choose **Replace**, then pick the plugin.
   - Browse that plugin's presets and choose by ear. The table describes the sound to aim for.

The parts are also exported separately (`1_Lead.mid` … `5_Drums.mid`), so you can drag a single part onto a channel.

## 2. Instruments

| Part | Plugin | Sound to aim for |
|---|---|---|
| **Lead** | Sytrus (FM leads) or Wasp XT | A bright, slightly nasal FM or square lead, like a Mega Drive melody. Give it a little vibrato. |
| **Bass** | Sytrus (FM bass), or Transistor Bass | A punchy FM "slap" bass with a bright attack that dulls quickly. Keep it mono. |
| **Stabs** | 3x Osc or Sytrus (brass or organ stab) | Short and snappy, on the off-beats. The intro bars hold long chords, so a pad suits them (Harmless or Harmor). |
| **Arp** | Sytrus bell or pluck, or Plucked! | Glassy and quick, sitting behind the lead. Pan it slightly left and right. |
| **Drums** | FPC with a punchy kit from the **Drums** pack | Tight 90s drum machine. The notes follow the General MIDI layout, which FPC's default pads match: 36 kick, 38 snare, 39 clap, 42 closed hat, 46 open hat, 49 crash, 45/47/50 toms. |

## 3. Effects and mix

- **Lead:**
  - Fruity Delay 3, dotted 1/8, about 20% wet.
  - Fruity Reeverb 2, small room, about 15%.
- **Arp:** the same delay, with a little more reverb. Cut everything below about 300 Hz.
- **Bass:** no reverb. Light compression.
- **Retro grit (optional):** a touch of Fruity Squeezes or a gentle low-pass on the master gives it the early-90s console feel.
- **Master:**
  - Fruity Limiter, with the ceiling at **−1 dB**.
  - Aim for a mix that sounds full at about 70% of your listening volume. The game's volume sliders handle the rest.

## 4. Export (what the game needs)

Export two files from the Playlist in Song mode as WAV, **48 kHz, 24-bit**. Save them to `SourceArt/Music/MenuTheme/Export/`.

1. **`MenuTheme_Intro.wav`:** bars 1–4.
2. **`MenuTheme_Loop.wav`:** bars 5 to the end of bar 28, cut exactly on the bar lines.
   - In the export window, set **Tail** to **"Wrap remainder"**. Reverb and delay tails then wrap to the start, so the loop is seamless.

For each export:
- Select the bar range on the Playlist timeline before exporting.
- Choose **Selected** as the export range.

Save the FL project as `SourceArt/Music/MenuTheme/MenuTheme.flp` so it can be changed later.

Then tell Claude. The game plays the intro once, then repeats the loop, through the Music volume slider.
