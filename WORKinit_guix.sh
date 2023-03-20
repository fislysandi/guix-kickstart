#!/bin/bash

# Create Guix directory and change ownership
sudo mkdir -p /var/guix/profiles/per-user/$USER
sudo chown -R $USER:$USER /var/guix/profiles/per-user/$USER

# Exit immediately if any command fails
set -e

echo "Downloading Guix installer..."
cd /tmp
sudo wget https://git.savannah.gnu.org/cgit/guix.git/plain/etc/guix-install.sh
sudo chmod +x guix-install.sh

echo "Running Guix installer..."
sudo ./guix-install.sh

echo "Updating package repository..."
sudo guix pull
