# Compatibility shim: makes HavocFramework/Modules scripts work with Mugen.
# Place this file next to any module script that does `import havoc`.
# The Mugen Python API exposes the same classes and methods as Havoc GPL:
#   Demon(agent_id) -> InlineExecute, DotnetInlineExecute, ConsoleWrite, Command
#   RegisterCommand, RegisterModule, RegisterCallback, GetDemons, GetListeners
from mugen import *
from mugen import RegisterTenguCommand, Tengu

# Packer is used by most HavocFramework modules without an explicit import.
# It was expected to be globally available after loading Packer/packer.py first.
# We bundle it here so any module that imports havoc gets Packer automatically.
from struct import pack, calcsize

class Packer:
    def __init__(self):
        self.buffer : bytes = b''
        self.size   : int   = 0

    def getbuffer(self):
        return pack("<L", self.size) + self.buffer

    def addstr(self, s):
        if s is None:
            s = ''
        if isinstance(s, str):
            s = s.encode("utf-8")
        fmt = "<L{}s".format(len(s) + 1)
        self.buffer += pack(fmt, len(s) + 1, s)
        self.size   += calcsize(fmt)

    def addWstr(self, s):
        if s is None:
            s = ''
        s = s.encode("utf-16_le")
        fmt = "<L{}s".format(len(s) + 2)
        self.buffer += pack(fmt, len(s) + 2, s)
        self.size   += calcsize(fmt)

    def addbytes(self, b):
        if b is None:
            b = b''
        fmt = "<L{}s".format(len(b))
        self.buffer += pack(fmt, len(b), b)
        self.size   += calcsize(fmt)

    def addbool(self, b):
        fmt = '<I'
        self.buffer += pack(fmt, 1 if b else 0)
        self.size   += 4

    def adduint32(self, n):
        fmt = '<I'
        self.buffer += pack(fmt, n)
        self.size   += 4

    def addint(self, n):
        self.buffer += pack("<i", n)
        self.size   += 4

    def addshort(self, n):
        fmt = '<h'
        self.buffer += pack(fmt, n)
        self.size   += 2
