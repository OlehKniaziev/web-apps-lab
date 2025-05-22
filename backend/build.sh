#!/bin/sh

set -xe

if [ ! -d ./third_party/mongo-c-driver/_build ]; then
    cd ./third_party/mongo-c-driver && cmake -B _build -DENABLE_STATIC=ON -DBUILD_VERSION="2.0.1" && cmake --build _build --parallel
    cd ../..
fi

LIBMONGOC_DIR="./third_party/mongo-c-driver/_build/src/libmongoc"
LIBBSON_DIR="./third_party/mongo-c-driver/_build/src/libbson"

cc -o backend -DMONGOC_STATIC -DBSON_STATIC -fPIC -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wno-unused-value -I./third_party/mongo-c-driver/_build/src/libbson/src/ -I./third_party/mongo-c-driver/_build/src/libmongoc/src/ -I./third_party/mongo-c-driver/src/libmongoc/src/ -I./third_party/mongo-c-driver/src/libbson/src/ -L$LIBMONGOC_DIR -L$LIBBSON_DIR -Wl,-rpath=$LIBMONGOC_DIR -Wl,-rpath=$LIBBSON_DIR -lmongoc2 -lbson2 -g main.c http.c db.c common.c json.c
