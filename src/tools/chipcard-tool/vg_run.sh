#!/bin/bash

valgrind --tool=memcheck --trace-children=yes -v --log-file=MEMERR --leak-check=full --show-reachable=yes --track-origins=yes --num-callers=50 --keep-stacktraces=alloc-and-free ./chipcard-tool atr
