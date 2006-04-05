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
 * 	SdpHeaderA.h
 * Author
 * 	Erik Eliasson, eliasson@it.kth.se
 * Purpose
 * 
*/

#include<libminisip/libminisip_config.h>

#ifndef SDPHEADERA_H
#define SDPHEADERA_H

#include<libminisip/sdp/SdpHeader.h>

class SdpHeaderA : public SdpHeader{
	public:
		SdpHeaderA(std::string buildFrom);
		virtual ~SdpHeaderA();

		virtual std::string getMemObjectType(){return "SdpHeaderA";}

		std::string getAttributes();
		void setAttributes(std::string attr);

		virtual std::string getString();

		std::string getAttributeType();
		std::string getAttributeValue();

		std::string getRtpMap(int format);

	private:
		std::string attributes;

};

#endif
