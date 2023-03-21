## GUIX-KICKSTART

### INTRODUCTION
are you -
- Tired of setting up your pc from scratch each time after you distrohop?
- Tired of the dependency hell and packages breaking because of it?
- Tired of learning 1000nds of package managers that do the same exact thing?
- Tired of Packages only working for one specific distro and having a limited pool of packages?
- Tired of Touching something and completly bricking your system?
- Tired of Not Being Able To Customize Each Part of your system?

### DONT WORRY

i/contributors created this repo just for you, this repo will handhold you with some of the most detailed documentation you will ever find, you will : 

1. learn how to package/compile something yourself from source or from another packaging format along using something i call 'Gentoo flags'

2. learn how to declare your entire configuration inside of a git repo and automatically symlink it with Gnu Stow and sync it across youw machines with git.

3. learn how to manage users or profiles so you can setup your pc for multiple users and having multiple isolated profiles for each work enviroment. 

4. learn how to make isolated dev enviroments for your projects.

5. learn how to create your custom channels or replace guix upstream with your mirror.

6. learning the basics of guile scheme(easier than python), and learn some tools that will help you immensely with that


### SO
this will help you slowly transition your entire configuration into a reproducible workstation that you can use across all of your machines on the go, on the fly, EVERYWHERE.

this is GUIX, this. is... GUIX-KICKSTART.

## END OF THE INTRODUCTION

## The Start of Your journey

ok you will encounter some challenges making this configuration. but i will assure you this will be worth it. because after learning this package manager and a little of guile scheme you will never ever EVER have to learn another package manager again.

DO NOT JUMP STRAIGHT INTO THIS and try to do everything at once, take your time reading the documentation ,testing stuff,trying out stuff,failing ,remind yourself that its a slow learning process and you should be patient and disciplined to achieve your goal.

i reccomend you to start out with a WSL debian instance or a virtual machine and slowly build it from there untill you thoughtfully tested it and it works 100 % of the time without issues ,you are ready to jump.

so what are we waiting for, lets go! (put install script there)

documentation will be inside of the github repo and can be invoke it with man guix-kickstart

## KEEP IN MIND

this repo is still under construction and mainainter is working hard on it!

### ROADMAP

1. make an shell init script that will do all the bootstraping heavylifting for you.

2. make an uninstallation script that will purge guix cleanly from your system if you did not like it also restoring your configs to defaults.

3. setup gnu stow automatic symlink hook, along with guix executing our symlink script if guix and guix-config points to the wrong folder.


