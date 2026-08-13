#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA="$ROOT/data"

mkdir -p "$DATA"

if [ ! -d "$DATA/x86_64-w64-mingw32-cross" ]; then
	if [ ! -f /tmp/mingw-musl-64.tgz ]; then
		wget https://musl.cc/x86_64-w64-mingw32-cross.tgz -q -O /tmp/mingw-musl-64.tgz
	fi
	tar zxf /tmp/mingw-musl-64.tgz -C "$DATA"
fi

if [ ! -d "$DATA/i686-w64-mingw32-cross" ]; then
	if [ ! -f /tmp/mingw-musl-32.tgz ]; then
		wget https://musl.cc/i686-w64-mingw32-cross.tgz -q -O /tmp/mingw-musl-32.tgz
	fi
	tar zxf /tmp/mingw-musl-32.tgz -C "$DATA"
fi
