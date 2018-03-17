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
    cerr<<"Invalid usage. Format: ./client <hostname> <port_num> <Filename>";
    //    close(sockfd);
    exit(1);
  }


  Filename = argv[3];
  //const char* sp = " ";

  //strcat(Filename, sp);
 
  
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

  char synp[MAXPACKETSIZE+1];
  char synack[MAXPACKETSIZE+1];
  p.createFirstPacket(Filename, synp);
  
  int rtc;
  rtc = sendto(sockfd, synp , sizeof(synp), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if(rtc == -1){
    cerr<<"Error sending SYN\n";
    exit(1);
  }

  cout<<"Sending Packet SYN"<<endl;

  //Sending ACK for SYNACK.

  socklen_t servr = sizeof(server_addr);

  rtc = recvfrom(sockfd, synack, MAXPACKETSIZE, 0, (struct sockaddr*)&server_addr, &servr);
  if(rtc == -1){
    cerr<<"Error receiveing SYNACK\n";
    exit(1);
  }

  pAck.extractPacket(synack, rtc);

  if(pAck.getSeqnum() == 0 && pAck.isSYN() == 1 && pAck.isACK() == 1)
    cout<<"Received Packet SYNACK"<<endl;

  Packet final;
  bzero(synp, MAXPACKETSIZE);

  final.setACKnum(0);
  final.setSeqnum(0);
  final.setACK();
  
  final.createPacket(synp);

  rtc = sendto(sockfd, synp, sizeof(synp), 0,  (struct sockaddr *) &server_addr, sizeof(server_addr));
  
  if(rtc < 0){
    cerr<<"Error in sending ACK\n";
    exit(1);
  }
  else
    {
      cout<<"Sending Packet ACK"<<endl;
    }
  
  ofstream out;
  out.open("received.data", fstream::out | fstream::trunc); //change to original filename

  Packet* buffer[] = {NULL, NULL, NULL, NULL, NULL};
  int head=0;
  int ctr=0;
  int bwnd = 1, ewnd = 4097;
  Packet in;
  char recBuff[MAXPACKETSIZE+1];
  
  //out.flush();
  while(1){
    
    //bzero(synp, MAXPACKETSIZE);

    
    rtc = recvfrom(sockfd, recBuff, MAXPACKETSIZE, 0, (struct sockaddr*)&server_addr, &servr);
    
    recBuff[MAXPACKETSIZE] = '\0';
    //cout << "Is this right: " << recBuff << endl;
    in.extractPacket(recBuff, rtc);

  

    char t3[MAXDATALENGTH+1];
    in.getData(t3);
    t3[MAXDATALENGTH]='\0';


    if(in.isFIN()==1)
      {

	//TODO: implement TIME_WAIT
	break;
      }

      cout<<"Receiving Packet "<<in.getSeqnum()<<endl;

    bool isInWindow = false;

    if(ewnd < bwnd && (in.getSeqnum() <= ewnd || in.getSeqnum() >= bwnd))
      isInWindow = true;
    else if (in.getSeqnum() >= bwnd && in.getSeqnum() <= ewnd)
      isInWindow = true;

    if (isInWindow){
	
      int pno = ((in.getSeqnum()-1)/MAXPACKETSIZE);
      buffer[pno % 5] = &in;

    
      
      //Check what to write
      //out.flush();
      while(buffer[head]!=NULL)
	{
	  char t2[MAXDATALENGTH+1];
	  
	  (*buffer[head]).getData(t2);
	  t2[MAXDATALENGTH]='\0';
	  out<<t2;
	  out.flush();

	  ctr++;
	  buffer[head]=NULL;
	  head++;
	  if(head == 5)
	    head=0;
	}

      bwnd+=ctr*1024;
      ewnd+=ctr*1024;
      
      if (bwnd > MAXSEQNO)
	bwnd = bwnd - MAXSEQNO;
      if (ewnd > MAXSEQNO)
	ewnd = ewnd - MAXSEQNO;
      
      ctr=0;
    }
    
    //Send ACK
      
    Packet pack;
    bzero(synp, MAXPACKETSIZE+1);
    
    pack.setACK();
    pack.setACKnum(in.getSeqnum());
    
    pack.createPacket(synp);
    
    rtc = sendto(sockfd, synp, MAXPACKETSIZE, 0,  (struct sockaddr *) &server_addr, sizeof(server_addr));

    cout<<"Sending Packet "<<pack.getACKnum()<<endl; 
    
    //ACK SENT
  }    
  
  out.close();
  close(sockfd);
  return 0;
}
