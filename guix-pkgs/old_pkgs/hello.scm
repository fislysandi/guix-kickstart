;;; Guix package file for hello-world

(define-module (guix packages hello-world)
     #:use-module (guix build-system trivial)
     #:use-module (guix download))
     #:use-module (guix packages)

(define-public hello-world
	(package
	    (name "hello-world")
	    (version "1.0")
	    (source (origin
		      (method url-fetch)
		      (uri "http://ftp.gnu.org/gnu/hello/hello-2.10.tar.gz")
		      (sha256
			(base32
			  "cf04af86dc085268c5f4470fbae49b18afbc221b78096aab842d934a76bad0ab"))))
	   (build-system trivial-build-system)
	   (arguments
	     '(#:mopdules ((guix build utils))
	       (begin
		 (use-modules (guix build utils))
		 (mkdir-p %output)
		 (with-output-to-file (string-append %output "/hello.txt")
			(lambda ()
			  (format #t "Hello, World!~%"))))
	       #:phases
	        (modify-phases %standart-phases
			(add-before 'install ' rename-file
			  (rename-file
			    (string-append %output "/hello.txt")
			    (string-append %output "/hello"))
			  #t))))))



