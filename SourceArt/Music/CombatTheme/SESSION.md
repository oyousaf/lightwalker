# Combat theme (UK drill): FL Studio session sheet

A dark UK drill instrumental for the waves.

- **Tempo:** 142 BPM in half-time. The snare lands on beat 3, so it feels heavy rather than fast.
- **Key:** C# minor, harmonic (the G# chord is major, which gives it that eerie pull).
- **Chords:** C#m | A | F#m | G# in the A sections; A | F#m | C#m | G# in the B sections (the lift).
- **Length:** a 112-bar loop, about 3 min 9 s, in 8-bar sections (the MIDI has a marker at each):

  | Bars | Section | What happens |
  |---|---|---|
  | 1 | A | The hook: a two-bar piano/bell ostinato over long, low strings. |
  | 9 | A2 | The hook with a turnaround. |
  | 17 | B | The lift: a higher, longer melody, string stabs on the 3-3-2 grid, a busier 808 that drops an octave. |
  | 25 | B2 | The lift, fuller, turning back. |
  | 33 | Break | Strings up high, the hook's notes held, 808 and drums thinned out. |
  | 41 | C | A new section: a lower melody over F#m, C#m, A and G#, pulsing strings. |
  | 49 | C2 | Its second half an octave up. |
  | 57 | A3 | The hook, its second half an octave up. |
  | 65 | A4 | The hook with a turnaround. |
  | 73 | B3 | The lift. |
  | 81 | B4 | The lift, turning back. |
  | 89 | Drop | Three bars of near silence, a roll, then everything slams back in. |
  | 97 | C3 | The new section again. |
  | 105 | Turn | The hook thinning out, then the turnaround back to A. |
- **Rough preview:** `CombatTheme_Preview.wav` uses crude built-in synths. It's only there to judge the groove and melody.

The MIDI comes from `Tools/music/compose_combat_theme.py`. If you change the notes there and re-run it, the files update.

## How the game uses it

Each part is a separate stem. All the stems are exactly the same length, so they stay in sync, and the game layers them up as the waves get harder:

| Waves | Stems playing |
|---|---|
| 1–3 | 1 Strings, 2 Melody |
| 4–7 | + 3 808 |
| 8–11 | + 4 Drums |
| 12–15 | + 5 Counter, 6 HatRolls |

Each stem needs to sound right on its own and with the others. Keep the reverb and delay tails inside each stem's own channel.

## 1. Set up

1. Start a new FL Studio project and set the tempo to **142**.
2. Go to **File → Import → MIDI file** and choose `CombatTheme_AllParts.mid`, keeping tracks separate.
   - FL puts **FLEX** on every imported channel. That's expected: a MIDI file only carries notes, not sounds. Either pick a FLEX preset (FLEX has good 808, keys, strings, bell and pad presets) or swap the plugin as below. Drums need a General MIDI drum kit: FLEX's **General Midi Library → Drum Electronic** (the easiest, and the closest to drill) or **FPC**, whose default pads match too. Any other FLEX preset would play them as pitched notes.
   - Drums and HatRolls both use the drum channel. Put them on **two separate channels** with the same kit, because they're separate stems. You can also drag `4_Drums.mid` and `6_HatRolls.mid` in one at a time.
3. Replace each channel's default instrument with the one in the table below, then browse its presets by ear.

## 2. Instruments

| Stem | Plugin | Sound to aim for |
|---|---|---|
| **1 Strings** | Harmless or Sakura (strings or dark pad) | Eerie, sustained and low, with a slow attack. In drill these sit in the background like a horror film score. |
| **2 Melody** | FL Keys (piano), or a Sytrus or Harmor bell | A dark plucked piano or bell. This is the hook, so it should be clear but cold. |
| **3 808** | FLEX (808 presets), or a sampler with an 808 from the **Drums** pack | A long, distorted 808. **Turn on Porta** (the portamento switch in the channel settings) so the overlapping notes slide. Every note meant to glide overlaps the next by half a 16th, so the slides land exactly where they're written: up to the octave, the fifth and back, and a slide down into each new section. Slide time about 60–90 ms. Keep it mono. |
| **4 Drums** | FLEX → General Midi Library → **Drum Electronic**, with its output turned well down (the MIDI hits hard) and a low shelf cut of 4–6 dB around 80 Hz to soften the kick, or FPC | A hard, dry kick (it follows the 808's long notes), a crisp snare layered with a clap on beat 3, the skippy late snare every other bar, and tight hats with **short decay** in the 3-3-2 grouping. The notes follow the General MIDI layout, which Drum Electronic and FPC's default pads both follow: 36 kick, 37 rim, 38 snare, 39 clap, 42 hat, 46 open hat, 49 crash. |
| **5 Counter** | Sytrus or Harmor (glassy bell, or a choir pad) | High bells answering the hook in the A sections, then a held line above the melody in the B sections. A soft choir works well for the B part. |
| **6 HatRolls** | A second channel with the same kit as the Drums stem | Rolls that speed up from triplets to 32nds into beat 3, stutters, and a tom roll into each section. Pan slightly off centre. Pitching the rolls up a few semitones in FPC is very drill. |

## 3. Effects and mix

- **808:**
  - Fruity Blood Overdrive or Fruity Fast Dist for grit.
  - Fruity Limiter on the channel to keep it steady.
  - Cut everything above about 4 kHz if it gets buzzy.
- **Melody:**
  - Fruity Reeverb 2, large and dark, about 25%.
  - Fruity Delay 3, 1/4 note, about 15%.
- **Strings:** heavy reverb and a low-pass around 6 kHz. They should sit behind everything.
- **Drums:** no reverb on the kick or 808. A touch of room on the snare.
- **Sidechain the 808 to the kick** (Fruity Limiter in compressor mode, or Fruity Peak Controller) so they don't fight.
- **Master:**
  - Fruity Limiter, ceiling **−1 dB**.
  - Check each stem solo as well as together.

## 4. Export (what the game needs)

Export as WAV, **48 kHz, 24-bit**, bars 1 to the end of bar 112, cut exactly on the bar lines. Use **Tail: "Wrap remainder"** so each stem loops seamlessly. Save to `SourceArt/Music/CombatTheme/Export/`.

To export every stem at once:
1. Route each instrument to its own mixer track.
2. In the export window, tick **"Split mixer tracks"**.
3. Rename the results:
   - `Combat_1_Strings.wav`
   - `Combat_2_Melody.wav`
   - `Combat_3_808.wav`
   - `Combat_4_Drums.wav`
   - `Combat_5_Counter.wav`
   - `Combat_6_HatRolls.wav`

**Stings** (one-shots, no loop, let the tails ring out):
- `Sting_WaveCleared.mid`: a rising bell arpeggio that resolves on E major. Use the Melody's bell or piano sound. Export it as `Sting_WaveCleared.wav`.
- `Sting_GameOver.mid`: a slow fall onto a low C#, with an 808 drop beneath it. Use the piano and 808. Export it as `Sting_GameOver.wav`.

Save the FL project as `SourceArt/Music/CombatTheme/CombatTheme.flp`.

Then tell Claude. The game layers the stems by wave, plays the stings, and runs everything through the Music volume slider.
