/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien
  Copyright (C) 2007 Mikael Magnusson
  
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
 *	    Cesc Santasusana < cesc Dot santa at@ gmail dOT com>
 *          Mikael Magnusson <mikma@users.sourceforge.net>
*/


#include<config.h>

#include<libmsip/SipDialogConfig.h>
#include<libmnetutil/NetworkException.h>
#include<libmnetutil/NetworkFunctions.h>
#include<libmutil/massert.h>

using namespace std;

#define DEBUG_OUTPUT

int SipIdentity::globalIndex = 1; //give an initial value

SipCredential::SipCredential( const std::string &username,
			      const std::string &password,
			      const std::string &realm ):
		realm( realm ),
		username( username ),
		password( password )
{
}

const std::string &SipCredential::getRealm() const{
	return realm;
}

const std::string &SipCredential::getUsername() const{
	return username;
}

const std::string &SipCredential::getPassword() const{
	return password;
}

void SipCredential::set( const std::string &aUsername,
			 const std::string &aPassword,
			 const std::string &aRealm ){
	realm = aRealm;
	username = aUsername;
	password = aPassword;
}



SipRegistrar::SipRegistrar(): uri(){
	autodetectSettings = false; //dont autodetect ... the values are invalid
	registerExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
	defaultExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
}

SipRegistrar::SipRegistrar(const SipUri &addr, int port){
	autodetectSettings = false;
	try {
		registerExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
		defaultExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
		setRegistrar( addr, port );
	} catch (NetworkException & ) {
		#ifdef DEBUG_OUTPUT
		cerr << "SipRegistrar(str, int) throwing ... " << endl;
		#endif
		throw HostNotFound( "[SipRegistrar " + addr.getString() + "]" );
	}
}

SipRegistrar::SipRegistrar(const SipUri &userUri, string transportParam) {
	SipUri addr;
//	bool unknown = true; //unused
	autodetectSettings = true;
	
	registerExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
	defaultExpires=DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;

	addr = userUri;
	if( transportParam != "" )
		addr.setTransport( transportParam );

	setRegistrar( addr );
}

//addr could be "IP:port" ... but the port param passed to the function has precedence ...
void SipRegistrar::setRegistrar(const SipUri &addr, int port){
	massert(addr.getUserName()=="");
	if( port > 65535 || port < 0 ) port = -1; //check the port
	
	#ifdef DEBUG_OUTPUT
	cerr << "SipRegistrar:setProxy(str) : addr = " << addr << endl;
	#endif

	uri = addr;

	if( port != -1 ){
		uri.setPort(port);
	}
}

std::string SipRegistrar::getDebugString(){
	return "uri="+uri.getString()
		+"; autodetect="+ (autodetectSettings?"yes":"no")
// 		+"; user="+sipProxyUsername
// 		+"; password="+sipProxyPassword
		+"; expires="+itoa(defaultExpires);
}

int SipRegistrar::getRegisterExpires_int( ) {
	return registerExpires;
}

int SipRegistrar::getDefaultExpires_int( ) {
	return defaultExpires;
}

void SipRegistrar::setRegisterExpires( string _expires ) {
	int r;
	r = atoi( _expires.c_str() );
	setRegisterExpires( r );
}

void SipRegistrar::setRegisterExpires( int _expires ) {
	if( _expires >= 0 && _expires < 100000 ) //sanity check ...
		registerExpires = _expires;
	else registerExpires = DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
}

string SipRegistrar::getRegisterExpires( ) {
	return itoa(registerExpires); 
}

void SipRegistrar::setDefaultExpires( string _expires ) {
	int r;
	r = atoi( _expires.c_str() );
	setDefaultExpires( r );
}
void SipRegistrar::setDefaultExpires( int _expires ) {
	if( _expires >= 0 && _expires < 100000 ) //sanity check ...
		defaultExpires = _expires;
	else defaultExpires = DEFAULT_SIPPROXY_EXPIRES_VALUE_SECONDS;
}
string SipRegistrar::getDefaultExpires( ) {
	return itoa(defaultExpires); 
}


