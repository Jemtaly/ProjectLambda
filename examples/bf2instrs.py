#!/usr/bin/env python3
import argparse
import sys
import subprocess
def bf2instrs(bf, pf, ml = False):
    ind, sp, end = ('    ', '\n', '\n') if ml else ('', ' ', '')
    cnt = 0
    res = ''
    for ch in bf:
        if ch in '><+-.,':
            res += f'{cnt * ind}{pf}Instrs ({pf}{ch}) $ |{sp}'
        if ch == '[':
            res += f'{cnt * ind}{pf}Instrs ({pf}[] $ |{sp}'
            cnt += 1
        if ch == ']':
            res += f'{cnt * ind}{pf}Instop) $ |{sp}'
            cnt -= 1
    res += f'{cnt * ind}{pf}Instop{end}'
    assert cnt == 0
    return res
def enc(s):
    return ''.join(f'\op $op {ord(c)} ' for c in s) + '...'
def dec(l):
    return ''.join(chr(int(x)) for x in l.split(' ')[-2::-3])
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input', nargs = '?', type = argparse.FileType('r'), default = sys.stdin)
    ap.add_argument('-o', '--output', type = argparse.FileType('w'), default = sys.stdout)
    eg = ap.add_mutually_exclusive_group(required = True)
    eg.add_argument('-v', '--cbv', action = 'store_const', const = '&', dest = 'prefix')
    eg.add_argument('-n', '--cbn', action = 'store_const', const = '$', dest = 'prefix')
    args = ap.parse_args()
    args.output.write(bf2instrs(args.input.read(), args.prefix, True))
def test():
    ap = argparse.ArgumentParser()
    ap.add_argument('bf', type = argparse.FileType('r'))
    ap.add_argument('-i', '--input', type = argparse.FileType('r'), default = sys.stdin)
    ap.add_argument('-o', '--output', type = argparse.FileType('w'), default = sys.stdout)
    eg = ap.add_mutually_exclusive_group(required = True)
    eg.add_argument('-v', '--cbv', action = 'store_const', const = 'cbv', dest = 'choice')
    eg.add_argument('-n', '--cbn', action = 'store_const', const = 'cbn', dest = 'choice')
    args = ap.parse_args()
    pf = {'cbv': '&', 'cbn': '$'}[args.choice]
    l = open(f'bf_{args.choice}.λ').read()
    l += f':fun {bf2instrs(args.bf.read(), pf)}\n'
    l += f':input {enc(args.input.read())}\n'
    l += f'cal {pf}run {pf}fun {pf}input\n'
    args.output.write(dec(subprocess.check_output([f'../build/lambda_{args.choice}'], input = l.encode()).decode()))
if __name__ == '__main__':
    test()
