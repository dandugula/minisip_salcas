/*
  Copyright (C) 2005 Mikael Magnusson
  
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
 * Author(s): Mikael Magnusson <mikma@users.sourceforge.net>
 *
*/


/* Name
 * 	SipRequest.h
 * Authors
 * 	Mikael Magnusson, mikma@users.sourceforge.net
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 *	Base class of classes representing sip request messages, and
 *      used when parsing unsupported request methods.
*/



#ifndef SIPREQUEST_H
#define SIPREQUEST_H

#include<libmsip/libmsip_config.h>

#include<libmsip/SipMessage.h>
#include<libmutil/MemObject.h>

class SipStack;

class LIBMSIP_API SipRequest : public SipMessage{

	public:

		
		/**
		 *
		 * Note: The message will have a sequence
		 * number equal to the packet given as arguement
		 * to the function. This is ok for ACK packets,
		 * but for PRACK requests you will have to adjust it.
		 */
		static MRef<SipRequest*> createSipMessageAck(
				std::string branch,
				MRef<SipMessage*> pack,
				std::string to_tel_no,
				bool provisional=false);

		static MRef<SipRequest*> createSipMessageBye(
				std::string branch,
				std::string callId,
				std::string target,
				std::string to_uri,
				std::string from_uri,
				int32_t seq_no);

		static MRef<SipRequest*> createSipMessageCancel(
				std::string branch,
				MRef<SipRequest*> inv,
				std::string to_uri
				);

		static MRef<SipRequest*> createSipMessageIMMessage(
				std::string branch,
				std::string call_id,
				std::string toUri,
				const SipUri& fromUri,
				int32_t seq_no,
				std::string msg);


		static MRef<SipRequest*> createSipMessageInvite(
				const std::string &branch,
				const std::string &call_id,
				const std::string &tel_no,
				const std::string &proxyAddr,
				int32_t proxyPort,
				const std::string &localAddr,
				int32_t localSipPort,
				const std::string &from_tel_no,
				int32_t seq_no,
				const std::string &transport,
				MRef<SipStack*> stack
				);

		static MRef<SipRequest*> createSipMessageInvite(
				const std::string &branch,
				const std::string &call_id,
				const std::string &tel_no,
				const std::string &proxyAddr,
				int32_t proxyPort,
				const std::string &localAddr,
				int32_t localSipPort,
				const std::string &from_tel_no,
				int32_t seq_no,
				const std::string &username,
				const std::string &nonce,
				const std::string &realm,
				const std::string &password,
				const std::string &transport,
				MRef<SipStack*> stack);

		static MRef<SipRequest*> createSipMessageNotify(
				std::string branch,
				std::string call_id,
				const SipUri& toUri,
				const SipUri& fromUri,
				int32_t seq_no
				);

		static MRef<SipRequest*> createSipMessageRefer(
				std::string branch,
				MRef<SipRequest*> inv,
				std::string to_uri,
				std::string from_uri,
				std::string referredUri,
				int cSeqNo);

		static MRef<SipRequest*> createSipMessageRegister(
				std::string branch,
				std::string call_id,
				std::string domainarg,
				std::string localIp,
				int32_t sip_listen_port,
				std::string from_tel_no,
				int32_t seq_no,
				std::string transport,
				int expires,
				std::string auth_id="",
				std::string realm="",
				std::string nonce="",
				std::string password="");

		static MRef<SipRequest*> createSipMessageSubscribe(
				std::string branch,
				std::string call_id,
				const SipUri& toUri,
				const SipUri& fromUri,
				int32_t seq_no);




		SipRequest(std::string &build_from);

		SipRequest(std::string branch, const std::string &method,
				const std::string &uri = "");

		virtual ~SipRequest();

		virtual std::string getMemObjectType(){return "SipRequest("+method+")";}

		virtual std::string getString();

		virtual const std::string& getType();

		virtual void setMethod(const std::string &method);
		virtual std::string getMethod();

		virtual void setUri(const std::string &uri);
		virtual std::string getUri();

		/**
		 * Insert a route first in the list of routes, forcing
		 * the request through the proxy.
		 */
		void addRoute(const std::string &route);

		/**
		 * Insert a loose route first in the list of routes,
		 * forcing the request through the proxy with the
		 * specified address. Using the default transport if
		 * it's an empty string, and the default port if port
		 * is set to zero.
		 */
		void addRoute(const std::string &addr, int32_t port,
			      const std::string &transport);


		/**
		 * Adds From, To, CSeq, MaxForwards and CallId headers to
		 * the request. These are the required headers 
		 * that are added by the application layer (TU).
		 * (Via and Contact(?) headers are handled by the
		 *  transport layer.)
		 *
		 * This function is typically used when creating
		 * a SIP new request message.
		 *
		 * @param fromUri	Used to create From header
		 * @param toUri		Used to create To header
		 * @param method	Used to create CSeq header
		 * @param seqNo		Used to create CSeq header
		 * @param callId	Used to create CallId header. If
		 * 			empty string then a random value
		 * 			will be generated.
		 * 
		 */
		void addDefaultHeaders(const SipUri& fromUri, 
				const SipUri& toUri, 
				const std::string& method, 
				int seqNo, 
				const std::string& callId="");
		
	protected:

		virtual void init(std::string &build_from);

	private:
		std::string method;
		std::string uri;
};




#endif
