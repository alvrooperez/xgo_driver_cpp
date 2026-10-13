#!/usr/bin/env python3
import time
import struct
import os
import resource

# Funciones extraidas directamente de xgolib_dog.py
def conver2u8(data, limit, min_value=0):
    max_value = 0xff
    if not isinstance(limit, list):
        limit = [-limit, limit]
    if data >= limit[1]:
        return max_value
    elif data <= limit[0]:
        return min_value
    else:
        return int(255 / (limit[1] - limit[0]) * (data - limit[0]))

def Byte2Float(rawdata):
    a = bytearray()
    a.append(rawdata[3])
    a.append(rawdata[2])
    a.append(rawdata[1])
    a.append(rawdata[0])
    return struct.unpack("!f", a)[0]

def create_write_packet_py(addr, val):
    tx = [0x55, 0x00, 0x09, 0x00, addr, val]
    sum_data = (0x09 + 0x00 + addr + val) % 256
    sum_data = 255 - sum_data
    tx.extend([sum_data, 0x00, 0xAA])
    return tx

def parse_stream_py(stream):
    rx_FLAG = 0
    rx_COUNT = 0
    rx_ADDR = 0
    rx_LEN = 0
    rx_TYPE = 0
    rx_data = bytearray(50)
    packets_found = 0

    for num in stream:
        if rx_FLAG == 0:
            if num == 0x55:
                rx_FLAG = 1
        elif rx_FLAG == 1:
            if num == 0x00:
                rx_FLAG = 2
            elif num == 0x55:
                rx_FLAG = 1
            else:
                rx_FLAG = 0
        elif rx_FLAG == 2:
            rx_LEN = num
            rx_FLAG = 3
        elif rx_FLAG == 3:
            rx_TYPE = num
            rx_FLAG = 4
        elif rx_FLAG == 4:
            rx_ADDR = num
            rx_FLAG = 5
            rx_COUNT = 0
        elif rx_FLAG == 5:
            if rx_COUNT == (rx_LEN - 9):
                rx_data[rx_COUNT] = num
                rx_FLAG = 6
            elif rx_COUNT < rx_LEN - 9:
                rx_data[rx_COUNT] = num
                rx_COUNT += 1
        elif rx_FLAG == 6:
            rx_CHECK = 0
            for i in rx_data[0:(rx_LEN - 8)]:
                rx_CHECK += i
            rx_CHECK = 255 - (rx_LEN + rx_TYPE + rx_ADDR + rx_CHECK) % 256
            if num == rx_CHECK:
                rx_FLAG = 7
            else:
                rx_FLAG = 0
        elif rx_FLAG == 7:
            if num == 0x00:
                rx_FLAG = 8
            else:
                rx_FLAG = 0
        elif rx_FLAG == 8:
            if num == 0xAA:
                packets_found += 1
            rx_FLAG = 0

    return packets_found

def run_benchmark(N=1_000_000):
    print(f"=== BENCHMARK PYTHON ({N:,} iteraciones) ===")

    # 1. Test Conversiones y Creacion de Paquetes
    t0 = time.perf_counter()
    vx_lim = [-25, 25]
    for i in range(N):
        vel = -25.0 + (i % 50)
        b_vx = conver2u8(vel, vx_lim)
        pkt = create_write_packet_py(0x30, b_vx)
    t_conv = time.perf_counter() - t0
    print(f"  [1] Conversión + Creación de Paquetes: {t_conv*1000:.2f} ms ({t_conv/N*1e6:.3f} µs/op)")

    # 2. Test Desempaquetado de Float
    raw_float = bytes([0x00, 0x00, 0x48, 0xC1])
    t0 = time.perf_counter()
    for _ in range(N):
        f = Byte2Float(raw_float)
    t_float = time.perf_counter() - t0
    print(f"  [2] Byte2Float: {t_float*1000:.2f} ms ({t_float/N*1e6:.3f} µs/op)")

    # 3. Test Parsing de Flujo de Bytes
    sample_packet = [0x55, 0x00, 0x09, 0x02, 0x01, 0x5F, 0x94, 0x00, 0xAA]
    stream = sample_packet * (N // 10)
    t0 = time.perf_counter()
    found = parse_stream_py(stream)
    t_parse = time.perf_counter() - t0
    print(f"  [3] Parsing RX ({len(stream):,} bytes): {t_parse*1000:.2f} ms ({t_parse/len(stream)*1e6:.3f} µs/byte)")

    total_time = t_conv + t_float + t_parse
    mem_kb = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    print(f"  --> TIEMPO TOTAL PYTHON: {total_time*1000:.2f} ms")
    print(f"  --> Memoria RAM pico (RSS): {mem_kb / 1024:.2f} MB")
    return total_time, mem_kb

if __name__ == "__main__":
    run_benchmark()
