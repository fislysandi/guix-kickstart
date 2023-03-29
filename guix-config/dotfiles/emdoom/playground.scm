(define (calculator)
  (display "Welcome to the calculator program!\n\n")
  (let loop()
    (display "Please enter an operation (+,-,*,/), or type "exit"
    (let ((op (read-line)))
        (cond
            ((equal? op "exit")
             (display "Exiting Calc programm\n")
            ((or (equal? op "+")
                 (equal? op "-")
                 (equal? op "/")
                 (equal? op "*"))
            ; Call the appropriate function based on the operator
            ; and display the result to the user
            (display "Enter the fisrt number: ")
            (let ((nume1 (read)))
                (display "Enter the second number: ")
            (let ((num2 (read)))))
            ; Call the appropriate function and display the result to the user
            (let ((result (calculate op num1 num2)))
                (display "The result is: ")
                (display result)
                (newline)))))
            (else
                (display "invalid operation, please try again.\n")
                (loop))))))

(define (calculate op num1 num2)
        (cond
          ((equal? op "+") (+ num1 num2))
          ((equal? op "-") (- num1 num2)
          ((equal? op "*") (* num1 num2)
          ((equal? op "/") (/ nun1 num2)
          (else (error "Invalid operator"))))

(calculator)
