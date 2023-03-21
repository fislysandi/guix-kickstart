(use-modules (gnu packages image)
             (gnu packages ncurses)
             (gnu packages pkg-config)
             (gnu packages gtk))

(define-public fim-viewer
  (package
    (name "fim-viewer")
    (version "0.6")
    (source (origin
              (method url-fetch)
              (uri "https://download.savannah.nongnu.org/releases/fbi-improved/fim-0.6.tar.gz")
              (sha256 "11n4n4sc8zbpkb1ph9fv74kb7gvl2vml8vh7jzckmlnzmw7msz43"))))
    (build-system gnu-build-system)
    (inputs
      `(("gtk+" ,gtk+)
        ("ncurses" ,ncurses)))
    (home-page "https://www.nongnu.org/fbi-improved/")
    (synopsis "FIM - a lightweight universal image viewer")
    (description "FIM is a lightweight universal image viewer, mostly for Linux (but portable). FIM aims to be a highly customizable and scriptable image viewer for users who are comfortable with software like the Vim text editor or the Mutt mail user agent.")
    (license gpl3+)))
