(define-public st-luke.scm
  (package
    (name "st-luke.scm")
    (version "0.8.4")
    (source (origin
              (method git-fetch)
              (uri "https://github.com/LukeSmithxyz/st.git")
              (branch "master")))
    (build-system gnu-build-system)
    (arguments
     `(#:phases
       (modify-phases %standard-phases
         (replace 'configure
           (lambda* (#:key outputs #:allow-other-keys)
             (zero? (system "make PREFIX=/usr/local")))
           )
         (replace 'install
           (lambda* (#:key outputs #:allow-other-keys)
             (zero? (system (string-append "make PREFIX=/usr/local DESTDIR="
                                           (assoc-ref %outputs "out") "/usr/local install")))
             #t))
         )))
    #:description "st-luke.scm is a fork of the suckless simple terminal (st) with additional features and patches maintained by Luke Smith. This version includes support for ligatures, Xresources, and more."))
