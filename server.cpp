//server.cpp
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <vector>
#include <map>
#include <fstream>
#include "Packet.h"
using namespace std;

//const int port_num = 2010;
char* Filename;
char* ver;
char* conn;
int status=200;
char* stm;
int newfd;
ifstream infile;

int mapping(int temp){
  int ackNum = (temp -1)/1024;
  return ackNum;
}

int main(int argc, char** argv){

  if(argc!=2){
    cerr<<"Invalid usage. Format: ./server <portnumber>";
    exit(1);
  }

  int port_num = atoi(argv[1]);
  int sockfd, realsocketfd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client = sizeof(client_addr);

  size_t newrtc;


  sockfd = socket(AF_INET, SOCK_DGRAM, 0); //UDP sockets

  if(sockfd < 0) {
   cerr<<"\nError in opening socket";
   exit(1);
  }

  //Getting in all server information:
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Connect to local host
  server_addr.sin_port = htons(port_num);


  //Binding to socket and port:

  int rtc = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if(rtc<0){
    cerr<<"\nError in binding socket and port number";
    exit(1);
  }

  //Waiting for client requests:
  //listen(sockfd,10);
  int16_t pACKnum;
  //Accepting connection requests if SYN is available:
 
  while(1){

    char* dataBuff = (char*)malloc(MAXPACKETSIZE);
    bzero(dataBuff, MAXPACKETSIZE);
    
    newrtc = recvfrom(sockfd, dataBuff, MAXPACKETSIZE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client);
    if(newrtc<0){
      cerr<<"\nError in accepting connection";
      exit(1);
    }

   
    
	   
    //TODO: Check if Filename exists, handle errors!!!
    //
    //

    
    Packet p;
    p.extractPacket(dataBuff, newrtc);
 
    cout<<"Receiving packet SYN"<<endl;
    
    if(p.isSYN()==1){

      char t1[MAXDATALENGTH];
      p.getData(t1);
     
      infile.open(t1, std::fstream::in | std::fstream::binary);
      if(!infile.good())
	{
	  cerr<<"Error 404: File not found in server!"<<endl;
	  shutdown(sockfd, SHUT_RDWR);
	  close(sockfd);
	  exit(1);
	}
      
      //cout<<"Received SYN! Sending SYNACK... ";
      //pACKnum = p.getSeqnum() + 1;
      break;
    }
    else
      continue;
  }
  
  //Responding to the SYN with our SYN ACK packet:
  Packet pACK;
  pACK.setACKnum(0);
  pACK.setSeqnum(0);
  pACK.setACK();
  pACK.setSYN();

  char data[MAXPACKETSIZE+1];
  size_t ret;

  while(1){
      //size_t ret;

    pACK.createPacket(data);
    data[MAXPACKETSIZE]='\0';
    ret = sendto(sockfd, data, sizeof(data), 0,  (struct sockaddr *) &client_addr, client);
    if(ret < 0){
      cerr<<"Error in sending SYNACK";
      exit(1);
    }
    else{
      cout<<"Sending Packet SYNACK"<<endl;
      break;
    }
  }
   
    
    //Sent SYNACK, waiting for ACK from client

  bzero(data, MAXPACKETSIZE);
  ret = recvfrom(sockfd, data, MAXPACKETSIZE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client);
  if(ret < 0){
    cerr<<"Error in receiving ACK for SYNACK";
    exit(1);
  }
  data[MAXPACKETSIZE]='\0';
  
  Packet pAck;
  pAck.extractPacket(data, ret);
  
  if(pAck.getSeqnum() == 0 && pAck.isACK() == 1)
    cout<<"Received Packet ACK"<<endl;
  else
    cerr<<"Invalid ACK received";

  //Can start splitting file into packets now
  //char reader[MAXDATALENGTH];

  vector<Packet> storedata;
  //vector<int> seqNums;

  int readBytes;
  int wnd_start = 1;
  int wnd_end = 5*MAXPACKETSIZE;


  short int startSeq = 1;
  //seqNums.push_back(startSeq);
  char tempBuff[MAXDATALENGTH+1];
  
  if(infile.is_open()){

    while(!infile.eof()){

      bzero(tempBuff, MAXDATALENGTH+1);

      infile.read(tempBuff, MAXDATALENGTH);
      tempBuff[MAXDATALENGTH]='\0';

      
      if(startSeq > 30720)
	startSeq = 1;
      Packet pk;
      pk.setSeqnum(startSeq);
      
      pk.setPacketdata(tempBuff);
      storedata.push_back(pk);
      startSeq+=MAXPACKETSIZE;
    
    }
  }

    
  //Send the packets using Selective Repeat protocols:

  int nextseqNum = 1; //The next available sequence number
  int index = 0;
  char randomBuff[MAXPACKETSIZE+1];
  char packets[MAXPACKETSIZE+1];
  bzero(randomBuff, MAXPACKETSIZE+1);
  bzero(packets, MAXPACKETSIZE+1);
  size_t bt;
  int expectedACKnum = 1;

  int ackSize = storedata.size();

  vector<int> wait(ackSize,0);  //Vector if ackNumbers
  char t3[MAXDATALENGTH+1];
  
  
  while(true && index < storedata.size()){
    
    bool stillinWindow = false;
    if (wnd_start > wnd_end && (nextseqNum >= wnd_start || nextseqNum < wnd_end))
      stillinWindow = true;
    else if (wnd_start < wnd_end && (nextseqNum >= wnd_start && nextseqNum < wnd_end))
      stillinWindow = true;
    while (stillinWindow && index < storedata.size())
      {
	
	bzero(packets, MAXPACKETSIZE+1);
	
	storedata[index].getData(t3);
	storedata[index].createPacket(packets);
	packets[MAXPACKETSIZE] = '\0';
	

	
	if(sendto(sockfd, packets, MAXPACKETSIZE, 0,  (struct sockaddr *) &client_addr, client) <= 0){
	  cerr<<"Error in sending packet!";
	  exit(1);
	}
	cout<<"Sending Packet "<<storedata[index].getSeqnum()<<" 5120"<<endl;
	index++;
	nextseqNum+=MAXPACKETSIZE; // check if sizeof(data[index]) is MAXPACKETSIZE

      }
    
    if(bt = recvfrom(sockfd, randomBuff, MAXPACKETSIZE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client) < 0){
      cout<<"Error in receiving from client!";
      exit(1);
    }
    else {
   
      randomBuff[MAXPACKETSIZE]='\0';
      Packet pp;
      pp.extractPacket(randomBuff,bt);
      cout<<"Receving Packet "<<pp.getACKnum()<<" 5120"<<endl;
      if(pp.isACK()){

	int packetNumber = mapping(pp.getACKnum());
	wait.at(packetNumber) = pp.getACKnum();

	bool isInWindow = false;
	if (wnd_start > wnd_end && (pp.getACKnum() >= wnd_start || pp.getACKnum() < wnd_end))
	  isInWindow = true;
	else if (wnd_start < wnd_end && (pp.getACKnum()>= wnd_start && pp.getACKnum() < wnd_end))
	  isInWindow = true;
	
	if (isInWindow){
	  
	  if(pp.getACKnum() == expectedACKnum){
	    
	    int flag = 0;
	    int i;
	    for(i = packetNumber; i < packetNumber+5; i++){
	      if(wait[i]==0){
		flag=1;
		break;
	      }
	    }

	    
	    //If entire window has been ACKd
	    if(!flag){
	      expectedACKnum = wait[i-1] + MAXPACKETSIZE;
	      wnd_start = expectedACKnum;
	      wnd_end = expectedACKnum + 4*MAXPACKETSIZE;
	    }
	    else{
	      int temp;
	      expectedACKnum = wait[i-1] + MAXPACKETSIZE;

	      if(expectedACKnum > MAXSEQNO)
		expectedACKnum-=MAXSEQNO;
	      wnd_start = expectedACKnum;
	      if((temp = expectedACKnum + 4*MAXPACKETSIZE) > MAXSEQNO)
		wnd_end = temp - MAXSEQNO;
	      else
		wnd_end = temp;
	    }
	  }
	}
      }
      
      else
        cerr<<"Invalid ACK";continue;
    }
    
  }


  //Creating FIN packet after all data has been packetized:
  Packet fin;
  fin.setFIN();
  fin.setSeqnum(0);
  char last[MAXPACKETSIZE+1];
  fin.createPacket(last);
  storedata.push_back(fin);
  last[MAXPACKETSIZE]='\0';
  
  if(ret = sendto(sockfd,last, MAXPACKETSIZE ,0,(struct sockaddr*) &client_addr, client) < 0){
    cout<<"Error in sending FIN packet!";
    exit(1);
  }

  cout<<"Sending Packet "<< fin.getSeqnum()<<" 5120 FIN"<<endl;

  
 
   return 0;
  }

  

   
