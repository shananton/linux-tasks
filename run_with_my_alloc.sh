#!/usr/bin/env bash
LD_PRELOAD="$PWD/malloc.so" "$@"
