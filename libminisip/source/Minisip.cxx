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
#include<config.h>

#ifndef _MSC_VER
#	include<unistd.h>
#	include<iostream>
#endif

#include<libminisip/Minisip.h>

#include<exception>

#include<libmutil/Timestamp.h>
#include<libmutil/TextUI.h>
#include<libmutil/Thread.h>

#ifndef WIN32
#	ifdef DEBUG_OUTPUT
#		include<signal.h>
#		include<libmutil/Timestamp.h>
#	endif
#endif

#include<libmutil/termmanip.h>
#include<libmutil/MessageRouter.h>
#include<libmutil/MPlugin.h>
#include<libmutil/Library.h>

#include<libmnetutil/UDPSocket.h>
#include<libmnetutil/NetworkFunctions.h>
#include<libmnetutil/NetUtil.h>
#include<libmnetutil/NetworkException.h>

#include<libmcrypto/init.h>

#include<libmikey/KeyAgreementDH.h>

#include<libmsip/SipUtils.h>
#include<libmsip/SipCommandString.h>

#include<libminisip/gui/Gui.h>
#include<libminisip/gui/ConsoleDebugger.h>
#include<libminisip/sip/Sip.h>
#include<libminisip/gui/LogEntry.h>
#include<libminisip/contactdb/ContactDb.h>
#include<libminisip/mediahandler/MediaHandler.h>
#include<libminisip/conference/ConferenceControl.h>
#include<libminisip/conference/ConfCallback.h>
#include<libminisip/configbackend/ConfBackend.h>
#include<libminisip/conference/ConfMessageRouter.h>
#include<libminisip/soundcard/SoundDriverRegistry.h>
#include<libminisip/codecs/Codec.h>

#ifdef ZRTP_SUPPORT
#include<libminisip/zrtp/ZrtpHostBridgeMinisip.h>
#endif

#include<stdlib.h>

#ifdef OSSO_SUPPORT
#include<libosso.h>
#endif

using namespace std;

extern Mutex global;

#ifndef WIN32
#ifdef DEBUG_OUTPUT
static void signal_handler( int signal ){
	if( signal == SIGUSR1 ){
		merr << "ERROR: Minisip was stopped (signal SIGUSR1 caught)" << end;
		ts.print();
		exit( 1 );
	} else {
		merr << "ERROR: Minisip was stopped (some signal caught)" << end;
	}
}
#endif
#endif

#ifdef WIN32
# define DIR_SEPARATOR "\\"
# define PATH_SEPARATOR ";"
#else
# define DIR_SEPARATOR "/"
# define PATH_SEPARATOR ":"
#endif

static string buildPluginPath( const string &argv0 ){
	string pluginPath;
	size_t pos = argv0.find_last_of(DIR_SEPARATOR);
	string prefixDir;

	if( pos != string::npos )
		prefixDir = argv0.substr( 0, pos ) + DIR_SEPARATOR + "..";
	else{
		prefixDir = "..";
	}

	pluginPath += prefixDir + DIR_SEPARATOR + "lib" + DIR_SEPARATOR + PACKAGE + DIR_SEPARATOR + "plugins";
	return pluginPath;
}

static void loadPlugins(const string &argv0){
	SoundDriverRegistry::getInstance();
	AudioCodecRegistry::getInstance();
	ConfigRegistry::getInstance();
	MediaRegistry::getInstance();
	ResamplerRegistry::getInstance();

	MRef<MPluginManager *> pluginManager = MPluginManager::getInstance();

	string pluginPath;
	const char *path = getenv( "MINISIP_PLUGIN_PATH" );

	if( !path ){
		if( argv0 != "" )
			pluginPath += buildPluginPath( argv0 );

#ifdef MINISIP_PLUGINDIR
		if( pluginPath.size() > 0 )
			pluginPath += PATH_SEPARATOR;
		pluginPath += MINISIP_PLUGINDIR;
#endif

	}
	else
		pluginPath = path;


	// Load the video plugin first, if available, since it need to
	// initialize the grabber and display registries before those
	// plugins are loaded.
	pluginManager->setSearchPath( pluginPath );
	pluginManager->loadFromFile( "mvideo.la" );
	pluginManager->loadFromDirectory( pluginPath );
}

