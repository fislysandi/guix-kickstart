(use-modules (gnu) (gnu system nss) (guix packages))


(operating-system
  (host-name "fislypc")
  (timezone "Europe/Moscow")
  (locale "en_US.utf8")
  (bootloader (bootloader-configuration (bootloader grub-bootloader)))
  (file-systems (cons* (file-system
			(device "UUID=1234-5678")
			(mount-point "/")
			(type "ext4"))))	
  (users (cons* (user-account
		  (name "fislysandi")
		  (group "users")
		  (home-directory "/home/fislysandi")
		  (shell #~"/bin/bash"))
		%base-user-accounts))
  (packages (cons* %base-packages)))

