/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Copyright (C) 2004, 2005
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *          Ignacio Sanchez Pardo <isp@kth.se>
*/

#include<config.h>
#include<iostream>
#include"SoundSource.h"
#include<libmutil/mtime.h>

#include<iostream>

#include<libmutil/itoa.h> //for debug ... remove

#include<libmutil/CircularBuffer.h>

using namespace std;

SoundSource::SoundSource(int id):sourceId(id){
// 	leftch = NULL;
// 	rightch = NULL;
// 	lookupright = NULL;
// 	lookupleft = NULL;
	setSilenced( false );
	position = 0;
}

void SoundSource::setPointer(int32_t wpointer){
	pointer=wpointer;
}

int SoundSource::getId(){
	return sourceId;
}

int32_t SoundSource::getPos(){
	return position;
}

void SoundSource::setPos(int32_t position){
	this->position=position;
}

BasicSoundSource::BasicSoundSource(int32_t id,
				SoundIOPLCInterface *plc,
				int32_t position,
				uint32_t oFreq,
				uint32_t oDurationMs,
				uint32_t oNChannels):
		SoundSource(id),
		plcProvider(plc)   {
	this->oNChannels = oNChannels;
        
	this->position=position;

	oFrames = ( oDurationMs * oFreq ) / 1000;
	iFrames = ( oDurationMs * 8000 ) / 1000;
	
	resampler = Resampler::create( 8000, oFreq, oDurationMs, oNChannels );

	temp = new short[iFrames * oNChannels];
	
	//FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
	//With this implementation, we keep losing audio ... 
	//  For every 1000 round of getSound(), pushSound() only does 999, 
	//  causing a delay for the audio on the headphones from which we never
	//  recover. 
	//HACK Set a small buffer size ... kind of limiting the max delay.
	//	This limits the delay, but we loose audio frames all along ...
	//         For now, we do 20ms * 5 = 100ms
	//	We can set this even smaller ... but then we may have problems
	//	if rtp packets come in burst  ... 
	cbuff = new CircularBuffer( iFrames * oNChannels * 5 );

	/* spatial audio initialization */
	leftch = new short[1028];
	rightch = new short[1028];
	pointer = 0;
	j=0;
	k=0;
//	lookupleft = new short[65536];
//	lookupright = new short[65536];

#ifdef DEBUG_OUTPUT
	cerr << "BasicSoundSource::  - new with id(ssrc) = " << itoa(id) << endl;
/*	printf( "BasicSoundSource:: buffer size = %d, bufferSizeMonoSamples = %d, oFrames = %d, iFrames = %d, oDurationMs = %d, oNChannels = %d\n",
		bufferSizeInMonoSamples*oNChannels, bufferSizeInMonoSamples, oFrames, iFrames, oDurationMs, this->oNChannels );*/
#endif
}

BasicSoundSource::~BasicSoundSource(){
	delete [] temp;
// 	delete [] stereoBuffer;
	delete cbuff;
}

#ifdef DEBUG_OUTPUT
bool nprint=false;
int npush=1;
int nget=1;
#endif

void BasicSoundSource::pushSound(short * samples,
				int32_t nMonoSamples,
				int32_t index,
				bool isStereo)
{
#ifdef DEBUG_OUTPUT
	npush++;
	if (npush%1000 == 0) {
		printf( "pushSound: nget=%d; npush=%d; diff=%d\n", nget, npush, nget - npush) ;
// 		printf( "CircBuff_push: maxSize = %d; size=%d; free=%d\n", cbuff->getMaxSize(), cbuff->getSize(), cbuff->getFree() );
		//cerr << "Calling pushSound for source " << getId() << endl;
	}
#endif
        
	bufferLock.lock();
	//Check for OverFlow ... this happens if we receive big burst of packets ...
	//or we are not emptying the buffer quick enough ...
	if( cbuff->getFree() < nMonoSamples * (int)oNChannels ) {
	#ifdef DEBUG_OUTPUT
		printf("OF");
// 		cerr << "BasicSoundSource::pushSound - Buffer overflow - dropping packet"<<endl;
	#endif
		bufferLock.unlock();
		return;
	}

	//If the incoming samples are already stereo, as is our circular buffer,
	//just copy them to the buffer.
	//Otherwise, transform them from mono to stereo (copy them twice ... ).
	int writeRet;
	if( isStereo ) {
		writeRet = cbuff->write( samples, nMonoSamples * 2 );
	} else {
		int tempVal;
		memset( temp, 0, iFrames * oNChannels ); 
		for( int32_t i = 0; i<nMonoSamples; i++ ) {
			tempVal = i*oNChannels;
			temp[ tempVal ] = samples[i];
			tempVal ++;
			temp[ tempVal ] = samples[i];
		}
		writeRet = cbuff->write( temp, nMonoSamples * 2 );
	}
	bufferLock.unlock();
#ifdef DEBUG_OUTPUT
	if( writeRet == false ) {
		cerr << "BasicSoundSource::pushSound - Buffer write error"<<endl;
	}
#endif	

}


void BasicSoundSource::getSound(short *dest,
                bool dequeue)
{
#ifdef DEBUG_OUTPUT
	nget++;
	if (nget%1000==0) {
		printf( "getSound: nget=%d; npush=%d; diff=%d\n", nget, npush, nget - npush) ;
// 		printf( "CircBuff_get: maxSize = %d; size=%d; free=%d\n", cbuff->getMaxSize(), cbuff->getSize(), cbuff->getFree() );
		//cerr << "nget="<< nget<< endl;
		//cerr << "Calling getSound for source " << getId() << endl;
	}
#endif
	
	bufferLock.lock();
        
	//Check for underflow ...
	//	if it is so, use the PLC to fill in the missing audio, or produce silence
	//NOTE Underflow is not so bad ... for example, if using a peer like minisip, it will
	//	only send 1 packet/second when we are not the main call ... as long as we keep
	//	receiving 1 pack/set instead of 1pack/20ms ... we get underflow.
	if( cbuff->getSize() < (int) (iFrames*oNChannels) ) {
	#ifdef DEBUG_OUTPUT
		printf("UF");
	#endif
		if (plcProvider){
		#ifdef DEBUG_OUTPUT
			cerr << "PLC!"<< endl;
		#endif			
			short *b = plcProvider->get_plc_sound(oFrames);
			memcpy(dest, b, oFrames);
		}else{
			for (uint32_t i=0; i < oFrames * oNChannels; i++){
				dest[i]=0;
			}
		}
		bufferLock.unlock();
                return;
	}
	
	//If there is no underflow, take the data from the circular buffer and 
	//put it in the temp buffer, where it will be resampled
	memset( temp, 0, iFrames * oNChannels ); 
	bool retRead;
	if( ! isSilenced() ) {
		retRead = cbuff->read( temp, iFrames*oNChannels );
	} else {
		retRead = cbuff->remove( iFrames*oNChannels );
	}
#ifdef DEBUG_OUTPUT
	if( !retRead ) {
		cerr << "BasicSoundSource::pushSound - Buffer read error"<<endl;
	}
#endif
	resampler->resample( temp, dest );
	
	bufferLock.unlock();
	
}

