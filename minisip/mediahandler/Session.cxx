/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Copyright (C) 2004 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *	    Joachim Orrblad <joachim@orrblad.com>
*/

#include"Session.h"

#include"MediaStream.h"
#include"Media.h"
#include"AudioMedia.h"
#include"RtpReceiver.h"
#include"DtmfSender.h"
#include"../codecs/Codec.h"
#include"../minisip/ipprovider/IpProvider.h"
#include"../sdp/SdpPacket.h"
#include"../sdp/SdpHeaderV.h"
#include"../sdp/SdpHeaderT.h"
#include"../sdp/SdpHeaderC.h"
#include"../sdp/SdpHeaderA.h"
#include"../sdp/SdpHeaderM.h"
#include"../sdp/SdpHeaderS.h"
#include"../sdp/SdpHeaderO.h"
#include<libmikey/keyagreement.h>
#include<libmikey/keyagreement_dh.h>
#include<libmutil/dbg.h>
#include<libmutil/itoa.h>
#include<libmutil/Timestamp.h>

#ifdef _WIN32_WCE
#	include"../include/minisip_wce_extra_includes.h"
#endif

#define SESSION_LINE "s=Minisip Session"

// pn501 Added for multicodec list operations
using namespace std;

SessionRegistry * Session::registry = NULL;
MRef<KeyAgreement *> Session::precomputedKa = NULL;

Session::Session( string localIp, SipDialogSecurityConfig &securityConfig ):ka(NULL),localIpString(localIp){
	this->securityConfig = securityConfig; // hardcopy
	this->ka = Session::precomputedKa;
	dtmfTOProvider = new TimeoutProvider<DtmfEvent *, MRef<DtmfSender *> >;
	Session::precomputedKa = NULL;

	mutedSenders = true;
	silencedSources = false;
	
	if( registry ){
		registry->registerSession( this );
	}
}

void Session::unregister(){
	if( registry ){
		registry->unregisterSession( this );
	}

	if( Session::precomputedKa.isNull() ){
		Session::precomputedKa = new KeyAgreementDH( securityConfig.cert, securityConfig.cert_db, DH_GROUP_OAKLEY5 );
	}
}

Session::~Session(){
	dtmfTOProvider->stopThread();
}

MRef<SdpPacket *> Session::emptySdp(){
	MRef<SdpPacket *> result;

	result = new SdpPacket;

	MRef<SdpHeader*> v = new SdpHeaderV(0);
	result->addHeader(v);

	/* FIXME */
	string addrtype = "IP4";

	MRef<SdpHeader*> o = new SdpHeaderO("","3344","3344","IN", 
			addrtype, localIpString );
	result->addHeader(o);

	MRef<SdpHeader*> s = new SdpHeaderS(SESSION_LINE);
	result->addHeader(s);

	MRef<SdpHeader*> c = new SdpHeaderC("IN", addrtype, localIpString );
	result->addHeader(c);

	MRef<SdpHeader*> t = new SdpHeaderT(0,0);
	result->addHeader(t);

	return result;
}

