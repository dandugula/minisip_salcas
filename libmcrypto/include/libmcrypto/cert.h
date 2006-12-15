/*
  Copyright (C) 2005, 2004 Erik Eliasson, Johan Bilien
  Copyright (C) 2006 Mikael Magnusson
  
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
 *          Mikael Magnusson <mikma@users.sourceforge.net>
*/


#ifndef MLIBMCRYPTO_CERT_H
#define MLIBMCRYPTO_CERT_H

#include <libmcrypto/config.h>

#include<string>
#include<list>
#include<libmutil/Mutex.h>
#include<libmutil/MemObject.h>
#include<libmutil/Exception.h>

class certificate;

#define CERT_DB_ITEM_TYPE_OTHER  0
#define CERT_DB_ITEM_TYPE_FILE   1
#define CERT_DB_ITEM_TYPE_DIR    2

class LIBMCRYPTO_API ca_db_item{
	public:
		std::string item;
		int type;

		virtual ~ca_db_item();

		bool operator ==(const ca_db_item item2){ return (
				item2.item == item && 
				item2.type == type);};
};


class LIBMCRYPTO_API ca_db: public MObject{
	public:
		virtual ~ca_db();
		static ca_db *create();
		
		virtual void add_directory( std::string dir );
		virtual void add_file( std::string file );
		virtual void add_certificate( certificate * cert );
		virtual std::list<ca_db_item *> &get_items();
		virtual ca_db_item * get_next();
		virtual void init_index();
		virtual void lock();
		virtual void unlock();

		virtual void remove( ca_db_item * removedItem );

	protected:
		ca_db();
		virtual void add_item( ca_db_item* item );
		virtual ca_db_item* create_dir_item( std::string dir );
		virtual ca_db_item* create_file_item( std::string file );
		virtual ca_db_item* create_cert_item( certificate* cert );

	private:
		std::list<ca_db_item *>::iterator items_index;
		std::list<ca_db_item *> items;
                Mutex mLock;
};

class LIBMCRYPTO_API certificate: public MObject{
	public:
		static certificate* load( const std::string cert_filename );
		static certificate* load( const std::string cert_filename,
					  const std::string private_key_filename );
		static certificate* load( unsigned char * der_cert,
					  int length );
		static certificate* load( unsigned char * certData,
					  int length,
					  std::string path );
// 		static certificate *create();

		virtual ~certificate();
		

		virtual int control( ca_db * cert_db )=0;

		virtual int get_der_length()=0;
		virtual void get_der( unsigned char * output,
				      unsigned int * length )=0;

		virtual int envelope_data( unsigned char * data,
					   int size,
					   unsigned char *retdata,
					   int *retsize,
					   unsigned char *enckey,
					   int *enckeylgth,
					   unsigned char** iv)=0;

		virtual int denvelope_data( unsigned char * data,
					    int size,
					    unsigned char *retdata,
					    int *retsize,
					    unsigned char *enckey,
					    int enckeylgth,
					    unsigned char *iv)=0;

		virtual int sign_data( unsigned char * data, int data_length, 
				       unsigned char * sign,
				       int * sign_length )=0;
		virtual int verif_sign( unsigned char * sign, int sign_length,
					unsigned char * data,
					int data_length )=0;

		virtual std::string get_name()=0;
		virtual std::string get_cn()=0;
		virtual std::string get_issuer()=0;
		virtual std::string get_issuer_cn()=0;

		std::string get_file();
		std::string get_pk_file();
                   
		virtual void set_pk( std::string file )=0;
                virtual void set_encpk(char *derEncPk, int length,
				       std::string password,
				       std::string path)=0;

		virtual bool has_pk()=0;

	protected:
 		certificate();

		std::string file;
		std::string pk_file;
};

class LIBMCRYPTO_API certificate_chain: public MObject{
	public:
		static certificate_chain* create();
		virtual ~certificate_chain();
		
		virtual void add_certificate( MRef<certificate *> cert );
// 		virtual void remove_certificate( MRef<certificate *> cert );
		virtual void remove_last();

		virtual int control( MRef<ca_db *> cert_db )=0;
		virtual MRef<certificate *> get_next();
		virtual MRef<certificate *> get_first();

		virtual void clear();

		virtual int length();
		virtual void lock();
		virtual void unlock();

		virtual bool is_empty();

		virtual void init_index();

	protected:
		certificate_chain();
		certificate_chain( MRef<certificate *> cert );

		std::list< MRef<certificate *> > cert_list;
		std::list< MRef<certificate *> >::iterator item;
                Mutex mLock;
};

class LIBMCRYPTO_API certificate_exception : public Exception{
	public:
		certificate_exception( const char *desc):Exception(desc){};
};

class LIBMCRYPTO_API certificate_exception_file : public certificate_exception{
	public:
		certificate_exception_file( const char *message ):certificate_exception(message){};
};

class LIBMCRYPTO_API certificate_exception_init : public certificate_exception{
	public:
		certificate_exception_init( const char *message ):certificate_exception(message){};
};

class LIBMCRYPTO_API certificate_exception_pkey : public certificate_exception{
	public:
		certificate_exception_pkey( const char *message ):certificate_exception(message){};
};

class LIBMCRYPTO_API certificate_exception_chain : public certificate_exception{
	public:
		certificate_exception_chain( const char *message ):certificate_exception(message){};
};

#endif // MLIBMCRYPTO_CERT_H
