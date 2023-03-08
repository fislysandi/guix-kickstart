#!/bin/bash

#prompt user for sudo password


curl -LO https://mirror.hydra.gnu.org/guix/guix-binary-latest.tar.xz


sudo tar xvf guix-binary-latest.tar.xz -C /usr/local

echo 'export PATH="$PATH:/usr/local/guix/current/bin"' >> ~/.bashrc
source ~/.bashrc

guix pull