MRef<SdpPacket *> Session::getSdpOffer(){ // used by the initiator when creating the first message
	MRef<SdpPacket *> result;
	list< MRef<MediaStreamReceiver *> >::iterator i;
	std::list<std::string>::iterator iAttribute;
	std::list<std::string> attributes;
	string type;
	uint16_t localPort;	
	MRef<SdpHeaderM *> m;
	string keyMgmtMessage;
	std::list<MRef<Codec *> > codecs;
	std::list<MRef<Codec *> >::iterator iC;
	uint8_t payloadType;
	string rtpmap;
	const char *transport = NULL;

// 	cerr << "Session::getSdpOffer" << endl;
	result = emptySdp();
	if( securityConfig.secured ){
		MRef<SdpHeaderA *> a;
		keyMgmtMessage = initiatorCreate();  //in KeyAgreement.cxx
		if( ! securityConfig.secured ){
			// something went wrong
			return NULL;
		}
		result->setSessionLevelAttribute( "key-mgmt", keyMgmtMessage );
		transport = "RTP/SAVP";
	}
	else{
		transport = "RTP/AVP";
	}

	for( i = mediaStreamReceivers.begin(); i != mediaStreamReceivers.end(); i++ ){
		codecs = (*i)->getAvailableCodecs();

		type = (*i)->getSdpMediaType();
		localPort = (*i)->getPort();
		m = new SdpHeaderM( type, localPort, 1, transport );
		
		for( iC = codecs.begin(); iC != codecs.end(); iC ++ ){
			payloadType = (*iC)->getSdpMediaType();
			rtpmap = (*iC)->getSdpMediaAttributes();
			
			m->addFormat( payloadType );
			if( rtpmap != "" ){
				MRef<SdpHeaderA*> a = new SdpHeaderA("a=X");
				a->setAttributes( "rtpmap:" + itoa( payloadType) + " " + rtpmap );
				m->addAttribute( *a );
			}
			if( (*iC)->getCodecName() == "iLBC" ) { //for now, iLBC codec only supports 20ms frames
				MRef<SdpHeaderA*> ilbc_fmtp = new SdpHeaderA("a=X");
				ilbc_fmtp->setAttributes("fmtp:" + itoa( payloadType) + " mode=20" );
				m->addAttribute(*ilbc_fmtp);
			}
		}
		//added static DTMF SDP headers in INVITE
		m->addFormat(101);
		MRef<SdpHeaderA*> dtmf = new SdpHeaderA("a=X");
		dtmf->setAttributes("rtpmap:101 telephone-event/8000");
		m->addAttribute(*dtmf);
		MRef<SdpHeaderA*> dtmf_fmtp = new SdpHeaderA("a=X");
		dtmf_fmtp->setAttributes("fmtp:101 0-15");
		m->addAttribute(*dtmf_fmtp);
		
		result->addHeader( *m );

		attributes = (*i)->getSdpAttributes();
		for( iAttribute = attributes.begin(); iAttribute != attributes.end(); iAttribute ++ ){
			MRef<SdpHeaderA*> a = new SdpHeaderA("a=X");
			a->setAttributes( *iAttribute );
			m->addAttribute( *a );
		}
	}
#ifdef DEBUG_OUTPUT	
	cerr << "Session::getSdpOffer: " << endl << result->getString() << endl << endl;
#endif
	return result;
}

bool Session::setSdpAnswer( MRef<SdpPacket *> answer, string peerUri ){
	unsigned int i;
	int j;
	MRef<MediaStreamReceiver *> receiver;
	IPAddress * remoteAddress;
	// Not used
	int port;

	this->peerUri = peerUri;
#ifdef DEBUG_OUTPUT
// 	cerr << "Session::setSdpAnswer" << endl;
#endif
	if( securityConfig.secured ){
		/* get the keymgt: attribute */
		string keyMgmtMessage = 
			answer->getSessionLevelAttribute( "key-mgmt" );
		if( !initiatorAuthenticate( keyMgmtMessage ) ){
			errorString = "Could not authenticate the key management message";
			fprintf( stderr, "Auth failed\n");
			return false;
		}

		string mikeyErrorMsg = initiatorParse();
		if( mikeyErrorMsg != "" ){
			errorString = "Could not parse the key management message. ";
			errorString += mikeyErrorMsg;
			fprintf( stderr, "Parse failed\n");
			return false;
		}

		/*
		else{
			if( ka && ka->type() == KEY_AGREEMENT_TYPE_DH ){
				((KeyAgreementDH *)*ka)->computeTgk();
			}
		}
		*/
	}
	remoteAddress = answer->getRemoteAddr( port );

	for( i = 0; i < answer->getHeaders().size(); i++ ){
		if( answer->getHeaders()[i]->getType() == SDP_HEADER_TYPE_M ){
			MRef<SdpHeaderM *> m = ((SdpHeaderM*)*(answer->getHeaders()[i]));
#ifdef DEBUG_OUTPUT
			cerr << "Session::setSdpAnswer - trying media line " << m->getString() << endl;
#endif
			
			for( j = 0; j < m->getNrFormats(); j++ ){
				receiver = matchFormat( m, j, remoteAddress );
#ifdef DEBUG_OUTPUT
				if( receiver )
					cerr << "Session::setSdpAnswer - Found receiver!" << endl;
#endif
				if( receiver && m->getPort() == 0 ){
					/* This offer was rejected */
					receiver->disabled = true;
				}
				else if( receiver ){
					/* Be ready to receive */
					receiver->start();
				}
			}
		}
	}
	return true;
}

