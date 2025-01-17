#!/bin/python3
# functions needed across projects that make vulnerability research easier
from pwn import *

import argparse, re
import struct, sys, traceback

if len(sys.argv) != 3 or sys.argv[1] != "-f":
    print(f"[!] Usage: python -i {sys.argv[0]} -f FILE_NAME")
elif sys.argv[1] == "-f":
    context = ELF(f"{sys.argv[2]}")

DYNELF = None
DYNELF_SECTIONS = {}
PAYLOAD_FNAME = "test_32.txt"
PROC = None
SECTIONS = {}
W_SEGMENTS = {}

options = {
    0:"disassemble(WHAT,N_BYTES)",
    1:"list_functions()",
    2:"list_symbols()",
    3:"save_payload(PAYLOAD)",
    4:"read_payload(F_NAME) -> PAYLOAD",
    5:"list_libraries()",
    6:"start_proc() -> PROC",
    7:"load_dyn_lib(DYN_LIB_PATH) -> DYNELF",
    8:"list_dyn_functions()",
    9:"print_dynelf_got()",
    10:"print_dynelf_plt()",
    11:"print_got()",
    12:"print_plt()",
    13:"list_dynelf_segments()",
    14:"disassemble_dyn(WHAT,N_BYTES)",
    15:"dump_bin(START=0,END=-1)",
    16:"search(WHAT) -> START,END",
    17:"write(WHAT,WHERE)",
    18:"list_sections()",
    19:"print_section(SECTION_NAME)",
    20:"print_bin(START=0,END=-1)",
    21:"list_w_segments()",
    22:"list_w_segment_tags(SEGMENT_IDX)",
    23:"list_w_segment_symbols(SEGMENT_IDX)",
    24:"print_w_segment__tag_tbl_offset(SEGMENT_IDX,TAG_NAME)",
    25:"get_w_segment_tag(SEGMENT_IDX,TAG_NUMBER) -> DYNAMIC_TAG",
    36:"help()"
}

def help():
    print("\nPWN_H.FUNCTION_NAME (ARG_0,...,ARG_N)\n")
    for k in options:
        print(f"[*] {options[k]}")

def disassemble(what,n_bytes):
    try:
        if isinstance(what,str):
            print(f"{context.disasm(context.sym[what],n_bytes)}")
        elif isinstance(what,int):
            print(f"{context.disasm(what,n_bytes)}")
    except:
        print(f"[x] {traceback.format_exc()}")

def disassemble_dyn(what,n_bytes):
    if not DYNELF:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
        return
    try:
        if isinstance(what,str):
            print(f"{DYNELF.elf.disasm(DYNELF.elf.sym[what],n_bytes)}")
        elif isinstance(what,int):
            print(f"{DYNELF.elf.disasm(what,n_bytes)}")
    except:
        print(f"[x] {traceback.format_exc()}")

def dump_bin(start=0,end=-1):
    try:
        if not PROC:
            start_proc()
        PROC.hexdump(PROC.elf.data[start:end])
    except:
        print(f"[x] {traceback.format_exc()}")

# def dump_dyn_lib(start=None,end=None):
#     if not DYNELF:
#         print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
#         return
#     try:
#         if not start and not end:
#             DYNELF.elf.hexdump(DYNELF.elf.data)
#         elif start and not end:
#             DYNELF.elf.hexdump(DYNELF.elf.data[start:])
#         else:
#             DYNELF.elf.hexdump(DYNELF.elf.data[start:end])
#     except:
#         print(f"[x] {traceback.format_exc()}")

def get_w_segment_tag(segment_idx, tag_number):
    """
    quick index into a writable segment. a call to list_w_segment_tags
    will provide the tag number.
    """
    tag = None
    try:
        tag = W_SEGMENTS[segment_idx].get_tag(tag_number)
        print(f"[*] {tag.__str__()}")
        print(f"[*] {tag.__repr__()}")
    except:
        print(f"[x] {traceback.format_exc()}")
    return tag

