//NICK PFISTER contributions

1) Wrote UDP threaded (pthread) receiver and sender framework,
   that switch_sdn and sdn_controller were develop from into their final product

2) Wrote and debugged multi-threaded "switch_sdn"

//SDN client switch
//run command: ./switch_sdn <switchID> <controller hostname> <controller port>
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID>
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -l
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID_0> -l -f <neighbor ID_1>
// -l = log all messages sent and received, usually keep alives (in or out) are not logged
// -f <neighbor ID> = neighbor switch ID who's link is dead, although the switch itself is alive
