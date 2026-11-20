"""
Imports the music WAVs into the project, where ULWMusicSubsystem looks for them:

  SourceArt/Music/MenuTheme/Export/*.wav    ->  /Game/Audio/Music/Menu/
  SourceArt/Music/CombatTheme/Export/*.wav  ->  /Game/Audio/Music/Combat/

Loops and stems are marked as looping; stings play once. Re-running replaces the previous import, so dropping an
FL Studio export over a placeholder of the same name and running this again swaps it in.

Run headless with the editor closed:
  UnrealEditor-Cmd.exe "<path>/LightWalker.uproject" -run=pythonscript -script="<path>/Tools/import_audio.py"
"""

import glob
import os

import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCES = {
    os.path.join(PROJECT_DIR, "SourceArt", "Music", "MenuTheme", "Export"): "/Game/Audio/Music/Menu",
    os.path.join(PROJECT_DIR, "SourceArt", "Music", "CombatTheme", "Export"): "/Game/Audio/Music/Combat",
}

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

for folder, destination in SOURCES.items():
    files = sorted(glob.glob(os.path.join(folder, "*.wav")))
    if not files:
        unreal.log_warning(f"[LW] No WAVs in {folder}")
        continue

    tasks = []
    for path in files:
        name = os.path.splitext(os.path.basename(path))[0]
        if name.startswith("_"):
            continue  # scratch files
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = destination
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    tools.import_asset_tasks(tasks)

    for task in tasks:
        asset_path = f"{destination}/{task.destination_name}"
        sound = assets.load_asset(asset_path)
        if not isinstance(sound, unreal.SoundWave):
            unreal.log_error(f"[LW] {asset_path} didn't import as a sound")
            continue
        looping = not task.destination_name.startswith("Sting_") and not task.destination_name.endswith("_Intro")
        sound.set_editor_property("looping", looping)
        # Silent stems must keep playing (not be virtualised and restarted), or they'd drift out of sync.
        sound.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
        assets.save_asset(asset_path, only_if_is_dirty=False)
        unreal.log(f"[LW] {asset_path} ({sound.get_editor_property('duration'):.2f} s{', looping' if looping else ''})")
