/*#include <MsXml2.h>
#include <ExDisp.h>	//InternetExplorer!*/
#include "Shared.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sstream>

#define CLIENT_BUILD 1000

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef USE_SDK
	typedef unsigned int uint;
	#include <IGameFramework.h>
	#include <ISystem.h>
	#include <IScriptSystem.h>
	#include <IConsole.h>
	#include <IFont.h>
	#include <IUIDraw.h>
	#include <IFlashPlayer.h>
	#include <WinSock2.h>
	#pragma comment(lib,"ws2_32")
	#include "CPPAPI.h"
	#include "Socket.h"
	#include "Structs.h"
	CPPAPI *luaApi=0;
	Socket *socketApi=0;
	ISystem *pSystem=0;
	IConsole *pConsole=0;
	IGame *pGame=0;
	IScriptSystem *pScriptSystem=0;
	IGameFramework *pGameFramework=0;
	IFlashPlayer *pFlashPlayer=0;
#endif

#define getField(type,base,offset) (*(type*)(((unsigned char*)base)+offset))

typedef void (__fastcall *PFNLOGIN)(void*,void*,const char*);
typedef void (__fastcall *PFNSHLS)(void*,void*);
typedef void (__fastcall *PFNJS)(void*,void*);
typedef bool (__fastcall *PFNGSS)(void*,void*,SServerInfo&);
typedef void (__fastcall *PFNDE)(void*,void*,EDisconnectionCause, bool,const char*);
typedef void (__fastcall *PFNSE)(void*,void*,const char*,int);


int GAME_VER=6156;

PFNLOGIN pLoginCave=0;
PFNLOGIN pLoginSuccess=0;
PFNSHLS	pShowLoginScreen=0;
PFNJS pJoinServer=0;
PFNGSS pGetSelectedServer=0;
PFNDE pDisconnectError=0;
PFNSE pShowError=0;

void *m_ui;

char SvMaster[255]="openspy.org";

void ToggleLoading(const char *text,bool loading=true);

#ifdef USE_SDK
void CommandClMaster(IConsoleCmdArgs *pArgs){
	if (pArgs->GetArgCount()>1)
	{
		const char *to=pArgs->GetCommandLine()+strlen(pArgs->GetArg(0))+1;
		strcpy(SvMaster, to);
	}
	pScriptSystem->BeginCall("printf");
	char buff[50];
	sprintf(buff,"$0    cl_master = $6%s",SvMaster);
	pScriptSystem->PushFuncParam("%s");
	pScriptSystem->PushFuncParam(buff);
	pScriptSystem->EndCall();

	//ToggleLoading("Setting master",true);
}
void CommandRldMaps(IConsoleCmdArgs *pArgs){
	ILevelSystem *pLevelSystem = pGameFramework->GetILevelSystem();
	if(pLevelSystem){
		pLevelSystem->Rescan();
	}
}
#endif

void MemScan(void *base,int size){
	char buffer[8192]="";
	for(int i=0;i<size;i++){
		if(i%16==0) sprintf(buffer,"%s %#04X: ",buffer,i);
		sprintf(buffer,"%s %02X",buffer,((char*)base)[i]&0xFF);
		if(i%16==15) sprintf(buffer,"%s\n",buffer);
	}
	MessageBoxA(0,buffer,0,0);
}

/*
	DEPRECATED:
void __fastcall ShowError(void *self,void *addr,const char *msg,int code){
	if(code==eDC_MapNotFound || code==eDC_MapVersion){
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("finddownload");
		pScriptSystem->PushFuncParam(msg);
		pScriptSystem->EndCall();
	}
	MessageBoxA(0,msg,0,0);
	unhook(pShowError);
	pShowError(self,addr,msg,code);
	hook((void*)pShowError,(void*)ShowError);
}
void __fastcall OnLoginFailed(void *self,void *addr,const char *reason){
#ifdef USE_SDK
	pScriptSystem->BeginCall("OnLogin");
	pScriptSystem->EndCall();
#endif
	pLoginSuccess(self,addr,reason);
}
*/
void __fastcall OnShowLoginScr(void *self,void *addr){
#ifdef USE_SDK
	pScriptSystem->BeginCall("OnShowLoginScreen");
//#ifndef IS64
	pScriptSystem->PushFuncParam(true);
//#endif
	pScriptSystem->EndCall();
	pFlashPlayer=(IFlashPlayer*)*(unsigned long*)(self);
	pFlashPlayer->Invoke1("_root.Root.MainMenu.MultiPlayer.MultiPlayer.gotoAndPlay", "internetgame");
#endif
}

