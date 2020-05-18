# Visual Studio External Tool:
# Hashes the current text selection with case insensitive Fnv1a algorithm.
# Configuration:
#   Tools > External Tools > Add
#   Title: fnv1a
#   Command: C:\Program Files\Python37\python.exe
#   Arguments: prehash.py $(CurText)
#   Initial Directory: $(ProjectDir)/../src/tools/
#   Use Output Window: On

import sys
import subprocess

def fnv1a(txt):
    if txt is None:
        return 0
    if len(txt) == 0:
        return 0
    y = 2166136261
    for c in txt:
        c = ord(c.upper())
        y = 0xffffffff & ((y ^ c) * 16777619)
    return y

def copy2clip(txt):
    cmd='echo '+txt.strip()+'|clip'
    return subprocess.check_call(cmd, shell=True)

text = sys.argv[1].upper()
result = str(fnv1a(text))
cexpr = "static const u32 %s_hash = %su;" % (text.lower(), result)
print("x: %s" % text)
print("fnv1a(x): %s" % result)
print(cexpr)
copy2clip(cexpr)
