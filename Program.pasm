	; Test string
	DS 555 "Hello world!\nPress any key to exit...\n\0"

	; Start of program
	MOVE r0 #555
	LOAD r1 r0
	COND r1
	; Check the following address when adding instructions above cuz we have no labels :(
	JUMP 7
	WRITE r1
	ADD r0 r0 #1
	JUMP 1
	READ r1
	BREAK