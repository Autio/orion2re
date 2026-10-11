#!/usr/bin/env python3

import argparse
import hashlib
import struct
from pathlib import Path


LBX_HEADER_SIZE = 2048
LBX_MAGIC = 0xFEAD
LBX_OFFSET_COUNT = 510


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Extract entry payloads from an LBX archive.")
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--alias", action="append", default=[])
    return parser.parse_args()


def extract(source, output, aliases, expected_hash):
    data = source.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected_hash:
        raise ValueError(
            f"{source}: SHA-256 {digest} does not match {expected_hash}")
    if len(data) < LBX_HEADER_SIZE:
        raise ValueError(f"{source}: truncated LBX header")

    entry_count, magic = struct.unpack_from("<HH", data)
    if magic != LBX_MAGIC or entry_count >= LBX_OFFSET_COUNT:
        raise ValueError(f"{source}: invalid LBX header")
    offsets = struct.unpack_from(f"<{LBX_OFFSET_COUNT}I", data, 8)
    if offsets[0] < LBX_HEADER_SIZE or offsets[entry_count] != len(data):
        raise ValueError(f"{source}: invalid LBX entry offsets")

    destinations = [output, *(output.parent / alias for alias in aliases)]
    for destination in destinations:
        destination.mkdir(parents=True, exist_ok=True)
        for entry in range(entry_count):
            start = offsets[entry]
            end = offsets[entry + 1]
            if end < start or end > len(data):
                raise ValueError(f"{source}: invalid entry {entry}")
            (destination / f"{entry:03}").write_bytes(data[start:end])

    print(f"{source}: extracted {entry_count} entries")


def main():
    arguments = parse_arguments()
    extract(arguments.source, arguments.output, arguments.alias,
        arguments.sha256.lower())


if __name__ == "__main__":
    main()