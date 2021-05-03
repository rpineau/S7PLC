//
//  S7PLC.h
//  CS7PLC
//
//  Created by Rodolphe Pineau on 2021-04-30
//  S7PLC X2 plugin

#ifndef __S7PLC__
#define __S7PLC__
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include <curl/curl.h>

#ifdef SB_WIN_BUILD
#include <time.h>
#endif

#include <string>

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/sleeperinterface.h"

#define DRIVER_VERSION      1.0
#define RESP_BUFFER_SIZE   8192
#define ND_LOG_BUFFER_SIZE 256


#define PLUGIN_DEBUG 2

// erS7 codes
enum S7PLCErS7s {PLUGIN_OK=0, NOT_CONNECTED, S7_CANT_CONNECT, S7_BAD_CMD_RESPONSE, COMMAND_FAILED, COMMAND_TIMEOUT};

// ErS7 code
enum S7PLCShutterState {OPEN=1, OPENING, CLOSED, CLOSING, SHUTTER_ERROR, UNKNOWN};

class CS7PLC
{
public:
    CS7PLC();
    ~CS7PLC();

    int         Connect();
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; }

    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper; };

    int         getFirmware(std::string &sFirmware);

    // Dome commands
    int syncDome(double dAz, double dEl);
    int parkDome(void);
    int unparkDome(void);
    int gotoAzimuth(double newAz);
    int openShutter();
    int closeShutter();

    // command complete functions
    int isGoToComplete(bool &bComplete);
    int isOpenComplete(bool &bComplete);
    int isCloseComplete(bool &bComplete);
    int isParkComplete(bool &bComplete);
    int isUnparkComplete(bool &bComplete);
    int isFindHomeComplete(bool &bComplete);
    int isCalibratingComplete(bool &bComplete);

    int abortCurrentCommand();

    double getCurrentAz();
    double getCurrentEl();

    int getCurrentShutterState();

    int getNbTicksPerRev();
    int setNbTicksPerRev(int nSteps);

    int calibrate();

    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data);


    void getIpAddress(std::string &IpAddress);
    void setIpAddress(std::string IpAddress);

    void getTcpPort(int &nTcpPort);
    void setTcpPort(int nTcpPort);

protected:

    int             domeCommandGET(std::string sCmd, std::string &sResp);
    int             domeCommandPOST(std::string sCmd, std::string &sResp, std::string sParams);

    int             getDomeAz(double &domeAz);
    int             getDomeEl(double &domeEl);
    int             getShutterState(int &state);

    int             getDomeStepPerRev(int &nStepPerRev);
    int             setDomeStepPerRev(int nStepPerRev);

    bool            isDomeMoving();
    bool            isDomeAtHome();

    int             m_nNbStepPerRev;
    
    CURL            *m_Curl;
    std::string     m_sBaseUrl;

    std::string     m_sIpAddress;
    int             m_nTcpPort;

	SleeperInterface    *m_pSleeper;

    bool            m_bIsConnected;
    bool            m_bShutterOpened;

    double          m_dCurrentAzPosition;
    double          m_dCurrentElPosition;

    double          m_dGotoAz;

    int             m_nShutterState;
    int             m_nGotoTries;

#ifdef PLUGIN_DEBUG
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};

#endif
