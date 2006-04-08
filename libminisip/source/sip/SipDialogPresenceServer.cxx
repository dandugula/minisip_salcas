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

/* Copyright (C) 2004 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
*/

/* Name
 * 	SipDialogPresenceServer.cxx
 * Author
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 * 
*/

#include<config.h>

#include<libminisip/sip/SipDialogPresenceServer.h>
//#include<libmsip/SipDialogContainer.h>
#include<libmsip/SipHeaderFrom.h>
#include<libmsip/SipHeaderTo.h>
#include<libmsip/SipResponse.h>
#include<libmsip/SipTransactionNonInviteClient.h>
#include<libmsip/SipTransactionNonInviteServer.h>
#include<libmsip/SipTransactionUtils.h>
#include<libmsip/SipDialog.h>
#include<libmsip/SipCommandString.h>
#include<libminisip/sip/DefaultDialogHandler.h>
#include<libmutil/itoa.h>
#include<libmutil/Timestamp.h>
#include<libmutil/termmanip.h>
#include<libmutil/dbg.h>
#include<libmsip/SipSMCommand.h>
#include <time.h>
#include<libminisip/gui/LogEntry.h>
#include<libmsip/SipCommandString.h>
#include<libminisip/mediahandler/MediaHandler.h>
#include<libmutil/MemObject.h>
#include<libmsip/SipHeaderExpires.h>
#include<libminisip/sip/PresenceMessageContent.h>

#ifdef _WIN32_WCE
#	include"../include/minisip_wce_extra_includes.h"
#endif

using namespace std;

/*
 Presence dialog for user "user@domain".

      +-------------+
      |   s_start   |
      +-------------+
             | start_presence_server
	     | a0: -
             |
             | +---+ localPresenceUpdated
	     | |   | a2: map(new SipTransactionClient, subscribers)
             V |   V 
      +-------------+-----+ timerRemoveSubscriber
      |   default   |     | a1: removeSubscriber
      +-------------+<----+ 
             | |   ^
             | |   |
 stop_presen | +---+
 ce_server   |  SUBSCRIBE
 a3:	     |  a5: new SipTransactionServer ; addSubscribers ; set(timerRemoveSubscriber)
             V
      +-------------+
      |  term_wait  |
      +-------------+
	     | no_transactions
	     | a4:
             V
      +-------------+
      | terminated  |
      +-------------+
*/

/*
void SipDialogPresenceServer::createNotifyClientTransaction(){
	int seqNo = requestSeqNo();
	MRef<SipTransaction*> subscribetrans = new SipTransactionClient(MRef<SipDialog *>(this), seqNo, callId);
//	subscribetrans->setSocket( getPhoneConfig()->proxyConnection );
	registerTransaction(subscribetrans);
	sendNotify(subscribetrans->getBranch());
}
*/

bool SipDialogPresenceServer::a0_start_default_startpresenceserver(const SipSMCommand &command){
	if (transitionMatch(command, 
				SipCommandString::start_presence_server,
				SipSMCommand::dialog_layer,
				SipSMCommand::dialog_layer)){
		return true;
	}else{
		return false;
	}
}

bool SipDialogPresenceServer::a1_default_default_timerremovesubscriber(const SipSMCommand &command){
	if (command.getType()==SipSMCommand::COMMAND_STRING && 
			command.getCommandString().getOp().substr(0,22)=="timerRemoveSubscriber_"){
		
		string user = command.getCommandString().getOp().substr(22);
		cerr << "Removing user <"<< user << ">"<< endl;
		removeUser(user);
		return true;
	}else{
		return false;
	}
}

bool SipDialogPresenceServer::a2_default_default_localpresenceupdated(const SipSMCommand &command){
	if (transitionMatch(command, 
				SipCommandString::local_presence_update,
				SipSMCommand::dialog_layer,
				SipSMCommand::dialog_layer)){
		onlineStatus = command.getCommandString().getParam();
		sendNoticeToAll(command.getCommandString().getParam());
		return true;
	}else{
		return false;
	}
}


bool SipDialogPresenceServer::a3_default_termwait_stoppresenceserver(const SipSMCommand &command){
	if (transitionMatch(command, 
				SipCommandString::stop_presence_server,
				SipSMCommand::dialog_layer,
				SipSMCommand::dialog_layer)){
		signalIfNoTransactions();
		return true;
	}else{
		return false;
	}

}

