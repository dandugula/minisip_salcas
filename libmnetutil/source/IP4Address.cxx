/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
*/


#ifdef HAVE_CONFIG_H
#include<config.h>
#endif



#ifdef WIN32
#include"winsock2.h"

// borrowed from tcpdump
int
inet_aton(const char *cp, struct in_addr *addr)
{
  addr->s_addr = inet_addr(cp);
  return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}

#elif defined HAVE_NETINET_IN_H
#include<netdb.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#endif

#include<libmnetutil/IP4Address.h>
#include<libmnetutil/NetworkException.h>

#include<stdio.h>
#include<assert.h>
#include<libmutil/itoa.h>

#ifndef _MSC_VER
#include<strings.h>
#endif

#include<iostream>

#include<exception>
#include<typeinfo>

IP4Address::IP4Address(struct sockaddr_in *sin){
        sockaddress = new sockaddr_in;
	type = IP_ADDRESS_TYPE_V4;
	setAddressFamily(AF_INET);
	setProtocolFamily(PF_INET);
	
	memcpy(sockaddress, sin, sizeof(struct sockaddr_in));
	numIp = ntoh32(sin->sin_addr.s_addr);
} 

uint32_t IP4Address::getBinaryIP(){
	return numIp;
}

IP4Address::IP4Address(string addr){
        sockaddress = new sockaddr_in;
	type = IP_ADDRESS_TYPE_V4;
	ipaddr = addr;

	setAddressFamily(AF_INET);
	setProtocolFamily(PF_INET);
	struct in_addr ip_data;
	if (inet_aton(addr.c_str(),&ip_data)){
		numIp = ntoh32(ip_data.s_addr);
	}else{

		//unsigned char *ip;
		
#ifndef WIN32
		struct hostent *hp= gethostbyname2(ipaddr.c_str(), AF_INET);	
#else
		struct hostent *hp= gethostbyname(ipaddr.c_str());	
//		struct hostent *hp= gethostbyaddr(ipaddr.c_str(), 4, AF_INET);	

		if (WSAGetLastError() != 0) {
			if (WSAGetLastError() == 11001)
				throw new HostNotFound( addr );
		}
#endif

		
		if (!hp){
			throw new HostNotFound( addr );
		}
		
		numIp = ntoh32(*((uint32_t*)(hp->h_addr)));

		assert(hp->h_length==4);
		cerr << *this << endl;
	}

	memset(sockaddress, '\0', sizeof(sockaddress));
	sockaddress->sin_family=AF_INET;
	sockaddress->sin_addr.s_addr = hton32(numIp);
	sockaddress->sin_port=0;
}

IP4Address::IP4Address(const IP4Address& other){
	type = IP_ADDRESS_TYPE_V4;
	setAddressFamily(AF_INET);
	setProtocolFamily(PF_INET);
	ipaddr = other.ipaddr;
	numIp = other.numIp;
        sockaddress = new sockaddr_in;
	memcpy(sockaddress, other.sockaddress, sizeof(struct sockaddr_in));
}

IP4Address::~IP4Address(){
	delete sockaddress;
}

struct sockaddr * IP4Address::getSockaddrptr(int32_t port){
	sockaddress->sin_port=hton16(port);
	return (sockaddr *)sockaddress;
}

int32_t IP4Address::getSockaddrLength(){
	return sizeof(struct sockaddr_in);
}

string IP4Address::getString(){
	return ipaddr;
}

void IP4Address::connect(Socket &socket, int32_t port){

	unsigned char *ip;
	unsigned long int ip_data;
	if (inet_aton(ipaddr.c_str(),(struct in_addr *)&ip_data)){
		ip = (unsigned char *)&ip_data;
	}else{
	
#ifndef WIN32
		struct hostent *hp= gethostbyname2(ipaddr.c_str(), AF_INET);	
#else
		struct hostent *hp= gethostbyname(ipaddr.c_str());	
#endif
		if (!hp){ //throw host not found exception here
			throw new HostNotFound( ipaddr );
		}
		ip = (unsigned char *)hp->h_addr;
		assert(hp->h_length==4);
	}
	
	struct sockaddr_in sin;
	memset(&sin, '\0', sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr, ip, sizeof(ip_data));
	sin.sin_port = htons(port);
	
	int error = ::connect(socket.getFd(), (struct sockaddr *)&sin, sizeof(sin));
	if (error < 0){
		perror("connect");
		socket.close();
		throw new ConnectFailed( error );
	}

}

std::ostream& operator<<(std::ostream& out, IP4Address &a){
	out << a.ipaddr;
	
	unsigned char ip[4];
	//uint32_t beIp = hton32(a.numIp);
	ip[0] = (unsigned char)(a.numIp >> 24);
	ip[1] = (unsigned char)(a.numIp >> 16);
	ip[2] = (unsigned char)(a.numIp >>  8);
	ip[3] = (unsigned char)(a.numIp);
	cerr << " (";

	for (int32_t i=0; i<4; i++){
		if (i>0)
			cerr << ".";

		cerr << (unsigned)ip[i];

	}
	cerr << ")";

	return out;
}

bool IP4Address::operator ==(const IP4Address &i4) const{
	return this->numIp == i4.numIp;
}

bool IP4Address::operator ==(const IPAddress &i) const{

	try{
		const IP4Address &i4 = dynamic_cast<const IP4Address&>(i);
		return (*this == i4);
	}
	catch(std::bad_cast &){
		// Comparing IPv6 and IPv4 addresses
		return false;
	}
}
