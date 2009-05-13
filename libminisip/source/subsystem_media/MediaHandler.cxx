/*
 Copyright (C) 2004-2006 the Minisip Team
 
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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/* Copyright (C) 2004, 2005, 2006 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
*/

#include <config.h>

#include"MediaHandler.h"

#include<string.h>
#include<libminisip/signaling/sdp/SdpPacket.h>
#include<libmikey/KeyAgreement.h>
#include<libminisip/signaling/sip/SipSoftPhoneConfiguration.h>
#include<libminisip/ipprovider/IpProvider.h>
#include<libminisip/media/codecs/Codec.h>
#include"Session.h"
#include<libminisip/media/MediaStream.h>

#include<libminisip/media/zrtp/ZrtpHostBridgeMinisip.h>
#include<libminisip/media/Media.h>
#include<libminisip/media/ReliableMedia.h>
#include<libminisip/media/RtpReceiver.h>
#include<libminisip/media/MediaCommandString.h>
#include<libmnetutil/UDPSocket.h>

#include<libminisip/media/soundcard/SoundIO.h>
#include<libminisip/media/soundcard/SoundDevice.h>
#include<libminisip/media/codecs/Codec.h>

#include<libminisip/media/CallRecorder.h>
#include"SessionRegistry.h"

#ifdef _WIN32_WCE
#	include"../include/minisip_wce_extra_includes.h"
#endif



using namespace std;

MediaHandler::MediaHandler( MRef<SipSoftPhoneConfiguration *> conf, MRef<IpProvider *> ipp, MRef<IpProvider *> ip6p ): ip6Provider(ip6p){

	this->ipProvider = ipp;
	this->config = conf;
	init();
}

MediaHandler::~MediaHandler(){
}

void MediaHandler::init(){

	media.clear();

	MRef<MediaRegistry*> registry = MediaRegistry::getInstance();
	MediaRegistry::const_iterator i;
	MediaRegistry::const_iterator last = registry->end();

	for( i = registry->begin(); i != last; i++ ){
		MRef<MPlugin *> plugin = *i;
		MRef<MediaPlugin *> mediaPlugin = dynamic_cast<MediaPlugin*>( *plugin );
		if( mediaPlugin ){
			MRef<Media *> m = mediaPlugin->createMedia( config );
			MRef<AudioMedia *> audio = dynamic_cast<AudioMedia *>( *m );

			if( m ){
				registerMedia( m );
			}

			if( !audioMedia && audio ){
				audioMedia = audio;
			}

		}
	}

	Session::registry = this;

//	muteAllButOne = config->muteAllButOne;
	
        ringtoneFile = config->ringtone;
}

