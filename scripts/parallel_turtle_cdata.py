#!/usr/bin/env python3
"""Up-to-eight-process raw DEFLATE encoder for GoldenEye CDATA (parallel build path)."""
import argparse
import concurrent.futures
import ctypes
import ctypes.util
import pathlib
import subprocess
import tempfile
import zlib

class ZStream(ctypes.Structure):
    _fields_ = [
        ('next_in', ctypes.c_void_p), ('avail_in', ctypes.c_uint),
        ('total_in', ctypes.c_ulong), ('next_out', ctypes.c_void_p),
        ('avail_out', ctypes.c_uint), ('total_out', ctypes.c_ulong),
        ('msg', ctypes.c_void_p), ('state', ctypes.c_void_p),
        ('zalloc', ctypes.c_void_p), ('zfree', ctypes.c_void_p),
        ('opaque', ctypes.c_void_p), ('data_type', ctypes.c_int),
        ('adler', ctypes.c_ulong), ('reserved', ctypes.c_ulong),
    ]

LIBZ = ctypes.CDLL(ctypes.util.find_library('z') or 'libz.so.1')
LIBZ.zlibVersion.restype = ctypes.c_char_p
LIBZ.inflateInit2_.argtypes = [ctypes.POINTER(ZStream), ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
LIBZ.inflate.argtypes = [ctypes.POINTER(ZStream), ctypes.c_int]
LIBZ.inflateEnd.argtypes = [ctypes.POINTER(ZStream)]

def boundaries(data, expected):
    """Return (final_block_header_bit, end_of_stream_bit) using zlib Z_BLOCK."""
    s=ZStream()
    ibuf=ctypes.create_string_buffer(data)
    obuf=ctypes.create_string_buffer(len(expected) + 1024)
    s.next_in=ctypes.addressof(ibuf)
    s.avail_in=len(data)
    s.next_out=ctypes.addressof(obuf)
    s.avail_out=len(obuf)
    rc=LIBZ.inflateInit2_(ctypes.byref(s),-15,LIBZ.zlibVersion(),ctypes.sizeof(s))
    if rc != 0: raise RuntimeError('zlib init failed '+str(rc))
    prev=0
    final=None
    end=None
    try:
        for i in range(10000):
            last_total_in=s.total_in
            last_total_out=s.total_out
            rc=LIBZ.inflate(ctypes.byref(s),5)  # Z_BLOCK
            e=s.total_in*8-(s.data_type & 7)
            if s.data_type & 64:
                final=prev
                end=e
                break
            if rc==1: # Z_STREAM_END
                final=prev
                end=e
                break
            if rc!=0: raise RuntimeError(f'inflate status {rc} at step {i}')
            if e <= prev or (s.total_in==last_total_in and s.total_out==last_total_out):
                raise RuntimeError('inflate did not move to next block')
            prev=e
        else: raise RuntimeError('too many blocks')
        if bytes(obuf.raw[:s.total_out]) != expected:
            raise RuntimeError('deflate stream decoded to unexpected bytes')
        if final is None or end is None or end > len(data)*8:
            raise RuntimeError('no final DEFLATE block')
        # The final header BFINAL bit must be 1.
        if (data[final // 8] >> (final % 8)) & 1 != 1:
            raise RuntimeError('last DEFLATE block has BFINAL=0')
        return final,end
    finally:
        LIBZ.inflateEnd(ctypes.byref(s))


def append_bits(out, data, start, end, bitpos, clear_bit_at=None):
    # Write raw compressed bits LSB-first, allow unaligned joins
    for bit in range(start,end):
        val=(data[bit//8]>>(bit%8))&1
        if bit == clear_bit_at: val=0
        ix=bitpos//8
        if ix == len(out): out.append(0)
        out[ix] |= val << (bitpos%8)
        bitpos+=1
    return bitpos


def join_streams(compressed, chunks):
    out=bytearray()
    bitpos=0
    for i,(deflate,orig) in enumerate(zip(compressed,chunks)):
        final,end=boundaries(deflate,orig)
        bitpos=append_bits(out,deflate,0,end,bitpos,final if i < len(chunks)-1 else None)
    dec=zlib.decompressobj(-15)
    result=dec.decompress(bytes(out))+dec.flush()
    if result != b''.join(chunks) or not dec.eof or dec.unused_data or dec.unconsumed_tail:
        raise ValueError('strict joined-stream verification failed')
    return bytes(out)


def compress_job(task):
    i,chunk,executable,tempdir,level=task
    path=pathlib.Path(tempdir)/f'chunk{i:02}.bin'
    path.write_bytes(chunk)
    cmd=[executable,'--raw']
    if level==9: cmd+=['--9']
    cmd+=[str(path)]
    print(f'worker {i+1}: {len(chunk)} bytes, launching '+(' '.join(cmd[:3])),flush=True)
    subprocess.run(cmd,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=1200,check=True)
    output=pathlib.Path(str(path)+'.deflate')
    if not output.is_file(): raise RuntimeError(f'worker {i+1} created no DEFLATE output')
    packed=output.read_bytes()
    if zlib.decompress(packed,-15)!=chunk: raise RuntimeError(f'worker {i+1} raw decompression failed')
    print(f'worker {i+1}: done ({len(packed)} bytes)',flush=True)
    return packed


def main():
    p=argparse.ArgumentParser()
    p.add_argument('input',type=pathlib.Path)
    p.add_argument('output',type=pathlib.Path)
    p.add_argument('--bin',required=True)
    p.add_argument('--workers',type=int,default=8)
    p.add_argument('--level',type=int,choices=(7,9),default=9)
    a=p.parse_args()
    if not 1<=a.workers<=8: p.error('workers must be 1..8')
    src=a.input.read_bytes()
    chunks=[src[len(src)*i//a.workers:len(src)*(i+1)//a.workers] for i in range(a.workers)]
    with tempfile.TemporaryDirectory(prefix='goldeneye_turtle8_') as t:
        jobs=[(i,chunks[i],a.bin,t,a.level) for i in range(a.workers)]
        with concurrent.futures.ProcessPoolExecutor(max_workers=a.workers) as ex:
            results=list(ex.map(compress_job,jobs))
    final=b'\x11\x72'+join_streams(results,chunks)
    if len(final)>72704:
        raise RuntimeError(f'{a.workers}-worker candidate {len(final)} exceeds CDATA limit 72704')
    a.output.write_bytes(final)
    print(f'{a.workers}-worker final CDATA: {len(final)} bytes; available {72704-len(final)}',flush=True)

if __name__=='__main__':main()
