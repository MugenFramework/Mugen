#!/bin/bash

if [ ! -d "../data/x86_64-w64-mingw32-cross" ]; then
	if [ ! -d "../data" ]; then
		mkdir -p ../data
	fi

	if [ ! -f /tmp/mingw-musl-64.tgz ]; then
		wget https://musl.cc/x86_64-w64-mingw32-cross.tgz -q -O /tmp/mingw-musl-64.tgz
	fi

	tar zxf /tmp/mingw-musl-64.tgz -C ../data

	if [ ! -f /tmp/mingw-musl-32.tgz ]; then
		wget https://musl.cc/i686-w64-mingw32-cross.tgz -q -O /tmp/mingw-musl-32.tgz
	fi

	tar zxf /tmp/mingw-musl-32.tgz -C ../data
fi
