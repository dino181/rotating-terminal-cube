#!/bin/bash

gcc -o main.out main.c -lm -lncurses

if [ $? -eq 0 ]; then
	echo "[COMPILE AND RUN]: Compiled succesfully. running program..."
	./main.out
else 
	echo "[COMPILE AND RUN]: Failed to compile. See error log above"
	exit 1
fi 
