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
 * 	SipTransactionInviteInitiator.h
 * Author
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 * 
*/



#ifndef SIPTRANSACTIONINVITEICLIENTUA_H
#define SIPTRANSACTIONINVITEICLIENTUA_H

#ifdef _MSC_VER
#ifdef LIBMSIP_EXPORTS
#define LIBMSIP_API __declspec(dllexport)
#else
#define LIBMSIP_API __declspec(dllimport)
#endif
#else
#define LIBMSIP_API
#endif

#include<libmsip/SipSMCommand.h>
#include<libmsip/SipTransactionInviteClient.h>

class LIBMSIP_API SipTransactionInviteClientUA: public SipTransactionInviteClient{
	public:
		SipTransactionInviteClientUA(MRef<SipDialog*> d, int seq_no, string callid);
		
		virtual ~SipTransactionInviteClientUA();

		void changeStateMachine();

		virtual string getName(){return "transaction_ua_invite_client[branch="+getBranch()+"]";}
	private:
		bool a1001_calling_completed_2xx( const SipSMCommand &command);
		bool a1002_proceeding_completed_2xx( const SipSMCommand &command);
		bool a1003_completed_completed_2xx( const SipSMCommand &command);
};

#endif