MRef<MediaStreamReceiver *> Session::matchFormat( MRef<SdpHeaderM *> m, uint32_t iFormat, IPAddress * remoteAddress ){
	list< MRef<MediaStreamSender *> >::iterator iSStream;
	list< MRef<MediaStreamReceiver *> >::iterator iRStream;

	/* If we have a sender for this format, activate it */
#ifdef DEBUG_OUTPUT
	mdbg << "Session::matchFormat: Starting senders loop" << end;
#endif
	uint8_t j = 1;
	mediaStreamSendersLock.lock();
	for( iSStream =  mediaStreamSenders.begin(); iSStream != mediaStreamSenders.end(); iSStream++,j++ ){
#ifdef DEBUG_OUTPUT
		mdbg << "Trying a sender"<< end;
#endif
		if( (*iSStream)->matches( m, iFormat ) ){
#ifdef DEBUG_OUTPUT
			mdbg << "Found sender for " << (*iSStream)->getSdpMediaType()<< end;
#endif

#if 0
			if( ka ){
				ka->addSrtpStream( (*iStream)->getSsrc(),
					0, /*ROC */
					0, /* policy (fix me) */
					2*j/* CSID */
					);
			}
#endif
	
			(*iSStream)->setPort( (uint16_t)m->getPort() );
			(*iSStream)->setRemoteAddress( remoteAddress );
		}
	}
	mediaStreamSendersLock.unlock();
	/* Look for a receiver */
#ifdef DEBUG_OUTPUT
	mdbg << "Starting receivers loop"<< end;
#endif
	for( iRStream =  mediaStreamReceivers.begin(); iRStream != mediaStreamReceivers.end(); iRStream ++ ){
		if( (*iRStream)->matches( m, iFormat ) ){
#ifdef DEBUG_OUTPUT
			mdbg << "Found receiver for " << (*iRStream)->getSdpMediaType()<< end;
#endif
			return (*iRStream);
		}
	}

	return NULL;
}

bool Session::setSdpOffer( MRef<SdpPacket *> offer, string peerUri ){ // used by the responder when receiving the first message
	unsigned int i;
	int j;
	MRef<MediaStreamReceiver *> receiver;
	MRef<SdpPacket *> packet;
	IPAddress * remoteAddress;
	// Not used
	int port;
	string keyMgmtMessage;
	std::list<std::string>::iterator iAttribute;
	std::list<std::string> attributes;
	std::list<MRef<Codec *> > codecs;
	std::list<MRef<Codec *> >::iterator iC;
// 	uint8_t payloadType;
	string rtpmap;

	this->peerUri = peerUri;
// 	cerr << "Session::setSdpOffer" << endl;

	keyMgmtMessage = offer->getSessionLevelAttribute( "key-mgmt" );

	if( keyMgmtMessage != "" ){
		if( !responderAuthenticate( keyMgmtMessage ) ){
			errorString =  "Incoming key management message could not be authenticated";
			if( ka ){
				errorString += ka->authError();
			}
			return false;
		}
		else //Here we set the offer in ka
			setMikeyOffer();
	}
	else{
		securityConfig.secured = false;
		securityConfig.ka_type = KEY_MGMT_METHOD_NULL;
	}
		

	remoteAddress = offer->getRemoteAddr( port );

	sdpAnswer = emptySdp();

	for( i = 0; i < offer->getHeaders().size(); i++ ){

		if( offer->getHeaders()[i]->getType() == SDP_HEADER_TYPE_M ){
			MRef<SdpHeaderM *> offerM = (SdpHeaderM*)*(offer->getHeaders()[i]);

			const string &transport = offerM->getTransport();

			if (transport != "RTP/AVP" &&
			    !securityConfig.secured &&
			    transport == "RTP/SAVP") {
				errorString += "No supported SRTP key exchange method";
				return false;
			}


			MRef<SdpHeaderM *> answerM = new SdpHeaderM(
					offerM->getMedia(), 0, 0,
					offerM->getTransport() );

			sdpAnswer->addHeader( *answerM );

			for( j = 0; j < offerM->getNrFormats(); j++ ){
				receiver = matchFormat( offerM, j, remoteAddress );

				if( receiver ){
					if( answerM->getPort() == 0 ){
						answerM->setPort( receiver->getPort() );
					}
					else{
						/* This media has already been treated */
						continue;
					}
					
					/* found a receiver, accept the offer */
					//add the payload type to the offer, as accepted ...
					int payloadTypeAccepted = offerM->getFormat( j );
					string payloadStr = itoa( payloadTypeAccepted );
					answerM->addFormat( payloadTypeAccepted );
					MRef<SdpHeaderA*> rtpmap = new SdpHeaderA("a=X");
					MRef<SdpHeaderA*> fmtp = new SdpHeaderA("a=X");
					       
					rtpmap->setAttributes( "fmtp:" + payloadStr
								 + " " + offerM->getRtpMap( payloadTypeAccepted ) );
					fmtp->setAttributes(   "rtpmap:" + payloadStr
								+ " " + offerM->getRtpMap( payloadTypeAccepted ) );
					
					answerM->addAttribute( *rtpmap );
					answerM->addAttribute( *fmtp );
					
					/* Additional attributes (framesize, ...) */
					attributes = receiver->getSdpAttributes();
					for( iAttribute = attributes.begin(); iAttribute != attributes.end(); iAttribute ++ ){
						MRef<SdpHeaderA*> a = new SdpHeaderA("a=X");
						a->setAttributes( *iAttribute );
						answerM->addAttribute( *a );
					}
				}
			}
		}
	}
	return true;
}