Minisip::Minisip( MRef<Gui *> gui, int /*argc*/, char **argv ) : gui(gui){

	libmcryptoInit();

	srand((unsigned int)time(0));

	#ifndef WIN32
		#ifdef DEBUG_OUTPUT
		signal( SIGUSR1, signal_handler );
	#endif
	#endif
	

	#ifdef DEBUG_OUTPUT
	mdbg << "Loading plugins"<<end;
	#endif

	loadPlugins( argv ? argv[0] : "" );

	#ifdef DEBUG_OUTPUT
	mout << "Initializing NetUtil"<<end;
	#endif

	if ( ! NetUtil::init()){
		//printf("ERROR: Could not initialize Netutil package\n");
		merr << "ERROR: Could not initialize NetUtil package"<<end;
		exit();
	}

	#ifdef DEBUG_OUTPUT
	cerr << "Creating SipSoftPhoneConfiguration"<< endl;
	#endif
	phoneConf =  new SipSoftPhoneConfiguration();
	//phoneConf->sip=NULL;

	#ifdef MINISIP_AUTOCALL
	if (argc==3){
		phoneConf->autoCall = string(argv[2]);
	}
	#endif

	#ifdef DEBUG_OUTPUT
	mout << BOLD << "init 1/9: Creating contact database" << PLAIN << end;
	#endif

	/* Create the global contacts database */
	ContactDb *contactDb = new ContactDb();
	ContactEntry::setDb(contactDb);
	
	#ifdef OSSO_SUPPORT
		osso_context_t * ossoCtxt = NULL;
		ossoCtxt = osso_initialize( PACKAGE_NAME, PACKAGE_VERSION, TRUE, NULL );
		if( !ossoCtxt ){
			mdbg << "Could not initialize osso context" << end;
		}
	#endif

	#ifdef DEBUG_OUTPUT
		cerr << "Setting contact db"<< endl;
	#endif
	gui->setContactDb(contactDb);
}

Minisip::~Minisip(){
}

int Minisip::exit(){
	int ret = 1;
	mout << BOLD << "Minisip is Shutting down!!!" << PLAIN << end;
	
	if( ! sip.isNull() ) { //it may not be initialized ... 
		//Send a shutdown command to the sip stack ... 
		//it will take care of de-registering and closing on-going calls
		CommandString cmdstr( "", SipCommandString::sip_stack_shutdown );
		SipSMCommand sipcmd(cmdstr, SipSMCommand::dialog_layer, SipSMCommand::dispatcher);
		sip->getSipStack()->handleCommand(sipcmd);
		sip->stop();
	
#ifdef DEBUG_OUTPUT
		mout << "Waiting for the SipStack to close ..." << end;
#endif
		sip->join();
		sip->getSipStack()->setCallback( NULL );
		sip->getSipStack()->setDefaultDialogCommandHandler( NULL );
		sip = NULL;
	}

	gui->setCallback( NULL );
	gui->setSipSoftPhoneConfiguration( NULL );

	stopDebugger();
	
/*	if( messageRouter){
		mout << "Delete messageRouter" << end;
		delete messageRouter;
	}
*/
	
	messageRouter->clear();
	messageRouter=NULL;


	//phoneConf->sip = NULL;
	phoneConf = NULL;
	mediaHandler = NULL;
	confMessageRouter->setGui(NULL);
	confMessageRouter = NULL;
	gui = NULL;

	mout << end << end << BOLD << "Minisip can't wait to see you again! Bye!" << PLAIN << end << end << end;
	return ret;
}

