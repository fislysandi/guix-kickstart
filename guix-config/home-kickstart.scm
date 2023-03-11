;; This "home-environment" file can be passed to 'guix home reconfigure'
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
					    "keepassxc"
					    "dolphin"
					    "htop"
					    "nyxt"
					    "wayland"
					    )))

  ;; Below is the list of Home services.  To search for available
  ;; services, run 'guix home search KEYWORD' in a terminal.
  (services
   (list (service home-bash-service-type
                  (home-bash-configuration
                   (aliases '(("hbuild" . "home-manager build")
                              ("hswitch" . "home-manager switch")
                              ("l" . "ls -CF")
                              ("la" . "ls -A")
                              ("ll" . "ls -alF")
                              ("ls" . "ls --color=auto")))
                   (bashrc (list (local-file
                                  "/home/fislysandi/dev/git/guix_3drice/guix-config//.bashrc"
                                  "bashrc")))
                   (bash-logout (list (local-file
                                       "/home/fislysandi/dev/git/guix_3drice/guix-config//.bash_logout"
                                       "bash_logout"))))))))
