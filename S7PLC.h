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

#ifndef SB_WIN_BUILD
#include <curl/curl.h>
#else
#include "win_includes/curl.h"
#endif

#ifdef SB_WIN_BUILD
#include <time.h>
#endif

// C++ includes
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>

#include "../../licensedinterfaces/sberrorx.h"

#include "StopWatch.h"
#include "json.hpp"
using json = nlohmann::json;

#define PLUGIN_VERSION      1.1
#define RESP_BUFFER_SIZE   8192
#define ND_LOG_BUFFER_SIZE 256
#define WAIT_TIME_DOME    1.0 // time to wait before checking if dome is doing something after sending a command
#define WAIT_TIME_SHUTTER 2.0 // time to wait before checking if shutter is doing something after sending a command

#define PLUGIN_DEBUG 2

// S7 error codes
enum S7PLCErrors {PLUGIN_OK=0, NOT_CONNECTED, S7_CANT_CONNECT, S7_BAD_CMD_RESPONSE, COMMAND_FAILED, COMMAND_TIMEOUT};

// S7 dome state
enum S7PLCDomeState {IDLE=0, MOVING};

// S7 shutter state
// "0" close, "1" open, "2" opening, "3" closing.
enum S7PLCShutterState {CLOSED=0, OPEN, OPENING, CLOSING, SHUTTER_ERROR, SHUTTER_UNKNOWN};

class CS7PLC
{
public:
    CS7PLC();
    ~CS7PLC();

    int         Connect();
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; }

    int         getFirmware(std::string &sFirmware);

    // Dome commands
    int syncDome(double dAz, double dEl);
    int parkDome(void);
    int unparkDome(void);
    int goHome(void);

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

    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, void* data);


    void getIpAddress(std::string &IpAddress);
    void setIpAddress(std::string IpAddress);

    void getTcpPort(int &nTcpPort);
    void setTcpPort(int nTcpPort);

protected:

    int             domeCommandGET(std::string sCmd, std::string &sResp);
    int             domeCommandPOST(std::string sCmd, std::string &sResp, std::string sParams);

    std::string     cleanupResponse(const std::string InString, char cSeparator);

    int             getDomeAz(double &domeAz);
    int             getDomeEl(double &domeEl);
    int             getShutterState(int &state);

    bool            isDomeMoving();
    bool            isDomeAtHome();

    int             setAutoMode(bool bEnable);

    bool            checkGotoBoundaries(double dGotoAz, double dDomeAz);

    CURL            *m_Curl;
    std::string     m_sBaseUrl;

    std::string     m_sIpAddress;
    int             m_nTcpPort;

    bool            m_bIsConnected;
    bool            m_bShutterOpened;

    double          m_dCurrentAzPosition;
    double          m_dCurrentElPosition;

    double          m_dGotoAz;

    int             m_nDomeState;
    int             m_nShutterState;
    int             m_nGotoTries;

    CStopWatch      m_domeWaitTimer;
    CStopWatch      m_shutterWaitTimer;


    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);

    
#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
#endif

};

#endif