void SipIdentity::init(){
	/*sipProxyPort=0; sipProxyIpAddr=NULL;*/ 
	registerToProxy=false; 
	securityEnabled=false;
	ka_type=0;
	//use_srtp=false;
	use_zrtp=false;
	pskEnabled=false;
	dhEnabled=false;
	checkCert=false;

	identityIdx = itoa( globalIndex );
	globalIndex ++;
#ifdef DEBUG_OUTPUT	
	cerr << "SipIdentity::SipIdentity : created identity id=" << identityIdx << endl;
#endif
	setIsRegistered (false);
}

SipIdentity::SipIdentity(){
	init();
}

SipIdentity::SipIdentity(const SipUri &addr) : sipUri(addr){
	init();
}

void SipIdentity::setDoRegister(bool f){
	lock();
	registerToProxy=f;
	unlock();
}

bool SipIdentity::getDoRegister(){
	lock();
	bool ret = registerToProxy;
	unlock();
	return ret;
}

void SipIdentity::lock(){
	mutex.lock();
}

void SipIdentity::unlock(){
	mutex.unlock();
}

std::string SipIdentity::getId() {
	lock();
	std::string ret = identityIdx;
	unlock();
	return ret;
}

bool SipIdentity::isRegistered(){
	lock();
	bool ret = currentlyRegistered;
	unlock();
	return ret;
}



#if 0
void SipIdentity::setSipUri(string addr){
	if (addr.substr(0,4)=="sip:")
		addr = addr.substr(4);
	if (addr.find("@")==string::npos){
			#ifdef DEBUG_OUTPUT	
			cerr << "WARNING: Incomplete sip address: "<< addr<<endl;
			#endif
		sipUsername = addr.substr(0, addr.find("@"));
		sipDomain = "";
	} else {
		sipUsername = addr.substr(0, addr.find("@"));
		sipDomain = addr.substr(addr.find("@")+1);
	}

#ifdef DEBUG_OUTPUT	
	cerr << "SipIdentity::setSipUri: sipUsername=<"<< sipUsername << "> sipDomain=<" << sipDomain << ">"<< endl;
#endif
}

string SipIdentity::getSipUri() {
	string ret;
	lock();
	if( sipUsername != "" && sipDomain !="" ) {
		ret = sipUsername + "@" + sipDomain;
	} else {
		//one of the two is empty, so do not add the @ ... 
		ret = sipUsername + sipDomain;
	}
	unlock();
	return ret;
}
#endif

MRef<SipRegistrar *> SipIdentity::getSipRegistrar() {
	return sipProxy;
}

bool SipIdentity::setSipRegistrar( MRef<SipRegistrar *> proxy ){
	sipProxy = proxy;
	return true;
}

string SipIdentity::setSipProxy( bool autodetect, string userUri, string transport, string proxyAddr, int proxyPort ) {
	string ret = "";
	routeSet.clear();
	
	#ifdef DEBUG_OUTPUT
	if( autodetect ) cerr << "SipIdentity::setSipProxy: autodetect is true";
	else 		cerr << "SipIdentity::setSipProxy: autodetect is false";
	cerr << "; userUri=" << userUri << "; transport = "<< transport << "; proxyAddr=" << proxyAddr << "; proxyPort=" << proxyPort << endl;
	#endif	

	if( autodetect ){
		SipUri proxyUri;
		SipUri aor( userUri );

		proxyUri.setProtocolId( aor.getProtocolId() );
		proxyUri.setIp( aor.getIp() );
		if( transport != "" )
			proxyUri.setTransport( transport );
		proxyUri.setParameter( "lr", "true" );
		proxyUri.makeValid( true );

		routeSet.push_back( proxyUri );
	}
	else if( proxyAddr != "" ){
		SipUri proxyUri;
		
		proxyUri.setProtocolId( "sip" );
		proxyUri.setIp( proxyAddr );
		proxyUri.setPort( proxyPort );
		if( transport != "" )
			proxyUri.setTransport( transport );
		proxyUri.setParameter( "lr", "true" );
		proxyUri.makeValid( true );

		routeSet.push_back( proxyUri );
	}

	return ret;
}

