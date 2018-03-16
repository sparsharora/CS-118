  //packet.h
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

#define MAXSEQNO 30720
#define SSTHRESH 30720
#define MAXPACKETSIZE 1024
#define MAXDATALENGTH 1014
#define BUFSIZE 1032
#define MAXWINDOWSIZE 5120
using namespace std;

// Variables needed for basic TCP like header:
class Header {
public:
  int16_t seqnum;
  int16_t acknum;
  int16_t SYN;
  int16_t FIN;
  int16_t ACK;
  /*Header size = 7 bytes
  bool SYN = FALSE;
  bool ACK = FALSE;
  bool  = FALSE;*/
};

class Packet {
public:
  Packet();
  int getACKnum();
  int getSeqnum() const;
  void setACKnum(int16_t ACK);
  void setSeqnum(int16_t seq);
  bool isSYN();
  bool isFIN();
  bool isACK();
  void setSYN();
  void setACK();
  void setFIN();
  Header tcpHeader();
  char* getData(); 
  //void setData(char* ch);    //Make this function with a dynamic function signature
  void extractPacket(char* c, size_t bytes);
  void createPacket(char* store);
  void setPacketdata(char *ch);
  void createFirstPacket(const char* filename, char* store);

private:
  char packetData[MAXDATALENGTH];    //To store the data
  Header header;
};


Packet::Packet(){
  strcpy(packetData, " ");
  header.seqnum=0;
  header.acknum=0;
  header.SYN=0;
  header.ACK=0;
  header.FIN=0;
}


char* Packet::getData()
{
  return this->packetData;
}
void Packet::setPacketdata(char* ch){
  strcpy(this->packetData, ch);
}

void Packet::createFirstPacket(const char* filename, char* store){

  char temp[sizeof(int)];
  bzero(store, MAXPACKETSIZE);

 
  //memcpy(store, &header.seqnum, sizeof(int));
  sprintf(temp,"%d",header.seqnum);
  strcat(store,temp);
  strcat(store, ",");

  sprintf(temp,"%d",header.acknum);
  strcat(store,temp);
  strcat(store, ",");
  
  sprintf(temp,"%d",header.SYN);
  strcat(store,temp);
  strcat(store, ",");

  sprintf(temp,"%d",header.ACK);
  strcat(store,temp);
  strcat(store, ",");
  
  /* memcpy(store+2, &header.acknum, 2);
  memcpy(store+3, &header.SYN, 1);
  memcpy(store+4, &header.FIN, 1);
  memcpy(store+5, &header.ACK, 1);*/
  
  strcat(store, (const char*)filename);
  //cout<<endl<<store<<endl;
  //return store;
}
void Packet::createPacket(char* store){

  char temp[sizeof(int)];
  bzero(store, MAXPACKETSIZE);

  //memcpy(store, &header.seqnum, sizeof(int));
  sprintf(temp,"%d",header.seqnum);
  strcat(store,temp);
  strcat(store, ",");

  sprintf(temp,"%d",header.acknum);
  strcat(store,temp);
  strcat(store, ",");

  sprintf(temp,"%d",header.SYN);
  strcat(store,temp);
  strcat(store, ",");

  sprintf(temp,"%d",header.ACK);
  strcat(store,temp);
  strcat(store, ",");

  strcat(store, packetData);
  strcat(store,"\0");

}

void Packet::extractPacket(char* dataBuff, size_t bytes){

  char *temp;
 
  //cout<<"DATA: "<<dataBuff<<endl;
  
  temp = strtok(dataBuff, ",");
  header.acknum = atoi(temp);


  temp = strtok(NULL, ",");
  header.seqnum = atoi(temp);

  temp = strtok(NULL, ",");
  header.SYN = atoi(temp);

  temp = strtok(NULL, ",");
  header.ACK = atoi(temp);

  temp = strtok(NULL, ",");
  strcpy(packetData,temp);
 
 }

int Packet::getACKnum(){
  return this->header.acknum;
}

int Packet::getSeqnum() const{
  return this->header.seqnum;
}

bool Packet::isSYN(){
  return (this->header.SYN==1);
}

bool Packet::isFIN(){
  return (this->header.FIN==1);
}

bool Packet::isACK(){
  return (this->header.ACK==1);
}

void Packet::setSeqnum(int16_t seq){
  this->header.seqnum = seq;
}

void Packet::setACKnum(int16_t ACK){
  this->header.acknum = ACK;
}

void Packet::setACK(){
  this->header.ACK = 1;
}

void Packet::setSYN(){
  this->header.SYN = 1;
}

void Packet::setFIN(){
  this->header.FIN = 1;
}

Header Packet::tcpHeader(){
  return this->header;
}
