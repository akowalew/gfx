#!/usr/bin/sh

CFLAGS="$CFLAGS -DBUILD_LINUX -O0 -g3 -ggdb -ffunction-sections -fdata-sections -Wl,--gc-sections"
WFLAGS="$WFLAGS -Wall -Wextra -Werror -Wfatal-errors -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wshadow -Wundef"
LFLAGS="$LFLAGS -lX11 -lGL"

set -ex

time gcc nix_text.c -o text $CFLAGS $WFLAGS $LFLAGS