def init():
    set_sections()
    set_w_segments()

def leak(address):
    data = PROC.elf.read(address, 4)
    log.debug("%#x => %s", address, enhex(data or b''))
    return data

def list_dyn_functions():
    if DYNELF:
        print("\nDYNFUNCTION_NAME (DYNFUNCTION_ADDRESS)\n")
        for df_name,df_info in DYNELF.elf.sym.items():
            print(f"[*]\t{df_name} ({hex(df_info)})")
    else:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")

def list_dynelf_segments():
    if not DYNELF:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
        return
    try:
        print("\nDYNLIB_SEGMENT_START - DYNLIB_SEGMENT_END (DYNLIB_SEGMENT_SIZE BYTES)\n")
        memory_segments = DYNELF.elf.memory.items()
        idx = 0
        while len(memory_segments) != 0:
            segment = memory_segments.pop()
            idx += 1
            print(f"[*] segment-{idx}: {hex(segment[0])} - {hex(segment[1])} ({segment[1]-segment[0]} bytes)")
    except:
        print(f"[x] {traceback.format_exc()}")

def list_functions():
    print("\nFUNCTION_NAME (FUNCTION_SIZE)\n")
    for f_name,f_info in context.functions.items():
        print(f"[*]\t{f_name} ({hex(f_info.size)} bytes)")

def list_libraries():
    print("\nLIBRARY_NAME (LIBRARY_ADDRESS)\n")
    for lib_name,lib_addr in context.libs.items():
        print(f"[*]\t{lib_name} ({hex(lib_addr)})")

def list_sections():
    print("\nSECTION_NAME (SECTION_OFFSET, SECTION_SIZE)\n")
    for section_name,section in SECTIONS.items():
        print(f"[*]\t{section_name} (offset: {hex(section.header['sh_offset'])}, size: {hex(section.header['sh_size'])})")

def list_symbols():
    print("\nSYMBOL_NAME (SYMBOL_ADDRESS)\n")
    for s_name,s_addr in context.symbols.items():
        print(f"[*]\t{s_name} ({hex(s_addr)})")

def list_w_segments():
    print("\nSEGMENT_HEADER\n")
    unwrap_container(W_SEGMENTS,1)

def list_w_segment_symbols(segment_idx):
    try:
        it = W_SEGMENTS[segment_idx].iter_symbols()
        print("\nSEGMENT_SYMBOL\n")
        while True:
            try:
                symbol = it.__next__()
            except:
                break
            print("[*]")
            print(f"\tname: {symbol.name}")
            unwrap_container(symbol.entry,2)
            print()
    except:
        print(f"[x] {traceback.format_exc()}")

def list_w_segment_tags(segment_idx):
    try:
        it = W_SEGMENTS[segment_idx].iter_tags()
        print("\nSEGMENT_INFO\n")
        N = 0
        while True:
            try:
                tag = it.__next__()
            except:
                break
            print(f"[*] tag-#: {N}")
            unwrap_container(tag.entry,2)
            print()
            N += 1
    except:
        print(f"[x] {traceback.format_exc()}")

def load_dyn_lib(dyn_lib_path):
    global DYNELF
    if not PROC:
        start_proc()
    DYNELF = DynELF(leak,PROC.elf.sym["main"],elf=ELF(dyn_lib_path))
    try:
        set_dyn_sections()
    except:
        print(f"[x] {traceback.format_exc()}")

def print_bin(start=0,end=-1):
    print(f"{context.data[start:end]}")

def print_dynelf_got():
    if not DYNELF:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
        return
    print("\nDYNELF_GOT_SYMBOL_NAME (DYNELF_GOT_SYMBOL_ADDRESS)\n")
    for df_name,df_addr in DYNELF.elf.got.items():
        print(f"[*]\t{df_name} ({hex(df_addr)})")

