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


/* Name
 * 	SipTransaction.cxx
 * Author
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 * 
*/

#include<config.h>


#include<libmsip/SipDialog.h>
#include<libmsip/SipTransaction.h>
#include<libmsip/SipDialogContainer.h>
#include<libmsip/SipMessageTransport.h>
#include<libmsip/SipCommandString.h>
#include<libmutil/dbg.h>

#include<libmsip/SipTransaction.h>
#include<libmnetutil/IP4Address.h>
#include<libmnetutil/NetworkException.h>

SipTransaction::SipTransaction(/*const string &memType,*/ MRef<SipDialog*> d, int cseq, const string &b, string callid): 
		StateMachine<SipSMCommand, string>(d->getTimeoutProvider() ), 
		dialog(d), 
		socket(NULL),
		cSeqNo(cseq),
		branch(b)
{
#ifdef MINISIP_MEMDEBUG 
	dialog.setUser("SipTransaction");
#endif
	callId = callid;
	if (b==""){
		branch = "z9hG4bK" + itoa(rand());		//magic cookie plus random number
	}

	try{
		toaddr = new IP4Address(dialog->getDialogConfig()->inherited.sipIdentity->sipProxy.sipProxyAddressString);
	}
	catch( HostNotFound * exc ){
		toaddr = NULL;
		SipSMCommand cmd( CommandString( callId, SipCommandString::transport_error, exc->errorDescription() ), SipSMCommand::transaction, SipSMCommand::TU );
        	dialog->getDialogContainer()->enqueueCommand(cmd, HIGH_PRIO_QUEUE, PRIO_LAST_IN_QUEUE);
		delete exc;
	}

	port = dialog->getDialogConfig()->inherited.sipIdentity->sipProxy.sipProxyPort;
}

SipTransaction::~SipTransaction(){

}

string SipTransaction::getBranch(){
	return branch;
}

void SipTransaction::handleTimeout(const string &c){
        SipSMCommand cmd(CommandString(callId,c),SipSMCommand::transaction,SipSMCommand::transaction);
        dialog->getDialogContainer()->enqueueTimeout( this, cmd);
}


void SipTransaction::send(MRef<SipMessage*> pack, bool addVia, string br){
		if (br=="")
			br = branch;

		if(toaddr){
		
			dialog->getDialogConfig()->inherited.sipTransport->sendMessage(pack,
					*toaddr,
					port, 
					br,
					addVia);
		}
#ifdef DEBUG_OUTPUT
		mdbg<< "SipTransaction::send: WARNING: Ignoring created socket"<<end;
#endif
		
		return;
}


bool SipTransaction::handleCommand(const SipSMCommand &command){
        if (! (command.getDestination()==SipSMCommand::transaction 
				|| command.getDestination()==SipSMCommand::ANY)){
                return false;
	}

        if (command.getType()==SipSMCommand::COMMAND_PACKET 
				&& command.getCommandPacket()->getCSeq()!= getCSeqNo() 
				&& getCSeqNo()!=-1){
                return false;
        }

	if (command.getType()==SipSMCommand::COMMAND_PACKET &&
			command.getCommandPacket()->getCallId()!= callId){
		return false;
	}
	 
        return StateMachine<SipSMCommand,string>::handleCommand(command);
}


SipTransactionClient::SipTransactionClient(MRef<SipDialog*> d, int seq_no, const string &branch, string callid):
	SipTransaction(d,seq_no,branch,callid)
{
	
}

SipTransactionClient::~SipTransactionClient(){

}

SipTransactionServer::SipTransactionServer(MRef<SipDialog*> d, int seq_no, const string &branch, string callid):
	SipTransaction(d,seq_no,branch,callid)
{
	
}

SipTransactionServer::~SipTransactionServer(){

}
