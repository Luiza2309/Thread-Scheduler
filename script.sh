#!/bin/bash

make 
mv libscheduler.so ../checker-lin/
cd ../checker-lin
make -f Makefile.checker
cd ../util