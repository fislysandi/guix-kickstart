#!/bin/sh


$(guix package --manifest=/ --profile=/run/current-system/profile/ | which emacsclient) --eval "(kill-emacs)"
