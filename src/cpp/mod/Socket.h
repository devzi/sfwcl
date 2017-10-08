#pragma once
#ifndef SOCKET_H
#define SOCKET_H
#include "Shared.h"
#ifdef USE_SDK
#include <IGameFramework.h>
#include <ISystem.h>
#include <IScriptSystem.h>
#include <IConsole.h>
#include <ILevelSystem.h>
#include <I3DEngine.h>
#include <IRenderer.h>
#include <IRendererD3D9.h>
#include <Windows.h>
#include "NetworkStuff.h"
#pragma region SocketDefs
class Socket : public CScriptableBase {
public:
	Socket(ISystem*,IGameFramework*);
	~Socket();
	int socket(IFunctionHandler *pH,bool tcp=true);
	int connect(IFunctionHandler *pH,int sock,const char *host,int port);
	int close(IFunctionHandler *pH,int sock);
	int send(IFunctionHandler *pH,int sock,const char *buffer,int len);
	int recv(IFunctionHandler *pH,int sock,int len);
	int error(IFunctionHandler *pH);
	//int ClearLayer(IFunctionHandler* pH,int layer);
protected:
	void RegisterMethods();
	ISystem				*m_pSystem;
	IScriptSystem		*m_pSS;
	IGameFramework		*m_pGameFW;
};
extern IConsole *pConsole;
extern IGameFramework *pGameFramework;
extern ISystem *pSystem;
#pragma endregion

#endif
#endif