int Minisip::startSip() {
	int ret = 1;

#ifdef DEBUG_OUTPUT
	cerr << "Thread 2 running - doing initParseConfig"<< endl;
#endif	

	if( initParseConfig() < 0 ){
		merr << "Minisip::startSip::initParseConfig - fatal error" << end;
		return -1;
	}

	try{
		messageRouter =  new MessageRouter();
		confMessageRouter =  new ConfMessageRouter();

#ifdef DEBUG_OUTPUT
		mout << BOLD << "init 4/9: Creating IP provider" << PLAIN << end;
#endif
		MRef<IpProvider *> ipProvider = IpProvider::create( phoneConf );
		MRef<IpProvider *> ip6Provider;
		ip6Provider = IpProvider::create( phoneConf, true );
		//#ifdef DEBUG_OUTPUT
		//                mout << BOLD << "init 5/9: Creating SIP transport layer" << PLAIN << end;
		//#endif
		string localIpString;
		string externalContactIP;

		// FIXME: This should be done more often
		localIpString = externalContactIP = ipProvider->getExternalIp();                

		MRef<UDPSocket*> udpSocket = new UDPSocket( phoneConf->sipStackConfig->localUdpPort );

		phoneConf->sipStackConfig->localUdpPort = ipProvider->getExternalPort( udpSocket );
		phoneConf->sipStackConfig->localIpString = externalContactIP;
		phoneConf->sipStackConfig->externalContactIP = externalContactIP;
		if( ip6Provider )
			phoneConf->sipStackConfig->localIp6String = ip6Provider->getExternalIp();
		udpSocket=NULL;

#ifdef DEBUG_OUTPUT
		mout << BOLD << "init 5/9: Creating MediaHandler" << PLAIN << end;
#endif
		mediaHandler = new MediaHandler( phoneConf, ipProvider );
		confMessageRouter->setMediaHandler( mediaHandler );
		messageRouter->addSubsystem("media",*mediaHandler);
		mediaHandler->setMessageRouterCallback(*messageRouter);

		if( consoleDbg ){
			consoleDbg->setMediaHandler( mediaHandler );
		}

		Session::registry = *mediaHandler;
		/* Hack: precompute a KeyAgreementDH */
		//	Session::precomputedKa = new KeyAgreementDH( phoneConf->securityConfig.cert, 
		//                phoneConf->securityConfig.cert_db, DH_GROUP_OAKLEY5 );

#ifdef DEBUG_OUTPUT
		mout << BOLD << "init 6/9: Creating MSip SIP stack" << PLAIN << end;
#endif

		MRef<SipSim*> sim = phoneConf->defaultIdentity->getSim();
		if (sim){
			phoneConf->sipStackConfig->cert = sim->getCertificateChain();
			phoneConf->sipStackConfig->cert_db = sim->getCAs();
		}

		//save Sip object in Minisip::sip ...
		this->sip=new Sip(phoneConf,mediaHandler);
		//sip->init();

//		phoneConf->sip = sip;
		phoneConf->sipStack = sip->getSipStack();

		sip->getSipStack()->setCallback(*messageRouter);
		//sip->getSipStack()->setConfCallback(*confMessageRouter);
		//TODO: Send the callback to the  conference dialog
		//instead.

		//messageRouter->addSubsystem("sip",*sip);
		messageRouter->addSubsystem("sip",*sip->getSipStack());

		confMessageRouter->setSip(sip);
#ifdef ZRTP_SUPPORT
		ZrtpHostBridgeMinisip::initialize(sip->getSipStack()->getTimeoutProvider());
#endif
		/* Load the plugins at this stage */
		//		int32_t pluginCount = MPlugin::loadFromDirectory( PLUGINS_PATH );

		//		cerr << "Loaded " << pluginCount << " plugins from " << PLUGINS_PATH << endl;

#ifdef DEBUG_OUTPUT
		mout << BOLD << "init 7/9: Connecting GUI to SIP logic" << PLAIN << end;
#endif
		gui->setSipSoftPhoneConfiguration(phoneConf);
		//messageRouter->setGui(gui);
		messageRouter->addSubsystem("gui",*gui);
		confMessageRouter->setGui(gui);


		/*
		   mdbg << "Starting presence server"<< end;
		   CommandString subscribeserver("", SipCommandString::start_presence_server);
		   SipSMCommand sipcmdss(subscribeserver, SipSMCommand::remote, SipSMCommand::TU);
		   sip->getSipStack()->handleCommand(sipcmdss);
		   */

		/*
		   cerr << "Minisip: starting presence client for johan@bilien.org"<< endl;

		   CommandString subscribe("", SipCommandString::start_presence_client,"johan@bilien.org");
		   SipSMCommand sipcmd2(subscribe, SipSMCommand::remote, SipSMCommand::TU);
		   sip->getSipStack()->handleCommand(sipcmd2);
		   */

		gui->setCallback(*messageRouter);
		gui->setConfCallback(*confMessageRouter);

		sip->start(); //run as a thread ...
		//		sleep(5);

		//		CommandString pupd("", SipCommandString::remote_presence_update,"someone@ssvl.kth.se","online","Working hard");
		//		gui->handleCommand(pupd);

		//sip->run();
	}

	catch(exception &exc){
		//FIXME: Display message in GUI
		merr << "Minisip caught an exception. Quitting."<< end;
		merr << exc.what() << end;
		ret = -1;
	}
	catch(...){
		//FIXME: Display message in GUI
#ifdef DEBUG_OUTPUT
		merr << "Minisip caught an unknown exception (default). Quitting."<< end;
#endif
		ret = -1;
	};
	return ret;
}

