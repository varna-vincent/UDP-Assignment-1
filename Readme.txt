Compiling Instructions:-

Compile each file on seperate terminals
gcc Server.c -o ./s.out
gcc Client.c -o ./c.out

Run Server
./s.out

Run Client
./c.out

------------------------------------------------------------------------------

When server is run, if user presses 0, server will respond immediately to packets received from client

When server is run, if user presses 1, server will not respond to first packet received from client, client will retransmit the packet and server will respond with ACK

------------------------------------------------------------------------------

When client is run, if user presses 0, 5 correct packets will be sent to server

When client is run, if user presses 1, 1 correct packet and 4 wrong packets are sent to server, server responds with ACK and the corresponding REJECT packets