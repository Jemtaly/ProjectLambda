#!/usr/bin/env python3


import argparse
import subprocess
import sys
from pathlib import Path


def bf2instrs(bf: str, *, prefix: str, is_multiline: bool = False) -> str:
    if is_multiline:
        indent = "    "
        sep = "\n"
        end = "\n"
    else:
        indent = ""
        sep = " "
        end = ""
    count = 0
    res = ""
    for c in bf:
        if c in "><+-.,":
            res += f"{count * indent}{prefix}InstrCons ({prefix}{c}) $ |{sep}"
        if c == "[":
            res += f"{count * indent}{prefix}InstrCons ({prefix}[] $ |{sep}"
            count += 1
        if c == "]":
            res += f"{count * indent}{prefix}InstrNil) $ |{sep}"
            count -= 1
    res += f"{count * indent}{prefix}InstrNil{end}"
    assert count == 0
    return res


def enc(s: str) -> str:
    return "".join(f"\\op $op {ord(c)} " for c in s) + "..."


def dec(l: str) -> str:
    return "".join(chr(int(x)) for x in l.split(" ")[-2::-3])


PROJECT_DIR = Path(__file__).parent.parent.resolve()
BUILD_DIR = PROJECT_DIR / "build"
EXAMPLES_DIR = PROJECT_DIR / "examples"


def test():
    parser = argparse.ArgumentParser()
    subs = parser.add_subparsers(dest="command", required=True)

    run = subs.add_parser("run", help="Run a Brainfuck program")
    run.add_argument("bf", type=argparse.FileType("r"))
    run.add_argument("-i", "--input", type=argparse.FileType("r"), default=sys.stdin)
    run.add_argument("-o", "--output", type=argparse.FileType("w"), default=sys.stdout)
    run_x = run.add_mutually_exclusive_group(required=True)
    run_x.add_argument("-v", "--cbv", action="store_const", const="cbv", dest="choice")
    run_x.add_argument("-n", "--cbn", action="store_const", const="cbn", dest="choice")

    gen = subs.add_parser("gen", help="Generate instruction list from Brainfuck program")
    gen.add_argument("bf", nargs="?", type=argparse.FileType("r"), default=sys.stdin)
    gen.add_argument("-o", "--output", type=argparse.FileType("w"), default=sys.stdout)
    gen.add_argument("-m", "--multiline", action="store_true", help="Generate multiline output")
    gen_x = gen.add_mutually_exclusive_group(required=True)
    gen_x.add_argument("-v", "--cbv", action="store_const", const="cbv", dest="choice")
    gen_x.add_argument("-n", "--cbn", action="store_const", const="cbn", dest="choice")

    args = parser.parse_args()

    prefix = {"cbv": "&", "cbn": "$"}[args.choice]

    if args.command == "gen":
        bf_raw = args.bf.read()
        bf_enc = bf2instrs(bf_raw, prefix=prefix, is_multiline=args.multiline)
        args.output.write(bf_enc)
        return

    builds = sorted(BUILD_DIR.glob(f"lambda_{args.choice}*"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not builds:
        raise FileNotFoundError(f"No build found for lambda_{args.choice}")
    executable = str(builds[0])
    definition = EXAMPLES_DIR / f"bf_{args.choice}.λ"

    bf_raw = args.bf.read()
    bf_enc = bf2instrs(bf_raw, prefix=prefix)
    i_raw = args.input.read()
    i_enc = enc(i_raw)

    l = open(definition).read()
    l += f":fun {bf_enc}\n"
    l += f":input {i_enc}\n"
    l += f"cal {prefix}run {prefix}fun {prefix}input\n"

    o_enc = subprocess.check_output([executable], input=l.encode()).decode()
    o_raw = dec(o_enc)
    args.output.write(o_raw)


if __name__ == "__main__":
    test()