MRef<Session *> MediaHandler::createSession( MRef<SipIdentity*> id, string callId ){

	list< MRef<Media *> >::iterator i;
	MRef<Session *> session;
	MRef<RtpReceiver *> rtpReceiver = NULL;
	MRef<RtpReceiver *> rtp6Receiver;
	string contactIp;
	string contactIp6;
#ifdef ZRTP_SUPPORT
	MRef<ZrtpHostBridgeMinisip *> zhb = NULL;
#endif

	if( ipProvider )
		contactIp = ipProvider->getExternalIp();

	if( ip6Provider )
		contactIp6 = ip6Provider->getExternalIp();

	session = new Session( contactIp, /*securityConfig*/ id, contactIp6 );
	session->setCallId( callId );

	for( i = media.begin(); i != media.end(); i++ ){
		MRef<Media *> m = *i;
		MRef<RealtimeMedia*> rtm = dynamic_cast<RealtimeMedia*>(*m);
		MRef<ReliableMedia*> relm = dynamic_cast<ReliableMedia*>(*m);

		if (relm){
			session->addReliableMediaSession( relm->createMediaStream(callId) );
		}

		if( rtm && rtm->receive ){
			if( ipProvider )
				rtpReceiver = new RtpReceiver( ipProvider, callId );

			if( ip6Provider )
				rtp6Receiver = new RtpReceiver( ip6Provider, callId );

			MRef<RealtimeMediaStreamReceiver *> rStream;
			rStream = new RealtimeMediaStreamReceiver( callId, rtm, rtpReceiver, rtp6Receiver );
			session->addRealtimeMediaStreamReceiver( rStream );

/* FIXME: The call recorder makes the audio output sound bad. Most likely,
 * it causes incoming audio to be put into the jitter buffer twice which
 * makes it overflow. Not sure why. FIXME.

			// Need to dereference MRef:s, Can't compare MRef:s
			// of different types
			if( *rtm == *audioMedia ) {
				CallRecorder * cr;
				cr = new CallRecorder( audioMedia, rtpReceiver, ipProvider );
				session->callRecorder = cr;
			}
*/

#ifdef ZRTP_SUPPORT
		    if(/*securityConfig.use_zrtp*/ id->use_zrtp) {
#ifdef DEBUG_OUTPUT
		        cerr << "MediaHandler::createSession: enabling ZRTP for receiver" << callId << endl;
#endif

			zhb = new ZrtpHostBridgeMinisip(callId, *messageRouterCallback);
			zhb->setReceiver(rStream);
			rStream->setZrtpHostBridge(zhb);
		    }
#endif
		}
		
		if( rtm && rtm->send ){
		    if( !rtpReceiver && !ipProvider.isNull() ){
			rtpReceiver = new RtpReceiver( ipProvider, callId );
		    }

		    if( !rtp6Receiver && !ip6Provider.isNull() ){
		      rtp6Receiver = new RtpReceiver( ip6Provider, callId );
		    }

		    MRef<UDPSocket *> sock;
		    MRef<UDPSocket *> sock6;

		    if( rtpReceiver )
			    sock = rtpReceiver->getSocket();
		    if( rtp6Receiver )
			    sock6 = rtp6Receiver->getSocket();

		    MRef<RealtimeMediaStreamSender *> sStream;
		    sStream = new RealtimeMediaStreamSender( callId, rtm, sock, sock6 );
		    session->addRealtimeMediaStreamSender( sStream );
#ifdef ZRTP_SUPPORT
		    if(/*securityConfig.use_zrtp*/ id->use_zrtp) {
#ifdef DEBUG_OUTPUT
		        cerr << "MediaHandler::createSession: enabling ZRTP for sender: " << callId << endl;
#endif
			if (!zhb) {
                            zhb = new ZrtpHostBridgeMinisip(callId, *messageRouterCallback);
			}
			zhb->setSender(sStream);
			sStream->setZrtpHostBridge(zhb);
		    }
#endif
		}
	}
	
	//set the audio settings for this session ...
	session->muteSenders( true );
	session->silenceSources( false );
	
	return session;

}


void MediaHandler::registerMedia( MRef<Media*> m){
	this->media.push_back( m );
}

CommandString MediaHandler::handleCommandResp(string, const CommandString& c){
	assert(1==0); //Not used
	return c; // Not reached; masks warning
}


