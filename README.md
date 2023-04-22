# tictactoe
# Avi Patel Akp159
# Darius Karneckij dk910

Decisions:
Player Name: Maximum of 25 characters
notes(Simplied):
1:
PLAY X FIRST
End Conditions:
Win
Tie(check if the board is filled)
A player quits
both players agree on draw

2:
ttt:
Arguments: domain name and port number of the desired service

connect to the service if ttts exists
display board
get moves, transmit move, and communicate the move to the other player

ttts:
Arguments: port number it will use for connection requests

pair players
choose who goes first
receive commands
track state of grid
ensure moves are valid
check if player won

3:Protocol

3.1:Message Format

3.2:Message By Client
PLAY - Sent to create connection
MOVE: Player Move
RSGN: Player has Resigned
DRAW: Suggests a draw to be sent the to other player

3.3:Messages by server
WAIT:in reponse to PLAY
BEGN: Play is ready to begin, assigns X or O
MOVD:Show that a move has been created
INVL: message is invalid
DRAW: Sends message of other player wanting to draw
OVER: Win Loss Draw

3.4:Game Setup

4