MRef<SdpPacket *> Session::getSdpAnswer(){
// 	cerr << "Session::getSdpAnswer" << endl;
	if( securityConfig.secured ){
		string keyMgmtAnswer;
		// Generate the key management answer message
		keyMgmtAnswer = responderParse();
		
		if( !securityConfig.secured ){
			// Something went wrong
			errorString = "Could not parse key management message.";
			fprintf(stderr, "responderParse failed\n" );
			return NULL;
		}
		/*
		else{
			if( ka && ka->type() == KEY_AGREEMENT_TYPE_DH ){
				((KeyAgreementDH *)*ka)->computeTgk();
			}
		}
		*/
		
		sdpAnswer->setSessionLevelAttribute( "key-mgmt", "mikey "+keyMgmtAnswer );
	}
	
	return sdpAnswer;
}

void Session::start(){
	list< MRef<MediaStreamSender * > >::iterator iS;
	list< MRef<MediaStreamReceiver * > >::iterator iR;

	if( securityConfig.secured && ka && ka->type() == KEY_AGREEMENT_TYPE_DH ){
#ifdef ENABLE_TS
	ts.save( TGK_START );
#endif
	((KeyAgreementDH *)*ka)->computeTgk();
#ifdef ENABLE_TS
	ts.save( TGK_END );
#endif
	}

	for( iR = mediaStreamReceivers.begin(); iR != mediaStreamReceivers.end(); iR++ ){
		if( ! (*iR)->disabled ){
			if( securityConfig.secured ){
				(*iR)->setKeyAgreement( ka );
			}
			(*iR)->start();
		}
	}
	
	mediaStreamSendersLock.lock();
	for( iS = mediaStreamSenders.begin(); iS != mediaStreamSenders.end(); iS++ ){
		if( (*iS)->getPort() ){
			if( securityConfig.secured ){
				(*iS)->setKeyAgreement( ka );
			}
			(*iS)->start();
		}
	}
	mediaStreamSendersLock.unlock();
}

void Session::stop(){
	list< MRef<MediaStreamSender * > >::iterator iS;
	list< MRef<MediaStreamReceiver * > >::iterator iR;

	for( iR = mediaStreamReceivers.begin(); iR != mediaStreamReceivers.end(); iR++ ){
		if( ! (*iR)->disabled ){
			(*iR)->stop();
		}
	}
	
	mediaStreamSendersLock.lock();
	for( iS = mediaStreamSenders.begin(); iS != mediaStreamSenders.end(); iS++ ){
		if( (*iS)->getPort() ){
			(*iS)->stop();
		}
	}
	mediaStreamSendersLock.unlock();

}


void Session::addMediaStreamReceiver( MRef<MediaStreamReceiver *> mediaStream ){
	mediaStreamReceivers.push_back( *mediaStream );
	silenceSources( silencedSources );
}

void Session::addMediaStreamSender( MRef<MediaStreamSender *> mediaStream ){
	mediaStreamSendersLock.lock();
	mediaStream->setMuted( mutedSenders );
	mediaStreamSenders.push_back( *mediaStream );
	mediaStreamSendersLock.unlock();
}

string Session::getErrorString(){
	return errorString;
}

