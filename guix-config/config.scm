(use-modules (gnu) (gnu system nss) (guix packages))

;;(define %my-root-uuid
	;; Get UUID of the root partition using 'blkid' command
;;	(string-trim-right
;;	  (string-append
;;	    (car (string-tokenize (open-input-string
;;				    (string-append "blkid -s UUID -o value " %current-root-device))
;;				  #\newline))
;;	    "\n")))

(operating-system
  (host-name "fislypc")
  (timezone "Europe/Moscow")
  (locale "en_US.utf8")
  (bootloader (bootloader-configuration (bootloader grub-bootloader)))
  (file-systems 
    (cons* (file-system
	     (device "/dev/sda1"
	     (mount-point "/")
	     (type "ext4"))
	    %base-file-systems)
  (users (cons* (user-account
		  (name "fislysandi")
		  (group "users")
		  (home-directory "/home/fislysandi")
		  (shell #~"/bin/bash"))
		%base-user-accounts))
  (packages (cons* %base-packages)))

