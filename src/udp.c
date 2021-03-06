#include "comms.h"
#include "defs.h"

// dialUdp makes an UDP socket with a default remote address 'addr' or fills err
void dialUdp(Conn conn, char* addr, Error err) {
	int sockfd;
    struct addrinfo *result;
	result=solveAddress(addr,err,SOCK_DGRAM,"localhost");
	if(onError(err)) {
		return;
	}
	if(result==NULL || result->ai_addr==NULL) {
  		newError(err,"Unresolved address: %s",addr);
  		return;
  	}
  	sockfd = socket(result->ai_family, result->ai_socktype,result->ai_protocol);
  	if (sockfd<0) {
  		Error e;
  		newError(err,"Socket: %s",ERRDESC(e));
  		return;
	}
	// Otherwise we are connected
	conn->s=sockfd;
	conn->type=SOCKDGRAM_TYPE;
	conn->ver=result->ai_family;
	writeAddress(conn->remote,result->ai_addr);
	freeaddrinfo(result);
}

// connListenMsgs creates a new datagram listener on a network 'net' (like 'udp') and address 'addr'
Conn connListenMsgs(char* net, char* addr, Error err) {
	Conn conn;
	struct addrinfo *ainfo;
 	struct sockaddr* saddr;
	int sockfd;
 	int reuseaddr=1;
 	if(strcmp(net,"udp")!=0) {
 		newError("Unsupported net '%s'\n",net);
 		return NULL;
 	}
 	conn=malloc(sizeof(struct Conn_S));
 	if(conn==NULL) {
 		Error e;
  		newError(err,"Can't allocate %s connection: %s",net,ERRDESC(e));
  		return;
 	}
  	// SOCKET
  	sockfd=socket(PF_INET,SOCK_DGRAM, 0);
  	if(sockfd<0) {
  		Error e;
  		newError(err,"Socket: %s",ERRDESC(e));
  		return;
  	}
  	// BIND
  	ainfo=solveAddress(addr,err,SOCK_DGRAM,"0.0.0.0");
  	if(onError(err)) {
  		return;
  	}
  	if(ainfo==NULL || ainfo->ai_addr==NULL) {
  		newError(err,"Unresolved address: %s",addr);
  		return;
  	}
  	saddr=ainfo->ai_addr;
  	if(bind(sockfd, (struct sockaddr*)saddr, ainfo->ai_addrlen)<0) {
  		Error e;
  		newError(err,"Bind: %s",ERRDESC(e));
  		return;
	}
	conn->type=SOCKDGRAM_TYPE;
	conn->s=sockfd;
	conn->ver=saddr->sa_family;
	freeaddrinfo(ainfo);
	return conn;
}

// connReadFrom reads contents from a remote UDP source
int connReadFrom(Conn conn, Address from, char* buf, int size) {
	if(conn->type==SOCKDGRAM_TYPE) {
		int readed=0;
		struct sockaddr_in6 addrbuf;
		struct sockaddr* saddr=&addrbuf;
		int len=sizeof(addrbuf);
		/*printf("Recv From socket %d on buffer %p size %d from addr %p len %d\n",
			conn->s, buf, size, saddr, len);*/
		readed=recvfrom(conn->s, buf, size, 0, saddr, &len);
		if(readed<0) {
			Error e;
			newError(conn->e,"Recvfrom error %s\n",ERRDESC(e));
		}
		writeAddress(from,saddr);
		return readed;
	} else {
		newError(conn->e,"Unsupported IO type, must be an UDP Datagram Socket!\n");
		return -1;
	}
}

// connWriteTo writes contents of the buf buffer to the given remote UDP address 'to'
int connWriteTo(Conn conn, Address to, char* buf, int size) {
	if(conn->type==SOCKDGRAM_TYPE) {
		int written=0;
		struct addrinfo *ainfo;
		ainfo=solveAddress(to,conn->e,SOCK_DGRAM,"localhost");
	  	if(onError(conn->e)) {
  			return;
	  	}
		if(ainfo==NULL || ainfo->ai_addr==NULL) {
  			newError(conn->e,"Unresolved address: %s",to);
  			return;
	  	}
	  	/*{
	  		Address na;
	  		writeAddress(na,ainfo->ai_addr);
	  		printf("SEND/WRITE socket=%d buf=%p size=%d addr=%s\n",conn->s,buf,size,na);
	  	}*/
		written=sendto(conn->s, buf, size, 0, ainfo->ai_addr, ainfo->ai_addrlen);
		if(written<0) {
			Error e;
			newError(conn->e,"Sendto error %s\n",ERRDESC(e));
		}
		freeaddrinfo(ainfo);
		return written;
	} else {
		newError(conn->e,"Unsupported IO type, must be an UDP Datagram Socket!\n");
		return -1;
	}
}
