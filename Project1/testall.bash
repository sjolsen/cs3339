#!/bin/bash

for i in {0..9}; do
        echo "$i: "
        ./disassembler test$i.mips | diff -w - test$i.dis
done
