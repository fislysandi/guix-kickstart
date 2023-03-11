#!/bin/bash

#prompt user for sudo password
cd /tmp
curl -L -o guix-install.sh https://git.savannah.gnu.org/cgit/guix.git/plain/etc/guix-install.sh
chmod +x guix_install.sh
./guix-install.sh

guix pull


