 This "home-environment" file can be passed to 'guix home reconfigure'
;; to reproduce the content of your profile.  This is "symbolic": it only
;; specifies package names.  To reproduce the exact same profile, you also
;; need to capture the channels being used, as returned by "guix describe".
;; See the "Replicating Guix" section in the manual.

(use-modules (gnu home)
             (gnu packages)
             (gnu services)
             (guix gexp)
             (gnu home services shells))

(home-environment
  ;; Below is the list of packages that will show up in your
  ;; Home profile, under ~/.guix-home/profile.
  (packages (specifications->packages (list "guix"
											"htop"
											"stow"
											"guile"
											"neovim"
											"neovim-packer"
											"binutils"
											"gcc"
											"glibc"
											"clang-toolchain"
											"pandoc"
											"flex"
											"python"
											"tmux")))

  ;; Below is the list of Home services.  To search for available
  ;; services, run 'guix home search KEYWORD' in a terminal.
  (services
   (list (service home-bash-service-type
                  (home-bash-configuration
                   (aliases '(("gbuild" . "guix home container --no-substitutes")
                              ("grep" . "grep --color=auto")
                              ("gswitch" . "guix home switch --no-substitutes")
                              ("hbuild" . "home-manager build")
                              ("hswitch" . "home-manager switch")
                              ("l" . "ls -CF")
                              ("la" . "ls -A")
                              ("ll" . "ls -alF")
                              ("ls" . "ls --color=auto")))
                   (bashrc (list (local-file
                                  "/home/fislysandi/src/guix-config//.bashrc"
                                  "bashrc")))
                   (bash-profile (list (local-file
                                        "/home/fislysandi/src/guix-config//.bash_profile"
                                        "bash_profile")))
                   (bash-logout (list (local-file
                                       "/home/fislysandi/src/guix-config//.bash_logout"
                                       "bash_logout"))))))))



