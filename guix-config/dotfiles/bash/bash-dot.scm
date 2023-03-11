(use-modules (gnu))

;; symlink the .bashrc file
(file-symlink
  (string-append  (getenv "HOME") "/.bashrc")
  "dotfiles/bash/.bashrc")