uint16_t Session::getErrorCode(){
	return errorCode;
}

bool Session::isSecure(){
	return securityConfig.secured;
}

string Session::getCallId(){
	return callId;
}

void Session::setCallId( const string callId ){
	this->callId = callId;
}

void Session::sendDtmf( uint8_t symbol ){
	MRef<DtmfSender *> dtmfSender = new DtmfSender( this );
	uint32_t * ts = new uint32_t;
	*ts = 0;
	dtmfTOProvider->request_timeout( 0, dtmfSender, new DtmfEvent( symbol, 10, 0, false, true, ts ) );
	dtmfTOProvider->request_timeout( 5, dtmfSender, new DtmfEvent( symbol, 10, 0, false, false, ts ) );
	dtmfTOProvider->request_timeout( 10, dtmfSender, new DtmfEvent( symbol, 10, 0, false, false, ts ) );
	
	dtmfTOProvider->request_timeout( 15, dtmfSender, new DtmfEvent( symbol, 10, 800, true, false, ts ) );
	dtmfTOProvider->request_timeout( 20, dtmfSender, new DtmfEvent( symbol, 10, 800, true, false, ts ) );
	dtmfTOProvider->request_timeout( 25, dtmfSender, new DtmfEvent( symbol, 10, 800, true, false, ts, true ) );
	
}

void Session::muteSenders (bool mute) {
	mutedSenders = mute;
	mediaStreamSendersLock.lock();
	for( std::list< MRef<MediaStreamSender *> >::iterator it =  mediaStreamSenders.begin();
				it !=  mediaStreamSenders.end(); it++ ) {
		(*it)->setMuted( mute );
	}
	mediaStreamSendersLock.unlock();
	
}

void Session::silenceSources ( bool silence ) {
#ifdef DEBUG_OUTPUT
/*	if( silence )
		cerr << "Session::SilenceSources - true" << endl;
	else 
		cerr << "Session::SilenceSources - false" << endl;
*/
#endif
	silencedSources = silence;
	for( std::list< MRef<MediaStreamReceiver *> >::iterator it =  mediaStreamReceivers.begin();
				it !=  mediaStreamReceivers.end(); it++ ) {
		list<uint32_t> ssrcList;
		list<uint32_t>::iterator ssrcIt;
		MRef<AudioMedia *> audioMedia;
		MRef<AudioMediaSource *> audioSource;
		
		//obtain the media object used by the media stream, and try to cast it to
		//an audiomedia ... 
		audioMedia = dynamic_cast<AudioMedia *>( *( (*it)->getMedia() ) );
		//if it is not audiomedia, we are not interested
		if( !audioMedia ) {
			continue;
		}
		
		ssrcList = (*it)->getSsrcList();
		
		for( ssrcIt = ssrcList.begin(); ssrcIt != ssrcList.end(); ssrcIt++ ) {
			audioSource = audioMedia->getSource( *ssrcIt );
			if( !audioSource ) {
// 				cerr << "Session::SilenceSources - skipping ssrc ... no source found" << endl;
				continue;
			} else {
				audioSource->setSilenced( silence );
// 				cerr << "Session::SilenceSources - silencing source " << itoa(*ssrcIt) << endl;
			}
		}
	}
}

#ifdef DEBUG_OUTPUT
string Session::getDebugString() {
	string ret;
	ret = getMemObjectType() + ": this=" + itoa((int)this) +
		"\n;          callid=" + getCallId() +
		"; peerUri=" + peerUri;
		
	ret += "\n          ";
	if( mutedSenders )
		ret += "; mutedSenders = true";
	else 
		ret += "; mutedSenders = false";	
	
	ret += "\n          ";
	if( silencedSources )
		ret += "; silencedSources = true";
	else 
		ret += "; silencedSources = false";	
	
	for( std::list< MRef<MediaStreamReceiver *> >::iterator it = mediaStreamReceivers.begin();
				it != mediaStreamReceivers.end(); it++ ) {
		ret += "\n" + (*it)->getDebugString();
	}
	for( std::list< MRef<MediaStreamSender *> >::iterator it2 =  mediaStreamSenders.begin();
				it2 !=  mediaStreamSenders.end(); it2++ ) {
		ret += "\n" + (*it2)->getDebugString();
	}
	return ret;
}
#endif

void Session::clearMediaStreamReceivers() {
	mediaStreamReceivers.clear();
}

