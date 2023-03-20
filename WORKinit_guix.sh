#!/bin/bash

# sometimes Guix Doesnt make this folder so i do that manuallly
sudo mkdir -p /var/guix/profiles/per-user/$USER
sudo chown -R $USER:$USER /var/guix/profiles/per-user/$USER

# Exit immediately if any command fails
set -e

# installation script pulled from official documentation
echo "Downloading Guix installer..."
cd /tmp
sudo wget https://git.savannah.gnu.org/cgit/guix.git/plain/etc/guix-install.sh
sudo chmod +x guix-install.sh


# run the installer
echo "Running Guix installer..."
sudo ./guix-install.sh


# updating guix
echo "Updating package repository..."
sudo guix pull

# installing git
guix install git --no-substitutes

mkdir ~/Desktop
cd ~/Desktop
git clone https://github.com/fislysandi/guix-kickstart

cd guix-kickstart/guix-config/

guix home reconfigure home-kickstart.scm --no-substitutes
