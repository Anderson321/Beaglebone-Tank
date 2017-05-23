#!/bin/bash

ker=HBridge.ko
devfile=/dev/hbridge

find . -type f | xargs -n 5 touch
make clean
make

# remove kernel module if it exists
rmmod $ker
#insert kernel module
insmod $ker

# Create a new devfile if it doesn't exist
if [ ! -e $devfile ]; then
  mknod $devfile c 239 0
  echo "devfile created: $devfile"
fi

# Set up the ADC
./adc

# Run the program that controls the movements of tank
./roll

