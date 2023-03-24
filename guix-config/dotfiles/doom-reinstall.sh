#!/bin/bash

# Copy ~/.doom.d/* to the current directory
cp -r ~/.doom.d/* .

# Remove ~/.emacs.d and ~/.doom.d
rm -rf $PWD/emacs/
rm -rf ~/.doom.d/

# Clone the repo to the current directory and rename it to emacs
git clone --depth 1 https://github.com/doomemacs/doomemacs $PWD/emacs

# Link the emacs folder to ~/.emacs.d
ln -sf $PWD/emacs ~/.emacs.d

#kill emacs daemon before installing doom
emacsclient -e '(kill-emacs)'
# Run 'emacs/bin/doom install' and automatically agree to everything
$PWD/emacs/bin/doom install

# Initialize emacs daemon
exec emacs --fg-daemon > /dev/null 2>&1 &
# Move the previously copied files back to .doom.d
cp -r $PWD/*.el ~/.doom.d/
# sync packages
doom sync