def print_dynelf_plt():
    if not DYNELF:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
        return
    print("\nDYNELF_PLT_SYMBOL_NAME (DYNELF_PLT_SYMBOL_ADDRESS)\n")
    for df_name,df_addr in DYNELF.elf.plt.items():
        print(f"[*]\t{df_name} ({hex(df_addr)})")

def print_got():
    if not PROC:
        start_proc()
    print("\nGOT_SYMBOL_NAME (GOT_SYMBOL_ADDRESS)\n")
    for s_name, s_addr in PROC.elf.got.items():
        print(f"[*]\t{s_name} ({hex(s_addr)})")

def print_plt():
    if not PROC:
        start_proc()
    print("\nPLT_SYMBOL_NAME (PLT_SYMBOL_ADDRESS)\n")
    for s_name, s_addr in PROC.elf.plt.items():
        print(f"[*]\t{s_name} ({hex(s_addr)})")

def print_section(section_name):
    try:
        print(f"[*]\n{SECTIONS[section_name].data()}")
    except:
        print(f"[x] {traceback.format_exc()}")

def print_w_segment__tag_tbl_offset(segment_idx,tag_name):
    try:
        addr, size = W_SEGMENTS[segment_idx].get_table_offset(tag_name)
        print("\nTAG_ADDRESS (TAG_SIZE)\n")
        print(f"[*] {tag_name}: {hex(addr)} ({hex(size)})")
        print()
    except:
        print(f"[x] {traceback.format_exc()}")

def read_payload(f_name):
    fd = open(f_name, "r", errors="backslashreplace")
    payload = "".join( line for line in fd.readlines() )
    fd.close()
    return payload

def save_payload(payload):
    payload = str(payload)[2:-1]
    p = process(f"python2.7 -c 'print \"{payload}\"' > {PAYLOAD_FNAME}",
        shell=True, stderr=open('/dev/null', 'w+b'))

get_match = lambda m: m.span()
def search(what):
    match_idxs = []
    if isinstance(what,bytes):
        match_idxs = list(map(get_match,list(re.finditer(what,context.data))))
    else:
        print(f"[!] Usage: search( [ BYTES ] )")
    return match_idxs

def set_dyn_sections():
    if DYNELF_SECTIONS:
        DYNELF_SECTIONS.clear()
    if not DYNELF:
        print("[!] No dynamic library loaded. Call load_dyn_lib(DYN_LIB_PATH) first then try again")
        return
    try:
        [ DYNELF_SECTIONS.update({dyn_section.name:dyn_section}) for dyn_section in DYNELF.elf.sections ]
    except:
        print(f"[x] {traceback.format_exc()}")

def set_sections():
    [ SECTIONS.update({section.name:section}) for section in context.sections ]

def set_w_segments():
    [ W_SEGMENTS.update({idx:w_segment}) for idx,w_segment in enumerate(context.writable_segments) ]

def start_proc():
    global PROC
    PROC = process(context.path)

def unwrap_container(container,lvl):
    try:
        if not isinstance(container,dict):
            # these are the pwntools Containers that != python dicts
            for k,v in container.items():
                if isinstance(v,str):
                    print("\t"*(lvl-1) + f"{k}: {v}")
                elif isinstance(v,int):
                    print("\t"*(lvl-1) + f"{k}: {hex(v)}")
                else:
                    print("\t"*(lvl-1) + f"{k}:")
                    unwrap_container(v,lvl+1)
        else:
            # these are our custom containers since pwntools Containers != python dicts
            for k,element in container.items():
                try:
                    print(f"# {k}")
                    unwrap_container(element.header,lvl+1)
                    print()
                except:
                    print(f"[!][unwrap_container] {traceback.format_exc()}")
    except:
        print(f"[x] {traceback.format_exc()}")

def write(what,where):
    try:
        with open(context.path,"r+b") as fd:
            fd.seek(where)
            fd.write(what)
    except:
        print(f"[x] {traceback.format_exc()}")

init()
help()
