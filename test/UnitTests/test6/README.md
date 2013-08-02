The program catches the SIGUSR1 singnal sent by the shell whenever the user
types `kill -10 PID` at the keyboard. 

This test is for correctly handling asynchronous events (e.g., signal handlers).