bool SipDialogPresenceServer::a4_termwait_terminated_notransactions(const SipSMCommand &command){
	if (transitionMatch(command, 
				SipCommandString::no_transactions,
				SipSMCommand::dialog_layer,
				SipSMCommand::dialog_layer) ){
		
		SipSMCommand cmd( CommandString( dialogState.callId, SipCommandString::call_terminated), //FIXME: callId is ""
				  SipSMCommand::dialog_layer,
				  SipSMCommand::dispatcher);
		dispatcher->enqueueCommand( cmd, HIGH_PRIO_QUEUE/*, PRIO_LAST_IN_QUEUE*/ );
		return true;
	}else{
		return false;
	}
}


bool SipDialogPresenceServer::a5_default_default_SUBSCRIBE(const SipSMCommand &command){
	if (transitionMatch("SUBSCRIBE", command, SipSMCommand::transaction_layer, SipSMCommand::dialog_layer)){
		MRef<SipRequest *> sub = (SipRequest*)*command.getCommandPacket();
		
		string user = sub->getHeaderValueTo()->getUri().getUserIpString();
		cerr <<"SipDialogPresenceServer::a5_default_default_SUBSCRIBE: got subscribe request from <"<<user<<">"<<endl;

		addUser(user);
	
		sendSubscribeOk(sub);
		return true;
	}else{
		return false;
	}
}


void SipDialogPresenceServer::setUpStateMachine(){

	State<SipSMCommand,string> *s_start = new State<SipSMCommand,string>(this,"start");
	addState(s_start);

	State<SipSMCommand,string> *s_default = new State<SipSMCommand,string>(this,"default");
	addState(s_default);
	
	State<SipSMCommand,string> *s_termwait = new State<SipSMCommand,string>(this,"termwait");
	addState(s_termwait);
	
	State<SipSMCommand,string> *s_terminated = new State<SipSMCommand,string>(this,"terminated");
	addState(s_terminated);

	
	new StateTransition<SipSMCommand,string>(this, "transition_start_default_startpresenceserver",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a0_start_default_startpresenceserver,
		s_start, s_default);
	
 	new StateTransition<SipSMCommand,string>(this, "transition_default_default_timerremovesubscriber",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a1_default_default_timerremovesubscriber,
		s_default, s_default);
       
 	new StateTransition<SipSMCommand,string>(this, "transition_default_default_localpresenceupdated",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a2_default_default_localpresenceupdated,
		s_default, s_default);
 
 	new StateTransition<SipSMCommand,string>(this, "transition_default_termwait_stoppresenceserver",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a3_default_termwait_stoppresenceserver,
		s_default, s_termwait);

	new StateTransition<SipSMCommand,string>(this, "transition_termwait_terminated_notransactions",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a4_termwait_terminated_notransactions,
		s_termwait, s_terminated);
	
 	new StateTransition<SipSMCommand,string>(this, "transition_default_default_SUBSCRIBE",
		(bool (StateMachine<SipSMCommand,string>::*)(const SipSMCommand&)) &SipDialogPresenceServer::a5_default_default_SUBSCRIBE,
		s_default, s_default);


	setCurrentState(s_start);
}


SipDialogPresenceServer::SipDialogPresenceServer(MRef<SipStack*> stack, 
		//const SipDialogConfig &callconfig, 
		MRef<SipDialogConfig*> callconfig,
		bool use_stun) : 
                	SipDialog(stack,callconfig),
			useSTUN(use_stun),
			onlineStatus("online")
{

//	callId = itoa(rand())+"@"+getDialogConfig()->inherited.externalContactIP;
	dialogState.callId="";
	
	dialogState.localTag=itoa(rand());
	
	setUpStateMachine();
}

SipDialogPresenceServer::~SipDialogPresenceServer(){	
}

void SipDialogPresenceServer::sendNotice(string /*onlineStatus*/, string user){ //FIXME: use onlineStatus 
	//int seqNo = requestSeqNo();
	++dialogState.seqNo;
	string cid = "FIXME"+itoa(rand());
/*	MRef<SipTransaction*> subscribetrans = new SipTransactionNonInviteClient(sipStack, 
			//MRef<SipDialog *>(this), 
			dialogState.seqNo, 
			"NOTIFY", 
			cid);
	dispatcher->getLayerTransaction()->addTransaction(subscribetrans);
*/	//registerTransactionToDialog(subscribetrans);
	sendNotify(""/*subscribetrans->getBranch()*/, user, cid);

}

void SipDialogPresenceServer::sendNoticeToAll(string onlineStatus){
	usersLock.lock();
	for (int i=0; i<subscribing_users.size(); i++){
		sendNotice(onlineStatus, subscribing_users[i]);
	}
	usersLock.unlock();
}