void MediaHandler::handleCommand(string subsystem, const CommandString& command ){
	assert(subsystem=="media");

	if( command.getOp() == MediaCommandString::start_ringing ){
// 		cerr << "MediaHandler::handleCmd - start ringing" << endl;
		if( audioMedia && ringtoneFile != "" ){
			audioMedia->startRinging( ringtoneFile );
		}
		return;
	}

	if( command.getOp() == MediaCommandString::stop_ringing ){
// 		cerr << "MediaHandler::handleCmd - stop ringing" << endl;
		if( audioMedia ){
			audioMedia->stopRinging();
		}
		return;
	}
	
	if( command.getOp() == MediaCommandString::session_debug ){
	#ifdef DEBUG_OUTPUT
		cerr << getDebugString() << endl;
	#endif
		return;
	}

	if( command.getOp() == MediaCommandString::audio_forwarding_enable){
		getMedia("audio")->setMediaForwarding(true);
		return;
	}

	if( command.getOp() == MediaCommandString::audio_forwarding_disable){
		getMedia("audio")->setMediaForwarding(false);
		return;
	}

	if( command.getOp() == MediaCommandString::video_forwarding_enable){
		getMedia("video")->setMediaForwarding(true);
		return;
	}

	if( command.getOp() == MediaCommandString::video_forwarding_disable){
		getMedia("video")->setMediaForwarding(false);
		return;
	}


	if( command.getOp() == MediaCommandString::send_dtmf){
		MRef<Session *> session = Session::registry->getSession( command.getDestinationId() );
		if( session ){
			string tmp = command.getParam();
			if (tmp.length()==1){
				uint8_t c = tmp[0];
				session->sendDtmf( c );
			}else{
				merr("media/dtmf") << "Error: DTMF format error. Ignored."<<endl;
			}
		}
		return;
	}


	
	if( command.getOp() == MediaCommandString::set_session_sound_settings ){
		bool turnOn;
	#ifdef DEBUG_OUTPUT
		cerr << "MediaHandler::handleCmd: received set session sound settings" 
				<< endl << "     " << command.getString()  << endl;
	#endif
		if( command.getParam2() == "ON" ) turnOn = true;
		else turnOn = false;
		setSessionSoundSettings( command.getDestinationId(), 
					command.getParam(), 
					turnOn );
		return;
	}

	if( command.getOp() == MediaCommandString::reload ){
		init();
		return;
	}
	
	if( command.getOp() == "call_recorder_start_stop" ){
	#ifdef DEBUG_OUTPUT
		cerr << "MediaHandler::handleCmd: call_recorder_start_stop" << endl 
			<< command.getString() << endl;
	#endif		
		bool start = (command.getParam() == "START" );
		sessionCallRecorderStart( command.getDestinationId(), start );
	}
}

std::string MediaHandler::getExtIP(){
	return ipProvider->getExternalIp();
}

void MediaHandler::setSessionSoundSettings( std::string callid, std::string side, bool turnOn ) {
        list<MRef<Session *> >::iterator iSession;

	//what to do with received audio
	if( side == "receivers" ) {
		sessionsLock.lock();
		for( iSession = sessions.begin(); iSession != sessions.end(); iSession++ ){
			if( (*iSession)->getCallId() == callid ){
				//the meaning of turnOn is the opposite of the Session:: functions ... silence/mute
				(*iSession)->silenceSources( ! turnOn );
			} 
		}
		sessionsLock.unlock();
	} else if ( side == "senders" ) { //what to do with audio to be sent over the net
		//set the sender ON as requested ... 
		sessionsLock.lock();
		for( iSession = sessions.begin(); iSession != sessions.end(); iSession++ ){
			if( (*iSession)->getCallId() == callid ){
				//the meaning of turnOn is the opposite of the Session:: functions ... silence/mute
				(*iSession)->muteSenders( !turnOn );
				
			} 
		}
		sessionsLock.unlock();
	} else {
		cerr << "MediaHandler::setSessionSoundSettings - not understood" << endl;
		return;
	}
	
}

void MediaHandler::sessionCallRecorderStart( string callid, bool start ) {
	CallRecorder * cr;
	list<MRef<Session *> >::iterator iSession;
	
	sessionsLock.lock();
	for( iSession = sessions.begin(); iSession != sessions.end(); iSession++ ){
		if( (*iSession)->getCallId() == callid ){
			cr = dynamic_cast<CallRecorder *>( *((*iSession)->callRecorder) );
			if( cr ) {
				cr->setAllowStart( start );
			}
		}
	}
	sessionsLock.unlock();
}

MRef<Media*> MediaHandler::getMedia(std::string sdpType){
	list<MRef<Media*> >::iterator i;
	for (i=media.begin(); i!=media.end(); i++){
		if ((*i)->getSdpMediaType()==sdpType){
			return *i;
		}
	}
	return NULL;
}


#ifdef DEBUG_OUTPUT	
string MediaHandler::getDebugString() {
	string ret;
	ret = getMemObjectType() + ": Debug Info\n";
	for( std::list<MRef<Session *> >::iterator it = sessions.begin();
				it != sessions.end(); it++ ) {
		ret += "** Session : \n" + (*it)->getDebugString() + "\n";
	}
	return ret;
}
#endif
