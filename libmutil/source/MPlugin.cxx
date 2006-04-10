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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Copyright (C) 2004-2006
 *
 * Authors: Erik Eliasson <eliasson@it.kth.se>
 *          Johan Bilien <jobi@via.ecp.fr>
 *          Mikael Magnusson <mikma@users.sourceforge.net>
*/


#include<config.h>

#include<libmutil/MPlugin.h>
#include<libmutil/Library.h>

// Use ltdl symbols defined in ltdl.c
#define LT_SCOPE extern
#include<ltdl.h>

using namespace std;

MRef<MPluginManager*> MPluginManager::instance;

MPlugin::MPlugin(MRef<Library*> lib): library( lib ){
}

MPlugin::MPlugin(): library( NULL ){
}

MPlugin::~MPlugin(){
}

string MPlugin::getMemObjectType(){
	return "MPlugin";
}

MPluginManager::MPluginManager(){
}

MPluginManager::~MPluginManager(){
	libraries.clear();
	registries.clear();
}

MRef<MPluginManager*> MPluginManager::getInstance(){
	if( !instance ){
		instance = new MPluginManager();
	}

	return instance;
}

list<string> * MPluginManager::getListFromLibrary( MRef<Library *> lib ){
	MPlugin::lister listerFunction;

	listerFunction = (MPlugin::lister)lib->getFunctionPtr( "listPlugins" );

	if( listerFunction ){
		return listerFunction( lib );
	}

	return NULL;
}

int32_t MPluginManager::loadFromFile( const std::string &filename ){
	int32_t nPlugins = 0;
	list<string> * entryPoints;
	MRef<Library *> lib;

	lib = Library::open( filename );

	if( !lib ){
		merr << "MPluginManager: Can't load " << filename << end;
		// Continue;
		return -1;
	}

	entryPoints = getListFromLibrary( lib );

	if( entryPoints ){
		list<string>::iterator iEP;
		for( iEP = entryPoints->begin(); 
		     iEP!= entryPoints->end();
		     iEP ++ ){
			MRef<MPlugin *> p;

			p = loadFromLibrary( lib, *iEP );
			if( p ){
				if( registerPlugin( p ) ){
					nPlugins ++;
				}
			}
			else {
				merr << "MPluginManager: No plugin for ep: " << *iEP << end;
			}
		}
	}
	else{
		merr << "MPluginManager: No entrypoints in " << filename << end;
	}

	if( nPlugins > 0 ){
		libraries.push_back( lib );
	}
	else {
		mdbg << "MPluginManager: No plugins loaded from " << lib->getPath() << end;
	}
	return nPlugins;
}

struct ltdl_info {
		MRef<MPluginManager *> manager;
		int32_t nTotalPlugins;
};

static int ltdl_callback(const char *filename, lt_ptr data){
	ltdl_info *info = (ltdl_info*)data;
	int32_t nPlugins;

	nPlugins = info->manager->loadFromFile( filename );

	if( nPlugins > 0 ){
		info->nTotalPlugins += nPlugins;
	}

	// Continue;
	return 0;
}

int32_t MPluginManager::loadFromDirectory( const string &path ){
	ltdl_info info;

	memset(&info, 0, sizeof(info));
	info.manager = this;

	int res = lt_dlforeachfile(path.c_str(), ltdl_callback, &info);
	if( res < 0 ){
		merr << lt_dlerror() << end;
	}
	return info.nTotalPlugins;
}

MRef<MPlugin *> MPluginManager::loadFromLibrary( const string &file, 
						 const string &entryPoint ){
	MRef<Library *> lib = NULL;
	list< MRef<Library *> >::iterator iLib;
	MRef<MPlugin *> p;
	bool newLib = false;

	for( iLib = libraries.begin(); iLib != libraries.end(); iLib ++ ){
		if( (*iLib)->getPath() == file ){
			lib = *iLib;
		}
	}

	/* This library hasn't been opened yet */
	if( !lib ){
		lib = Library::open( file );
		if( lib ){
			newLib = true;
		}
	}

	p =  loadFromLibrary( lib, entryPoint );

	if( p && newLib ){
		libraries.push_back( lib );
	}

	return p;
}


MRef<MPlugin *> MPluginManager::loadFromLibrary( MRef<Library *> lib, 
					  const string &entryPoint ){
	MPlugin::creator creatorFunction;

	if( !lib ){
		/* This library doen't exist or couldn't be opened */
		return NULL;
	}

	creatorFunction = (MPlugin::creator)(lib->Library::getFunctionPtr(
				entryPoint ));

	if( creatorFunction ){
		MRef<MPlugin *> * pp = creatorFunction( lib );
		fprintf( stderr, "pp: %x\n", (unsigned int)pp );
		fprintf( stderr, "getName: %s\n", (**pp)->getName().c_str() );
		fprintf( stderr, "getDescription: %s\n", (**pp)->getDescription().c_str() );
		return *pp;
	}

	return NULL;
}

bool MPluginManager::registerPlugin( MRef<MPlugin *> p ){
	std::list< MPluginRegistry * >::iterator iReg;


	for( iReg = registries.begin(); iReg != registries.end(); iReg ++ ){
		if( (*iReg)->getPluginType() == p->getPluginType() ){
			(*iReg)->registerPlugin( p );
			return true;
		}
	}

	merr << "MPluginManager: Can't find registry for " << p->getPluginType() << endl;
	return false;
}

void MPluginManager::addRegistry( MPluginRegistry * registry ){
	registries.push_back( registry );
}

void MPluginManager::removeRegistry( MPluginRegistry * registry ){
	registries.remove( registry );
}

void MPluginRegistry::registerPlugin( MRef<MPlugin *> p ){
	plugins.push_back( p );
}

MPluginRegistry::MPluginRegistry(){
	manager = MPluginManager::getInstance();
	manager->addRegistry( this );
}

MPluginRegistry::~MPluginRegistry(){
	plugins.clear();
	manager->removeRegistry( this );
}

MRef<MPlugin*> MPluginRegistry::findPlugin( std::string name ){
	list< MRef<MPlugin *> >::iterator iter;
	list< MRef<MPlugin *> >::iterator last = plugins.end();

	for( iter = plugins.begin(); iter != last; iter++ ){
		MRef<MPlugin *> plugin = *iter;

		if( plugin->getName() == name ){
			return plugin;
		}
	}

	return NULL;
}