void SipDialogPresenceServer::sendSubscribeOk(MRef<SipRequest*> sub){
/*	MRef<SipTransaction*> sr( new SipTransactionNonInviteServer(sipStack, 
				//MRef<SipDialog*>(this),
				sub->getCSeq(),
				sub->getCSeqMethod(),
				sub->getLastViaBranch(),
				sub->getCallId()) );
	dispatcher->getLayerTransaction()->addTransaction(sr);
	//registerTransactionToDialog(sr);

	MRef<SipMessage*> mref = *sub;
	SipSMCommand c( mref, SipSMCommand::dialog_layer, SipSMCommand::transaction_layer);
	dispatcher->enqueueCommand(c, HIGH_PRIO_QUEUE);
*/
	
	MRef<SipResponse*> ok= new SipResponse(""/*sr->getBranch()*/, 200,"OK", MRef<SipMessage*>(*sub));
	ok->getHeaderValueTo()->setParameter("tag",dialogState.localTag);

        MRef<SipMessage*> pref(*ok);
        SipSMCommand cmd( pref, SipSMCommand::dialog_layer, SipSMCommand::transaction_layer);
        dispatcher->enqueueCommand(cmd, HIGH_PRIO_QUEUE/*, PRIO_LAST_IN_QUEUE*/);


	sendNotice(onlineStatus, sub->getFrom().getUserIpString());
}

void SipDialogPresenceServer::removeUser(string user){
	usersLock.lock();
	subscribing_users.remove(user);
	usersLock.unlock();
}

void SipDialogPresenceServer::addUser(string user){
	usersLock.lock();
	subscribing_users.push_back(user);
	usersLock.unlock();
}

void SipDialogPresenceServer::sendNotify(const string &branch, string toUri, string cid){
	
	MRef<SipRequest*> notify;
	int32_t localSipPort;
	
	localSipPort = getDialogConfig()->inherited->getLocalSipPort( useSTUN );
	
	MRef<SipIdentity*> toId( new SipIdentity(toUri));
	notify = SipRequest::createSipMessageNotify(
				branch,
				cid,
				toId->getSipUri(),
				getDialogConfig()->inherited->sipIdentity->getSipUri(),
				dialogState.seqNo
				);

	notify->getHeaderValueFrom()->setParameter("tag",dialogState.localTag);

	notify->setContent(new PresenceMessageContent(getDialogConfig()->inherited->sipIdentity->getSipUri(),
				toId->getSipUri(),
				onlineStatus,
				onlineStatus
				));

        MRef<SipMessage*> pktr(*notify);

        SipSMCommand scmd(
                pktr, 
                SipSMCommand::dialog_layer, 
                SipSMCommand::transaction_layer
                );
	
	dispatcher->enqueueCommand(scmd, HIGH_PRIO_QUEUE/*, PRIO_LAST_IN_QUEUE*/);
}

bool SipDialogPresenceServer::handleCommand(const SipSMCommand &c){
	mdbg << "SipDialogPresenceServer::handleCommand got "<< c << end;

/*	if (c.getType()==SipSMCommand::COMMAND_STRING && callId.length()>0){
		if (c.getCommandString().getDestinationId() != callId ){
			cerr << "SipDialogPresenceServer returning false based on callId"<< endl;
			return false;
		}
	}
*/	

/*	if (c.getType()==SipSMCommand::COMMAND_PACKET  && callId.length()>0){
		if (c.getCommandPacket()->getCallId() != callId ){
			cerr << "SipDialogPresenceServer returning false based on callId"<< endl;
			return false;
		}
		if (c.getType()!=SipSMCommand::COMMAND_PACKET && 
				c.getCommandPacket()->getCSeq()!= getDialogConfig()->seqNo){
			cerr << "SipDialogPresenceServer returning false based on seq no"<< endl;
			return false;
		}
	
	}
*/
	
//	if (c.getType()!=SipSMCommand::COMMAND_PACKET && 
//			c.getCommandPacket()->getCSeq()!= command_seq_no)
//		return false;
	
//	mdbg << "SipDialogPresenceServer::handlePacket() got "<< c << end;
	merr << "SipDialogPresenceServer returning dialogs handleCommand"<< end;
	bool handled = SipDialog::handleCommand(c);
	
	if (!handled && c.getType()==SipSMCommand::COMMAND_STRING && c.getCommandString().getOp()==SipCommandString::no_transactions){
		return true;
	}
/*	
	if (c.getType()==SipSMCommand::COMMAND_STRING && callId.length()>0){
		if (c.getCommandString().getDestinationId() == callId ){
			mdbg << "Warning: SipDialogPresenceServer ignoring command with matching call id"<< end;
			return true;
		}
	}
	if (c.getType()==SipSMCommand::COMMAND_PACKET && callId.length()>0){
		if (c.getCommandPacket()->getCallId() == callId){
			mdbg << "Warning: SipDialogPresenceServer ignoring packet with matching call id"<< end;
			return true;
		}
	}
*/

	return handled;
}


