
/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien, Joachim Orrblad
  
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
 * Authors: Joachim Orrblad <joachim[at]orrblad.com>
 *          
*/

/*
 * Authors: Prajwol Kumar Nakarmi <prajwolkumar.nakarmi@gmail.com>
 *
*/

#include<config.h>

#include<libmsip/SipMessageContentRCL.h>
#include<libmsip/SipMessage.h>
#include<libmsip/SipMessageContentFactory.h>
#include<libmutil/massert.h>
#include <iostream>

using namespace std;

MRef<SipMessageContent*> SipRCLContentFactory(const std::string & buf, const std::string & ContentType) {
	return new SipMessageContentRCL(buf, ContentType);
}

SipMessageContentRCL::SipMessageContentRCL(std::string t){
	this->contentType = t;
}

SipMessageContentRCL::SipMessageContentRCL(std::string content, std::string t) {
	this->contentType = t;
	this->participants = split(content, true, ',', false);
}

std::string SipMessageContentRCL::getString() const{
	std::string ret = "";
	std::vector <std::string>::const_iterator iter;

	iter = participants.begin();
	while(iter != participants.end()){
		ret += (*iter);

		iter++;

		if(iter != participants.end()){
			ret += ",";
		}
	}

	return ret;
}

std::string SipMessageContentRCL::getContentType() const{
	return contentType;
}

std::vector <std::string> SipMessageContentRCL::getParticipantList(){
	return participants;
}