void ToggleLoading(const char *text,bool loading){
	if(pFlashPlayer){
		pFlashPlayer->Invoke1("showLOADING", loading);
		if(loading){
			SFlashVarValue args[]={text,false};
			pFlashPlayer->Invoke("setLOADINGText",args,sizeof(args)/sizeof(args[0]));
		}
	}
}

void* __stdcall Hook_GetHostByName(const char* name){
	unhook(gethostbyname);
	hostent *h=0;
	if(strcmp(SvMaster,"gamespy.com")){
		int len=strlen(name);
		char *buff=new char[len+255];
		strcpy(buff,name);
		int a,b,c,d;
		bool isip = sscanf(SvMaster,"%d.%d.%d.%d",&a,&b,&c,&d) == 4;
		if(char *ptr=strstr(buff,"gamespy.com")){
			if(!isip)
				memcpy(ptr,SvMaster,strlen(SvMaster));
		}
		else if(char *ptr=strstr(buff,"gamesspy.eu")){
			if(!isip)
				memcpy(ptr,SvMaster,strlen(SvMaster));
		}
		h=gethostbyname(buff);
		delete [] buff;
	} else {
		h=gethostbyname(name);
	}
	hook(gethostbyname,Hook_GetHostByName);
	return h;
}

bool checkFollowing=false;
bool __fastcall GetSelectedServer(void *self, void *addr, SServerInfo& server){
#ifdef USE_SDK
	m_ui=self;
	unhook(pGetSelectedServer);
	bool result=pGetSelectedServer(self,addr,server);
	hook(pGetSelectedServer,GetSelectedServer);
	if(checkFollowing){
		char ip[30];
		int iIp=server.m_publicIP;
		#ifdef IS64
		#define IPOFFSET 0x30
		#define PORTOFFSET 0x34
		//MemScan(&server,sizeof(server));
		#else
		#define IPOFFSET 0x14
		#define PORTOFFSET 0x18
		#endif
		if(GAME_VER==5767)
			iIp=getField(int,&server,IPOFFSET);
		sprintf(ip,"%d.%d.%d.%d",(iIp)&0xFF,(iIp>>8)&0xFF,(iIp>>16)&0xFF,(iIp>>24)&0xFF);
		ToggleLoading("Checking server",true);
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("CheckSelectedServer");
		pScriptSystem->PushFuncParam(ip);
		if(GAME_VER==6156)
			pScriptSystem->PushFuncParam(server.m_publicPort);
		else pScriptSystem->PushFuncParam((int)(getField(unsigned short,&server,PORTOFFSET)));
		if(GAME_VER==6156)
			pScriptSystem->PushFuncParam("<unknown map>");
		pScriptSystem->EndCall();
	}
	return result;
#endif
}
void __fastcall JoinServer(void *self,void *addr){
#ifdef USE_SDK
	checkFollowing=true;
	unhook((void*)pJoinServer);
	pJoinServer(self,addr);
	hook((void*)pJoinServer,(void*)JoinServer);
	checkFollowing=false;
#endif
}
void __fastcall DisconnectError(void *self, void *addr,EDisconnectionCause dc, bool connecting, const char* serverMsg){
	if(dc==eDC_MapNotFound || dc==eDC_MapVersion){
		IScriptSystem *pScriptSystem=pSystem->GetIScriptSystem();
		pScriptSystem->BeginCall("finddownload");
		pScriptSystem->PushFuncParam(serverMsg);
		pScriptSystem->EndCall();
	}
	unhook(pDisconnectError);
	pDisconnectError(self,addr,dc,connecting,serverMsg);
	hook((void*)pDisconnectError,(void*)DisconnectError);
}
BOOL APIENTRY DllMain(HANDLE,DWORD,LPVOID){
	return TRUE;
}
inline void fillNOP(void *addr,int l){
	DWORD tmp=0;
	VirtualProtect(addr,l*2,PAGE_READWRITE,&tmp);
	memset(addr,'\x90',l);
	VirtualProtect(addr,l*2,tmp,&tmp);
}
#ifdef USE_SDK
int OnImpulse( const EventPhys *pEvent ){
#ifdef WANT_CIRCLEJUMP
	return 1;
#else
	return 0;
#endif
}
#endif

