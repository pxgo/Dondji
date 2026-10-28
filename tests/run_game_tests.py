"""Compile real game/piano/snake/UI C sources against host-only stubs; no dependencies."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True, help="Path to an existing host GCC")
    args = parser.parse_args()
    cc = Path(args.cc).resolve()
    root = Path(__file__).resolve().parent.parent
    env = os.environ.copy()
    env["PATH"] = str(cc.parent) + os.pathsep + env.get("PATH", "")
    flags = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    defines = ["ENABLE_GAME_SNAKE", "ENABLE_FEAT_F4HWN", "ENABLE_FMRADIO",
               "ENABLE_NOAA", "ENABLE_FEAT_F4HWN_SLEEP",
               "ENABLE_FEAT_F4HWN_SCREENSHOT"]
    common = ["-D" + name for name in defines] + ["-Itests/game_stubs", "-IApp"]
    failures = []
    with tempfile.TemporaryDirectory(prefix="dondji-game-tests-") as directory:
        def run(name, sources, options):
            exe = str(Path(directory) / (name + ".exe"))
            command = [str(cc)] + flags + options + sources + ["-o", exe]
            print(subprocess.list2cmdline(command), flush=True)
            result = subprocess.run(command, cwd=root, env=env)
            if result.returncode:
                failures.append((name, options, "compile", result.returncode))
                return
            cases = ("language", "list", "board", "piano") if name == "ui" else (None,)
            for case in cases:
                invocation = [exe] + ([case] if case else [])
                print(subprocess.list2cmdline(invocation), flush=True)
                result = subprocess.run(invocation, cwd=root, env=env)
                if result.returncode:
                    failures.append((name, options, case, result.returncode))

        for optimization in ("-O0", "-O2"):
            run("snake", ["App/app/snake.c", "tests/snake_test.c"], [optimization])
            for vox in (False, True):
                run("session", ["App/app/snake.c", "App/app/game.c", "App/app/piano.c",
                                "tests/game_session_test.c"],
                    common + [optimization] + (["-DENABLE_VOX"] if vox else []))
            for chinese in (False, True):
                run("ui", ["App/app/snake.c", "App/app/game.c", "App/app/piano.c", "App/ui/game.c",
                           "tests/game_ui_test.c"],
                    common + [optimization, "-DENABLE_VOX"]
                    + (["-DENABLE_CHINESE"] if chinese else []))
    if failures:
        for name, options, case, code in failures:
            print(f"FAILED {name}/{case}: {options}, exit={code}")
        raise SystemExit(1)
    print("All game host tests passed; temporary executables removed.")


if __name__ == "__main__":
    main()
