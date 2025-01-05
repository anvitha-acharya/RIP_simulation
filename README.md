# RIP_simulation
RIP (Routing Internet Protocol) simulation using NS3

## NS-3 Installation
https://youtu.be/0OKn9e1Gg48?si=z-j3FrIYbOQPsFxJ

## Implementing the codes
1. Open a new terminal
2. Run the following commands to create and write into the file:
   
   `cd ns-allinone-3.XX`  - replace XX with the version number
   
   `cd ns-3.XX`
   
   `cd scratch`
   
   `gedit rip-simple-network.cc`
   
4. Save and close the file. Run the following commands for execution:
   `cd ..` - go back to ns-3.xx directory
   
   `./ns3 run scratch/rip-simple-network.cc`
   
6. For wireshark:
   `ls`
   
   `wireshark rip-simple-routing-DstNode-0.pcap` - replace with the node or router of your choice
   
   On executing the above command, wireshark window pops up. Go to statistics -> I/O Graphs to view the graphs

## Detailed explanation of the concept
https://youtu.be/bCXI1GoCIj4?si=BQbnO_NCaP9cOaQ6