extern "C" {
	__declspec(dllexport) void patchMem(int ver){
		switch(ver){
#ifdef IS64
			case 5767:
				fillNOP((void*)0x3968C719,6);
				fillNOP((void*)0x3968C728,6);
				pShowLoginScreen=(PFNSHLS)0x39308250;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3931F3E0;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x39313C40;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);
				break;
			case 6156:
				fillNOP((void*)0x39689899,6);
				fillNOP((void*)0x396898A8,6);
				pShowLoginScreen=(PFNSHLS)0x393126B0;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3932C090;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x39320D60;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				//pDisconnectError=(PFNDE)0x39315EB0;
				//hook((void*)pDisconnectError,(void*)DisconnectError);
				break;
			case 6729:
				fillNOP((void*)0x3968B0B9,6);
				fillNOP((void*)0x3968B0C8,6);
				break;
#else
			case 5767:
				fillNOP((void*)0x3953F4B7,2);
				fillNOP((void*)0x3953F4C0,2);
				pShowLoginScreen=(PFNSHLS)0x3922A330;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x39234E50;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x3922E650;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				//pLoginSuccess=(PFNLOGIN)0x3922C090;
				//hookp((void*)0x3922C170,(void*)OnLoginFailed,6);

				//pShowError=(PFNSE)0x3922A7F0;
				//hook((void*)pShowError,(void*)ShowError);
				break;
			case 6156:
				fillNOP((void*)0x3953FB7E,2);
				fillNOP((void*)0x3953FB87,2);
				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);

				pJoinServer=(PFNSHLS)0x3923D820;
				hook((void*)pJoinServer,(void*)JoinServer);

				pGetSelectedServer=(PFNGSS)0x3923BB70;
				hook((void*)pGetSelectedServer,(void*)GetSelectedServer);

				pDisconnectError=(PFNDE)0x39232D90;
				hook((void*)pDisconnectError,(void*)DisconnectError);

				//pLoginSuccess=(PFNLOGIN)0x39232B30;
				//hookp((void*)0x39232C10,(void*)OnLoginFailed,6);
				break;
			case 6729:
				fillNOP((void*)0x3953FF89,2);
				fillNOP((void*)0x3953FF92,2);
				
				pShowLoginScreen=(PFNSHLS)0x39230E00;
				hook((void*)pShowLoginScreen,(void*)OnShowLoginScr);
				//pLoginSuccess=(PFNLOGIN)0x392423A0;
				//hookp((void*)0x3923F8C0,(void*)OnLoginFailed,6);
				break;
#endif
		}
	}
	__declspec(dllexport) void* CreateGame(void* ptr){

		bool needsUpdate = false;
		std::string newestVersion = fastDownload(
			(
				std::string("http://crymp.net/dl/version.txt?")
				+(std::to_string(time(0)))
			).c_str()
		);
		if(atoi(newestVersion.c_str())!=CLIENT_BUILD && GetLastError()==0){
			//MessageBoxA(0,newestVersion.c_str(),0,MB_OK);
			if(autoUpdateClient()){
				TerminateProcess(GetCurrentProcess(),0);
				::PostQuitMessage(0);
				return 0;
			}
		}
		typedef void* (*PFNCREATEGAME)(void*);
		int version=getGameVer(".\\.\\.\\Bin32\\CryGame.dll");
#ifdef IS64
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin64\\CryGame.dll");
#else
		HMODULE lib=LoadLibraryA(".\\.\\.\\Bin32\\CryGame.dll");
#endif
		PFNCREATEGAME createGame=(PFNCREATEGAME)GetProcAddress(lib,"CreateGame");
		void *pGame=createGame(ptr);
		GAME_VER=version;
		patchMem(version);
		hook(gethostbyname,Hook_GetHostByName);
#if !defined(USE_SDK)
	#ifndef IS64
		void *pSystem=0;
		void *pScriptSystem=0;
		void *tmp=0;
		const char *idx="GAME_VER";
		float ver=version;
		mkcall(pSystem,ptr,64);
		mkcall(pScriptSystem,pSystem,108);
		mkcall2(tmp,pScriptSystem,84,idx,&ver);
	#endif
#else
		pGameFramework=(IGameFramework*)ptr;
		pSystem=pGameFramework->GetISystem();
		pScriptSystem=pSystem->GetIScriptSystem();
		pConsole=pSystem->GetIConsole();
		pConsole->AddCommand("cl_master",CommandClMaster);
		pConsole->AddCommand("reload_maps",CommandRldMaps);
#ifdef WANT_CIRCLEJUMP
		IPhysicalWorld *pPhysicalWorld=pSystem->GetIPhysicalWorld();
		pPhysicalWorld->AddEventClient( 1,OnImpulse,0 );  
#endif
		pScriptSystem->SetGlobalValue("GAME_VER",version);
		if(!luaApi)
			luaApi=new CPPAPI(pSystem,pGameFramework);
		if(!socketApi)
			socketApi=new Socket(pSystem,pGameFramework);
#endif
		return pGame;
	}
}