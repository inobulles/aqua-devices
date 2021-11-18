#!/bin/sh
cc -shared -fPIC main.c -o device -lpng "$@"
