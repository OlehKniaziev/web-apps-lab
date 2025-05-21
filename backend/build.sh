#!/bin/sh

set -xe

cc -o backend -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror main.c http.c
