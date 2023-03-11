#!/bin/bash

# Get UUID of the root partition
uuid=$(lsblk -no UUID $(mount | grep 'on \/ ' | awk '{print $1}'))

# print the UUID
echo $uuid 

