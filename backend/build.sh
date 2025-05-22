#!/bin/sh

set -xe

if [ ! -d ./third_party/mongo-c-driver/_build ]; then
    cd ./third_party/mongo-c-driver && cmake -B _build && cmake --build _build
    cd ../..
fi

cc -o backend -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wno-unused-value -I./third_party/mongo-c-driver/_build/src/libbson/src/ -I./third_party/mongo-c-driver/_build/src/libmongoc/src/ -I./third_party/mongo-c-driver/src/libmongoc/src/ -I./third_party/mongo-c-driver/src/libbson/src/ -L./third_party/mongo-c-driver/_build/src/libmongoc -L./third_party/mongo-c-driver/_build/src/libbson -lbson2 -lmongoc2 -g main.c http.c db.c
