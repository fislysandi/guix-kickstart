(use-modules (gnu packages hello))

(operating-system
  (host-name "kickstart")
  (timezone "Europe/Moscow")
  (locale "en_US.utf8")
  (enviroment-variables
    '("GUIX_PROFILE=~/.config/guix-config"))
  (packages(cons hello %base-packages)))
