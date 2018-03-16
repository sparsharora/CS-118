//server.cpp
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <iostream>
#include "Packet.h"
#include <fstream>
#include <vector>

using namespace std;


//const int port_num = 2010;
const char* Filename;
char* ver;
char* conn;
int status=200;
char* stm;
int newfd;

int main(int argc, char** argv){


  //TODO: Check for other errors in arguments!!!!
  
  if(argc!=4){
    cerr<<"Invalid usage. Format: ./server <portnumber>";
    exit(1);
  }


  Filename = argv[3];
 
  
  int port_num = atoi(argv[2]);
 
  int sockfd, realsocketfd;
  struct sockaddr_in server_addr;
  
  int pID, newrtc;
  
  
  sockfd = socket(AF_INET, SOCK_DGRAM, 0); //UDP sockets
  
  if(sockfd < 0) {
    cerr<<"\nError in opening socket";
    exit(1);
  }

 

  
  //Getting in all server information:
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Connect to local host
  server_addr.sin_port = htons(port_num);
  
 
  // Make initial request packet
  
  Packet p, pAck;
  p.setSeqnum(0);
  p.setACKnum(0);
  p.setSYN();

  char synp[MAXPACKETSIZE];
  char synack[MAXPACKETSIZE];
  p.createFirstPacket(Filename, synp);
  
  int rtc;
  rtc = sendto(sockfd, synp , sizeof(synp), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if(rtc == -1){
    cerr<<"Error sending SYN\n";
    exit(1);
  }

  cout<<"Sent Filename! Waiting for File....\n";

  //Sending ACK for SYNACK.

  socklen_t servr = sizeof(server_addr);

  rtc = recvfrom(sockfd, synack, MAXPACKETSIZE, 0, (struct sockaddr*)&server_addr, &servr);
  if(rtc == -1){
    cerr<<"Error receiveing SYNACK\n";
    exit(1);
  }

  pAck.extractPacket(synack, rtc);

  if(pAck.getSeqnum() == 0 && pAck.isSYN() == 1 && pAck.isACK() == 1)
    cout<<"Recieved SYNACK. Completing Handshake...\n";

  Packet final;
  bzero(synp, MAXPACKETSIZE);

  final.setACKnum(0);
  final.setSeqnum(0);
  final.setACK();
  
  final.createPacket(synp);
  //cout<<"Data: "<<data<<endl;
  rtc = sendto(sockfd, synp, sizeof(synp), 0,  (struct sockaddr *) &server_addr, sizeof(server_addr));
  if(rtc < 0){
    cerr<<"Error in sending ACK";
    exit(1);
  }
  else{
    cout<<"Sent FinalACK, Handshake complete.\n";
  }


  ofstream out;
  out.open(Filename);

  Packet* buffer[] = {NULL, NULL, NULL, NULL, NULL};
  int head=0;
  int ctr=0;
  int bwnd = 1, ewnd = 4097;

  while(1){
    
    bzero(synp, MAXPACKETSIZE);
    
    rtc = recvfrom(sockfd, synp, MAXPACKETSIZE, 0, (struct sockaddr*)&server_addr, &servr);

    Packet in;
    in.extractPacket(synp, rtc);

    if(in.isFIN()==1)
      {
	//TODO: implement TIME_WAIT
	break;
      }

    if(in.getSeqnum() >= bwnd && in.getSeqnum() <= ewnd){

      int pno = ((in.getSeqnum()-1)/MAXPACKETSIZE);
      buffer[pno % 5] = &in;

      //Check what to write
      while(buffer[head]!=NULL)
	{
	  out<<in.getData();
	  ctr++;
	  buffer[head]=NULL;
	  head++;
	}

      bwnd+=ctr*1024;
      ewnd+=ctr*1024;
      ctr=0;

    }
      
      //Send ACK
      
      Packet pack;
      bzero(synp, MAXPACKETSIZE);
      
      final.setACK();
      final.setSeqnum(in.getSeqnum());
      
      final.createPacket(synp);
      
      rtc = sendto(sockfd, synp, sizeof(synp), 0,  (struct sockaddr *) &server_addr, sizeof(server_addr));
      
      //ACK SENT
  
      
  }
    

	



  
  return 0;
}
