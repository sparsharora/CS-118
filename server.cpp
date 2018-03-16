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
 
   
    
    if(p.isSYN()==1){

      char t1[MAXDATALENGTH];
      p.getData(t1);
     
      infile.open(t1);
      
      cout<<"Received SYN! Sending SYNACK... ";
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

  char data[MAXPACKETSIZE];
  size_t ret;

  while(1){
    //Turn the side bitch to a nine piece
    //size_t ret;

    pACK.createPacket(data);
    //cout<<"Data: "<<data<<endl;
    ret = sendto(sockfd, data, sizeof(data), 0,  (struct sockaddr *) &client_addr, client);
    if(ret < 0){
      cerr<<"Error in sending SYNACK";
      exit(1);
    }
    else{
      cout<<"Sent SYNACK, waiting for ACK...\n";
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
  
  Packet pAck;
  pAck.extractPacket(data, ret);
  
  if(pAck.getSeqnum() == 0 && pAck.isACK() == 1)
    cout<<"Recieved Final Ack. Handshake Complete!...\n";
  else
    cout<<"Invalid ACK received";

  //Can start splitting file into packets now
  //char reader[MAXDATALENGTH];

  vector<Packet> storedata;
  //vector<int> seqNums;

  int readBytes;
  int wnd_start = 1;
  int wnd_end = 5*MAXPACKETSIZE;


  int startSeq = 1;
  //seqNums.push_back(startSeq);
  char tempBuff[MAXDATALENGTH];

  if(infile.is_open()){

    while(!infile.eof()){

      infile >> tempBuff;
    
      if(startSeq > 30720)
	startSeq = 1;
      
      Packet pk;
      pk.setSeqnum(startSeq);
      cout<<"Fseq: "<<startSeq<<endl;
      pk.setPacketdata(tempBuff);
      storedata.push_back(pk);
      startSeq+=MAXPACKETSIZE;
    
    }
  }

  cout<<"exit\n";
    
  //Send the packets using Selective Repeat protocols:

  int nextseqNum = 1; //The next available sequence number
  int index = 0;
  char randomBuff[MAXPACKETSIZE];
  char packets[MAXPACKETSIZE];
  bzero(randomBuff, MAXPACKETSIZE);
  bzero(packets, MAXPACKETSIZE);
  size_t bt;
  int expectedACKnum = 1;

  while(true){

    while(nextseqNum >= wnd_start && nextseqNum < wnd_end){   //TODO : Chek when seq number changes!!!!
     
      storedata[index].createPacket(packets);
      cout<<"Seq: "<<storedata[index].getSeqnum()<<endl;
      if(sendto(sockfd, packets, sizeof(data), 0,  (struct sockaddr *) &client_addr, client) <= 0){
        cerr<<"Error in sending packet!";
        exit(1);
      }
      index++;
      nextseqNum+=sizeof(data[index]); // check if sizeof(data[index]) is MAXPACKETSIZE
    }

    bt = recvfrom(sockfd, randomBuff, MAXPACKETSIZE, 0, (struct sockaddr*)&client_addr, (socklen_t*)&client);

    if(bt < 0){
      cerr<<"Error in receiving from client!";
      exit(1);
    }
    else {
      Packet pp;
      pp.extractPacket(randomBuff,bt);
      if(pp.isACK()){
        if(pp.getACKnum() == expectedACKnum){
          continue;
	}
      }
      else
        cerr<<"Invalid ACK";
    }

  }


  //Creating FIN packet after all data has been packetized:
  Packet fin;
  fin.setFIN();
  fin.setSeqnum(0);
  storedata.push_back(fin);
  
  
  /*
  cerr<<"Received ACK for SYNACK";
    Packet finalACK;
    finalACK.extractPacket(newBuff, rtt);
    if(finalACK.isACK()  && finalACK.getACKnum()==0){
    //Handshake is complete now
    cerr<<"Handshake complete";break;
    }
    else
      cerr<<"Unexpected packet received";
  }
  //Can start splitting file into packets now
  
/*
	//creating seperate process to keep connection going
	pID = fork();
	if(pID<0)
	  fprintf(stderr,"\nError in creating process");
	if(pID==0){    //child process
	  close(sockfd);
	  memset(message, 0, 1024);
	  rtc = read(new, message, 1024);
	  if(rtc<0)
	    fprintf(stderr,"\nError reading from socket");
	  fprintf(stdout,"\n%s", message);       //TO DO: close socket!!!
	  parse(message);
	  exit(0);
	}
	else
	  close(new);
  */
   return 0;
  }

  

   
