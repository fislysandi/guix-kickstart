(defun my-guile-eval-buffer ()
  (interactive)
  (if (eq major-mode 'scheme-mode)
      (progn
        (save-buffer)
        (async-shell-command (format "guile -c '(display (load \"%s\")) (newline)'" buffer-file-name))
        (display-buffer "*Async Shell Command*"))
    (message "Not in a Guile Scheme buffer.")))


;(map! :map scheme-mode-map
;      :localleader
;      :desc "Evaluate buffer" "e g" #'my-guile-eval-buffer)

;(map! :map scheme-mode-map
;      :localleader
;      :desc "Evaluate whole buffer" "E g" #'my-scheme-mode-hook)

(map! :map scheme-mode-map
      :localleader
      (:prefix "e"
       :desc "Evaluate buffer" "g" #'my-guile-eval-buffer
       :desc "Evaluate expression" "e" #'my-guile-eval-expression))
(map! :map scheme-mode-map
      :localleader
      (:prefix "E"
       :desc "Evaluate buffer" "G" #'my-guile-eval-buffer
       :desc "Evaluate expression" "e" #'my-guile-eval-expression-selected))