MRef<SipCredential*> SipIdentity::getCredential() const{
	return credential;
}

void SipIdentity::setCredential( MRef<SipCredential*> aCredential ){
	credential = aCredential;
}

const list<SipUri> &SipIdentity::getRouteSet() const {
	return routeSet;
}

void SipIdentity::setRouteSet( const list<SipUri> &aRouteSet ){
	routeSet = aRouteSet;
}

void SipIdentity::addRoute( const SipUri &route ){
	routeSet.push_back( route );
}


void SipIdentity::setIdentityName(string n){
	identityIdentifier = n;
}

void SipIdentity::setIsRegistered( bool registerOk ) {
	if( registerOk == true && getSipRegistrar()->getRegisterExpires_int() != 0 ) {
		currentlyRegistered = true;
	} else {
		currentlyRegistered = false;
	}
}

void SipIdentity::setRegisteredContacts( const list<SipUri> &contacts ){
	registeredContacts = contacts;
}

const list<SipUri>& SipIdentity::getRegisteredContacts() const{
	return registeredContacts;
}

string SipIdentity::getDebugString(){
	lock();
	string ret = "identity="+identityIdx+
			"; uri="+sipUri.getString()+ 
			" proxy=["+(getSipRegistrar()?getSipRegistrar()->getDebugString():"")+
			"]; isRegistered="+itoa(currentlyRegistered);
	unlock();
	return ret;
}

void SipIdentity::setPsk( string key ){
	psk=key;
}

SipStackConfig::SipStackConfig():
	externalContactUdpPort(0),
	localUdpPort(0),
	localTcpPort(0),
	localTlsPort(0),
	autoAnswer(false),
	use100Rel(false){

}

int32_t SipStackConfig::getLocalSipPort(bool usesStun, const string &transport ) {
	int32_t localSipPort;
	
	if(transport=="TCP" || transport=="tcp")
		localSipPort = localTcpPort;
	else if(transport=="TLS" || transport=="tls")
		localSipPort = localTlsPort;
	else{ /* UDP, may use STUN */
		if( usesStun ){
			localSipPort = externalContactUdpPort;
		} else {
			localSipPort = localUdpPort;
		}
	}
	return localSipPort;
}


SipDialogConfig::SipDialogConfig(MRef<SipStack*> stack) {
//	inherited = new SipStackConfig; /// We want do do a "deep copy" here. This is so that
//					 /// we have a local copy that we can modify and that 
//					 /// no one else modifies.
//	**inherited = **commonconf;
	
	sipStack = stack;


// 	last_invite=NULL;


	local_ssrc = rand();
}

void SipDialogConfig::useIdentity(
			MRef<SipIdentity*> id,
			string transport)
{
	this->sipIdentity=id;
//	inherited->sipIdentity = identity;
// 	inherited->transport = transport;
// 	inherited->transport = inherited->sipIdentity->sipProxy.getTransport();
}


SipUri SipDialogConfig::getContactUri( bool useStun ) const{
	return sipIdentity->getContactUri( sipStack, useStun );
}

SipUri SipIdentity::getContactUri( MRef<SipStack*> sipStack,
				   bool useStun ) const{
	const SipIdentity * sipIdentity = this;
	SipUri contactUri;
	const SipUri &fromUri = sipIdentity->getSipUri();
	string transport;
	int port = 0;

	const list<SipUri> &routes = sipIdentity->getRouteSet();

	if( !routes.empty() ){
		SipUri proxy = *routes.begin();

		transport = proxy.getTransport();
	}
	else
		transport = fromUri.getTransport();

	port =sipStack->getStackConfig()->getLocalSipPort(useStun, transport);

	contactUri.setParams( fromUri.getUserName(),
			      sipStack->getStackConfig()->externalContactIP,
			      "",
			      port);
	if( transport != "" )
		contactUri.setTransport( transport );

	contactUri.setParameter("minisip", "true");

	return contactUri;
}
