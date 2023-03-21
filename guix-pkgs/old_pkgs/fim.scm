;; -*- mode: guile -*-

(define-module (guix packages fim
  #:use-module (guix build-system gnu)
  #:use-module (guix download)
  #:use-module (guix packages)
  #:use-module (guix build-system make)
  #:use-module (gnu packages)))



(define-public fim
	(package "fim")
	(version "0.6")
	(source (origin
		  (method url-fetch)
		  (uri (string-append "https://download.savannah.nongnu.org/releases/fbi-improved/fim-0.6.tar.gz")
		  (sha256 "11n4n4sc8zbpkb1ph9fv74kb7gvl2vml8vh7jzckmlnzmw7msz43" ))))
	(build-system gnu-build-system)
	;; decompiling x11 support since we use wayland
	(inputs
	  '(("imlib2" ,imlib2)
	    ("libxinerama" ,libxinerama)
	    ("libxft" ,libxft)
	    ("libxext" ,libxext)))
	(arguments
	  '(#:make-flags
	    (list (string-append "IMLIB2_CFLAGS=-I" (assoc-ref %build-inputs "imlib2")
				 "/include")
		  (string-append "IMLIB2_CFLAGS=-L" (assoc-ref %build-inputs "imlib2")
				 "/lib -limlib2")
		  (string-append "XINERAMA_LIBS=-L" (assoc-ref %build-inputs "libxinerama")
				 "lib -lXinerama")
	          (string-append "XFT_LIBS=-L" (assoc-ref %build-inputs "libxft")
				 "/lib -lXft")
		  (string-append "XEXT_LIBS=-L" (assoc-ref %build-inputs "libxext")
				 "/lib -lXext"))
	#:tests? #f))
   (home-page "https://nongnu.org/fbi-imporoved")
   (synopsis "Fbi Improved, a framebuffer image viewer based on Fbi")
   (description "FIM is a lightweight universal image viewer,mostly for framebuffer and X,
		Supports the following image formats : BMP, GIF, JPEG, LBM, PCD, PCX, PNG, PNM/PGM/PPM, SVG, TGA, TIFF, XBM and XPM.")
   (license license:gpl2+))
		  