int Minisip::initParseConfig(){

	bool done=false;
	int retGlobal = -1;
	do{
		try{
#ifdef DEBUG_OUTPUT
			mout << BOLD << "init 3/9: Parsing configuration" << PLAIN << end;
#endif
			MRef<ConfBackend *> confBackend = ConfigRegistry::getInstance()->createBackend( gui );
			if( !confBackend ){
				merr << "Minisip could not load a configuration" << end << 
					"back end. The application will now" << end <<
					"exit." << end;
				::exit( 1 );
			}
			string ret = phoneConf->load( confBackend );

			done = true;
			retGlobal = 1; //for now, we finished ok ... check the return string
			if (ret.length()>0){
				if( ret == "ERROR" ) { //severe error
					retGlobal = -1;
				} else { //error, but not severe
					merr << ret << end;
				}
			}
#ifdef DEBUG_OUTPUT
			if( retGlobal > 0 ) {
				cerr << "Identities: "<<endl;
				for (list<MRef<SipIdentity*> >::iterator i=phoneConf->identities.begin(); 
						i!=phoneConf->identities.end(); i++){
					cerr<< "\t"<< (*i)->getDebugString()<< endl;
				}
			}
#endif

		}catch(XMLElementNotFound & enf){
#ifdef DEBUG_OUTPUT
			merr << FG_ERROR << "Element not found: "<< enf.what()<< PLAIN << end;
#endif
			merr << string("ERROR: Could not parse configuration item: ")+enf.what() << end;
			gui->configDialog( phoneConf );
			done=false;
		}
	}while(!done);
	return retGlobal;
}

int Minisip::runGui(){
	gui->run();
	return 1;
}

void Minisip::startDebugger(){
	cerr << "startDebugger" << endl;
	consoleDbg = MRef<ConsoleDebugger*>(new ConsoleDebugger(phoneConf->sipStack));
	MRef<Thread *> consoleDbgThread = consoleDbg->start();
}

void Minisip::stopDebugger(){
	if( ! consoleDbg.isNull() ) {
		mout << end << "Stopping the Console Debugger thread" << end;
		consoleDbg->stop(); //uufff ... we are killing the thread, not nice ...
		consoleDbg->join();
		consoleDbg->setMediaHandler( NULL );
		consoleDbg = NULL;
	}
}

 
