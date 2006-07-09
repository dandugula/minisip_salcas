/*
 Copyright (C) 2006 Werner Dittmann
 
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

/*
 * Authors: Werner Dittmann <Werner.Dittmann@t-online.de>
 */
#ifndef _ZRTPPACKETCOMMIT_H_
#define _ZRTPPACKETCOMMIT_H_

#include <libminisip/zrtp/ZrtpPacketBase.h>

class ZrtpPacketCommit : public ZrtpPacketBase {

 protected:
    Commit_t* commitHeader;

 public:
    ZrtpPacketCommit();		 /* Creates a Commit packet with default data */
    ZrtpPacketCommit(uint8_t* data); /* Creates a Commit packet from received data */
    virtual ~ZrtpPacketCommit();

    uint8_t* getHashType()    { return commitHeader->hash; };
    uint8_t* getCipherType()  { return commitHeader->cipher; };
    uint8_t* getPubKeysType() { return commitHeader->pubkey; };
    uint8_t* getSasType()     { return commitHeader->sas; };
    uint8_t* getZid()         { return commitHeader->zid; };
    uint8_t* getHvi()         { return commitHeader->hvi; };

    void setHashType(uint8_t* text)    { memcpy(commitHeader->hash, text, 8); };
    void setCipherType(uint8_t* text)  { memcpy(commitHeader->cipher, text, 8); };
    void setPubKeyType(uint8_t* text)  { memcpy(commitHeader->pubkey, text, 8); };
    void setSasType(uint8_t* text)     { memcpy(commitHeader->sas, text, 8); };
    void setZid(uint8_t* text)         { memcpy(commitHeader->zid, text, 12); };
    void setHvi(uint8_t* text)         { memcpy(commitHeader->hvi, text, 32); };
 private:
};

#endif // ZRTPPACKETCOMMIT

