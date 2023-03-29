#!/bin/sh


$(guix package --manifest=/ --profile=/run/current-system/profile/ | which emacs) --fg-daemon





