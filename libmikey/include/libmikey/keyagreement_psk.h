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


#ifndef KEYAGREEMENT_PSK_H
#define KEYAGREEMENT_PSK_H

#ifdef _MSC_VER
#ifdef LIBMIKEY_EXPORTS
#define LIBMIKEY_API __declspec(dllexport)
#else
#define LIBMIKEY_API __declspec(dllimport)
#endif
#else
#define LIBMIKEY_API
#endif

#include<openssl/dh.h>
#include<openssl/rand.h>
#include<libmikey/keyagreement.h>
#include<libmikey/oakley_groups.h>

#define DH_GROUP_OAKLEY5 0
#define DH_GROUP_OAKLEY1 1
#define DH_GROUP_OAKLEY2 2


using namespace std;


class LIBMIKEY_API KeyAgreementPSK : public KeyAgreement{
	public:
		KeyAgreementPSK( byte_t * psk, int pskLength );
		~KeyAgreementPSK();

		void generateTgk( uint32_t tgkLength = 192 );

		void genTranspEncrKey( byte_t * encrKey, int encrKeyLength );

		void genTranspSaltKey( byte_t * encrKey, int encrKeyLength );
		
		void genTranspAuthKey( byte_t * encrKey, int encrKeyLength );

		uint64_t tSent();
		void setTSent( uint64_t tSent );

		uint64_t t_received;
		byte_t * authKey;
		unsigned int authKeyLength;
		void setV(int value) {v=value;}
		int getV() {return v;}
		int macAlg;

		MikeyMessage * parseResponse( MikeyMessage * response);
		void setOffer( MikeyMessage * offer );
		MikeyMessage * buildResponse( MikeyMessage * offer);
		bool authenticate( MikeyMessage * msg);

	private:
		byte_t * pskPtr;
		int pskLengthValue;
		int v;
		uint64_t tSentValue;
};

#endif
