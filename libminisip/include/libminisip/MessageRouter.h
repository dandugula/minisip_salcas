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


/* Copyright (C) 2004 
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
*/

#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#ifdef _MSC_VER
#ifdef LIBMINISIP_EXPORTS
#define LIBMINISIP_API __declspec(dllexport)
#else
#define LIBMINISIP_API __declspec(dllimport)
#endif
#else
#define LIBMINISIP_API
#endif


#include<libmsip/SipCallback.h>
#include<libmsip/SipInvite.h>
#include<libmsip/SipResponse.h>
#include<libmutil/minilist.h>
#include<libminisip/GuiCallback.h>
#include<libminisip/Sip.h>
#include<libminisip/Gui.h>
#include<libminisip/ConferenceControl.h>
#include<libminisip/ConfCallback.h>
#include<libminisip/SipSoftPhoneConfiguration.h>


class LIBMINISIP_API MessageRouter: public SipCallback, public GuiCallback, public ConfCallback{
	public:
		MessageRouter();
		virtual ~MessageRouter(){}
		
		void setSip(MRef<Sip*> ssp);
		void setGui(MRef<Gui *>g){gui = g;}
		virtual void setConferenceController(ConferenceControl *conf);
		virtual void removeConferenceController(ConferenceControl *conf);
		void setMediaHandler(MRef<MediaHandler *> mediaHandler){
			this->mediaHandler = mediaHandler;}

		virtual void sipcb_handleCommand(CommandString &command);
		virtual void sipcb_handleConfCommand(CommandString &command);
		virtual void guicb_handleCommand(CommandString &command);
		virtual void guicb_handleConfCommand(string &conferencename);
		virtual void guicb_handleConfCommand(CommandString &command);
		virtual void guicb_handleMediaCommand(CommandString &command);
		
		virtual string guicb_doInvite(string sip_url);
		
		virtual string confcb_doJoin(string user, minilist<ConfMember> *list, string congId);
		virtual string confcb_doConnect(string user, string confId);
		virtual void confcb_handleSipCommand(string &command){}
		virtual void confcb_handleSipCommand(CommandString &command);
		virtual void confcb_handleGuiCommand(CommandString &command);	
		virtual ConferenceControl* getConferenceController(string confid);
	private:
		
		MRef<Gui *> gui;
		minilist<ConferenceControl *> confrout;//bm
		MRef<Sip*> sip;
		MRef<MediaHandler *> mediaHandler;
};

#endif