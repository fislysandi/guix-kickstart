#!/bin/bash

# Get the root path of the project
ROOT="$(pwd | rev | cut -d '/' -f 3- | rev)"

# Define the paths to the nix and nixpkgs folders
guix_path="$ROOT/guix-config"


# Define the paths to the existing symlinks
guix_link="$HOME/.config/guix-config"


# Check if the symlinks exist and are correct, otherwise remove them
if [[ -L $guix_link && "$(readlink $nixpkgs_link)" == "$nixpkgs_path" ]]; then
    echo "$guix_link already exists and is correct"
else
    rm -rf $guix_link
    ln -s $guix_path 
    echo "Symlink created for nixpkgs"
fi

