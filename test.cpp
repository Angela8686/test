// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



/*********************************************************/
/** Sample program                                      **/
/** This program is a sample program to conduct a       **/
/** connection test between the Ethernet block and      **/
/** target device.                                      **/
/** This program accesses the data register (D) of      **/
/** the Base Module installed together with the         **/
/** Ethernet block.                                     **/
/** Copyright(C) 2009 Mitsubishi Electric               **/
/** Corporation                                         **/
/** All Rights Reserved                                 **/
/*********************************************************/


#include <stdio.h>
#include <winsock.h>
#include <iostream>  
#include <string.h>

using namespace std; 

#define FLAG_OFF 0 // Completion flag OFF
#define FLAG_ON 1 // Completion flag ON
#define SOCK_OK 0 // Normal completion
#define SOCK_NG -1 // Abnormal completion
#define BUF_SIZE 4096 // Receive buffer size
#define ERROR_INITIAL 0 // Initial error
#define ERROR_SOCKET 1 // Socket creation error
#define ERROR_BIND 2 // Bind error
#define ERROR_CONNECT 3 // Connection error
#define ERROR_SEND 4 // Send error
#define ERROR_RECEIVE 5 // Receive error
#define ERROR_SHUTDOWN 6 // Shutdown error
#define ERROR_CLOSE 7 // Line close error
//Definitions for checking the receiving sizes
#define RECV_ANS_1 4 // Receiving size of response message in reply to device write (1E frame)
//#define RECV_ANS_2 24 // Receiving size of response message in reply to device read (1E frame)
#define RECV_ANS_2 4
 struct sck_inf
{
	struct  in_addr		my_addr;
	unsigned short		my_port;
	struct in_addr		FX_IP_addr;
	unsigned short		FX_port;
} ;

