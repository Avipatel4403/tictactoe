# tictactoe
# Avi Patel Akp159
# Darius Karneckij dk910

Malicious Messages:
If a malicious message is detected, the player is kicked, the opponent Wins, and the game ends.

Draw:
If a draw is requested, the only valid messages are either accepting/rejecting the draw. RSGN is not valid.

Client:
In the client we used, hardcoded where the piece name of BEGN was in the buffer, thus if the length field of >9, the client will not work. Need to keep names short when testing.