int nErrorStatus; // Error information storage variable
int Dmykeyin; // Dummy key input
int Closeflag; // Connection completion flag
SOCKET socketno;
void Sockerror(int); // Error handling function
int main(int argc, char* argv[])
{
	int cnt=0;
    WORD wVersionRequested=MAKEWORD(1,1); // Winsock Ver 1.1 request
	WSADATA wsaData;
	int length; // Communication data length
    char  s_buf[BUF_SIZE]; // Send buffer
//		unsigned char s_buf[BUF_SIZE]; // Send buffer
//	unsigned char r_buf[BUF_SIZE]; // Receive buffer
	 char r_buf[BUF_SIZE]; // Receive buffer
//	 char r_buf_tmp[BUF_SIZE]; // Receive buffer temp
	int rbuf_idx; // Receive data storage head index
	int recv_size; // Number of receive data
	struct sck_inf sc;
	struct sockaddr_in hostdata; // External device side data
	struct sockaddr_in FX3UENET_L; // Ethernet block side data
//	void Sockerror(int); // Error handling function
	unsigned long ulCmdArg ; // Non-blocking mode setting flag

	sc.my_addr.s_addr=htonl(INADDR_ANY); // External device side IP address
	sc.my_port=htons(0); // External device side port number
	sc.FX_IP_addr.s_addr=inet_addr("192.168.1.137"); // Ethernet block side IP address
	// (AC103863h)
	sc.FX_port=htons(1025); // Ethernet block side port number
	Closeflag=FLAG_OFF; // Connection completion flag off

	nErrorStatus=WSAStartup(wVersionRequested,&wsaData); // Winsock Initial processing
	if (nErrorStatus!=SOCK_OK) {
	Sockerror(ERROR_INITIAL); // Error handling
	return (SOCK_NG);
	}
	
printf ("Winsock Version is %ld.%ld\n",HIBYTE(wsaData.wVersion),LOBYTE(wsaData.wVersion));
printf ("FX3U-ENET-L Test Start\n");
socketno=socket(AF_INET,SOCK_STREAM,0); // Create socket for TCP/IP

if (socketno==INVALID_SOCKET){
Sockerror (ERROR_SOCKET); // Error handling
return(SOCK_NG);
}

hostdata.sin_family=AF_INET;
hostdata.sin_port=sc.my_port;
hostdata.sin_addr.s_addr=sc.my_addr.s_addr;

if(bind(socketno,(LPSOCKADDR)&hostdata,sizeof(hostdata))!=SOCK_OK){
// Bind
Sockerror(ERROR_BIND); // Error handling
return(SOCK_NG);
}

FX3UENET_L.sin_family=AF_INET;
FX3UENET_L.sin_port=sc.FX_port;
FX3UENET_L.sin_addr.s_addr=sc.FX_IP_addr.s_addr;
if(connect(socketno,(LPSOCKADDR)&FX3UENET_L,sizeof(FX3UENET_L))!=SOCK_OK){
// Connection (Active open)
Sockerror(ERROR_CONNECT); // Error handling
return(SOCK_NG);
}
 cnt =0;
while(cnt < 6)
{
Closeflag=FLAG_ON; // Connection completion flag ON
// Go to non-blocking mode
ulCmdArg = 1;
ioctlsocket(socketno, FIONBIO, &ulCmdArg); // Set to non-blocking mode
//strcpy(s_buf, "03FF000A4420000000000500112233445566778899AA");
strcpy(s_buf, "00FF000A5920000000000300");
// D0 to D4 batch write request (1E frame)
length=(int)strlen(s_buf);

	if(send(socketno,s_buf,length,0)==SOCKET_ERROR){ // Data sending
	Sockerror(ERROR_SEND); // Error handling
	return (SOCK_NG);
	}
printf("\n send data\n%s\n",s_buf);
// Perform receiving size check and receiving processing simultaneously
rbuf_idx = 0; // Receive data storage head index initialization
recv_size = 0; // Initialize the number of receive data

while(1) {
	length = recv(socketno, &r_buf[rbuf_idx], (BUF_SIZE - rbuf_idx), 0);
	// Response data receiving
	if(length == 0) { // Is connection cut off?
	Sockerror(ERROR_RECEIVE); // Error handling
	return (SOCK_NG);
	}
	
	if(length == SOCKET_ERROR) {
		nErrorStatus = WSAGetLastError();

		if(nErrorStatus != WSAEWOULDBLOCK)
		{
			Sockerror(ERROR_RECEIVE); // Error handling
			return (SOCK_NG);
		} 
		else
		{
			continue; // Repeat until messages are received
		}
	} 
	else
	{
		rbuf_idx += length; // Update the receive data storage
		// position
		recv_size += length; // Update the number of receive data
		if(recv_size >= RECV_ANS_1) // Have all response messages been
		// received?
        	
		break; // Stop repeating as messages have
		// been received
	}

}  // end while

r_buf[rbuf_idx] = '\0' ; // Set NULL at the end of receive data
printf("\n receive data\n%s\n",r_buf);
//strcpy(s_buf, "01FF000A4420000000000500"); // D0 to D4 batch read request
strcpy(s_buf, "00FF000A5920000000000300"); // D0 to D4 batch read request
// (1E frame)
length=(int)strlen(s_buf);

if(send(socketno,s_buf,length,0)==SOCKET_ERROR)
{ // Data sending
	Sockerror(ERROR_SEND); // Error handling
	return (SOCK_NG);
}
printf("\n send data\n%s\n",s_buf);
// Perform receiving size check and receiving processing simultaneously
rbuf_idx = 0; // Receive data storage head index
// initialization
recv_size = 0; // Initialize the number of receive data
  

while(1)
 {
	length = recv(socketno, &r_buf[rbuf_idx], (BUF_SIZE - rbuf_idx), 0);
	// Response data receiving
	if(length == 0) { // Is connection cut off?
		Sockerror(ERROR_RECEIVE); // Error handling
		return (SOCK_NG);
	}
	if(length == SOCKET_ERROR) 
	{
		nErrorStatus = WSAGetLastError();
		if(nErrorStatus != WSAEWOULDBLOCK) {
			Sockerror(ERROR_RECEIVE); // Error handling
			return (SOCK_NG);
		}
		else
		{
			continue; // Repeat until messages are received
		}
	}
	else
	{
		rbuf_idx += length; // Update the receive data storage
		// position
		recv_size += length; // Update the number of receive data

		if(recv_size >= RECV_ANS_2) // Have all response messages been
			// received?
			break; // Stop repeating as messages have
			// been received
	}
}  // end while
r_buf[rbuf_idx] = '\0' ; // Set NULL at the end of receive data
printf("\receive data\n%s\n", r_buf);


   cnt += 1;
   Sleep(5000);
   }


if(shutdown(socketno,2)!=SOCK_OK)
{
	// Processing to disable
	// sending/receiving
	Sockerror(ERROR_SHUTDOWN); // Error handling
	return(SOCK_NG);
}

if(closesocket(socketno)!=SOCK_OK)
{ // Close processing
	Sockerror(ERROR_CLOSE); // Error handling
	return(SOCK_NG);
}

Closeflag=FLAG_OFF; // Connection completion flag off
WSACleanup(); // Release Winsock.DLL
printf("\nFX3U-ENET-L Test End.\n\n Normally completed. \n");
printf("Press any key to exit the program.\n");
Dmykeyin=getchar(); // Wait for key input
return(SOCK_OK);

}


void Sockerror(int error_kind) // Error handling function
{
	if(error_kind==ERROR_INITIAL){
	printf("Initial processing is abnormal.");
	}
	else
	{
		nErrorStatus=WSAGetLastError();
		switch(error_kind)
		{
			case ERROR_SOCKET:
				printf("Failed to create socket.");
				break;
 
			case ERROR_BIND:
				printf("Failed to bind.");
				break;
			case ERROR_CONNECT:
				printf("Failed to establish connection.");
				break;
			case ERROR_SEND:
				printf("Sending failed.");
				break;
			case ERROR_RECEIVE:
				printf("Receiving failed.");
				break;
			case ERROR_SHUTDOWN:
				printf("Failed to shutdown.");
				break;
			case ERROR_CLOSE:
				printf("Failed to close normally.");
				break;
		}  // end switch
	}  // end if
	printf("Error code is %d.\n", nErrorStatus);
	if(Closeflag==FLAG_ON)
	{
		nErrorStatus=shutdown(socketno,2); // Shutdown processing
		nErrorStatus=closesocket(socketno); // Close processing
		Closeflag=FLAG_OFF; // Connection completion flag off
	}
	printf("Press any key to exit the program.\n");
	Dmykeyin=getchar(); // Wait for a key input
	WSACleanup(); // Release Winsock.DLL
	return;
}