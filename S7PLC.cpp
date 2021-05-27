//
//  S7PLC.cpp
//  CS7PLC
//
//  Created by Rodolphe Pineau on 2021-04-30
//  S7PLC X2 plugin

#include "S7PLC.h"

CS7PLC::CS7PLC()
{
    // set some sane values
    m_bIsConnected = false;


    m_dCurrentAzPosition = 0.0;
    m_dCurrentElPosition = 0.0;


    m_bShutterOpened = false;
    m_nShutterState = SHUTTER_UNKNOWN;

    m_nGotoTries = 0;
    m_sBaseUrl.clear();
    m_nTcpPort = 80;
    m_sIpAddress.clear();

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\S7PLCLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/S7PLCLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/S7PLCLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::CS7PLC] New Constructor Called\n", timestamp);
    fflush(Logfile);
#endif

    curl_global_init(CURL_GLOBAL_ALL);
    m_Curl = nullptr;

}

CS7PLC::~CS7PLC()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::~CS7PLC] Destructor Called\n", timestamp );
    fflush(Logfile);
#endif

    curl_global_cleanup();
}

int CS7PLC::Connect()
{
    int nErr = SB_OK;
    std::string sDummy;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::Connect] Called\n", timestamp);
    fflush(Logfile);
#endif

    if(m_sIpAddress.empty())
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::Connect] Base url = %s\n", timestamp, m_sBaseUrl.c_str());
    fflush(Logfile);
#endif

    m_Curl = curl_easy_init();

    if(!m_Curl) {
        m_Curl = nullptr;
        return ERR_CMDFAILED;
    }

    m_bIsConnected = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::Connect] Getting Shutter state.\n", timestamp);
    fflush(Logfile);
#endif
    // get the current shutter state just to check the connection, we don't care about the state for now.
    nErr = getShutterState(m_nShutterState);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::Connect] Error getting Shutter state = %d.\n", timestamp, nErr);
        fflush(Logfile);
#endif
        m_bIsConnected = false;
        return ERR_COMMNOLINK;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::Connect] Shutter state = %d.\n", timestamp, m_nShutterState);
    fflush(Logfile);
#endif

    nErr = getFirmware(sDummy);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::Connect] Firmware = %s.\n", timestamp, sDummy.c_str());
    fflush(Logfile);
#endif

    return SB_OK;
}


void CS7PLC::Disconnect()
{
    m_bIsConnected = false;
    curl_easy_cleanup(m_Curl);
}


int CS7PLC::domeCommandGET(std::string sCmd, std::string &sResp)
{
    int nErr = PLUGIN_OK;
    CURLcode res;
    std::string response_string;
    std::string header_string;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandGET]\n", timestamp);
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandGET] Doing get on  %s\n", timestamp, sCmd.c_str());
    fflush(Logfile);
#endif
    curl_easy_setopt(m_Curl, CURLOPT_URL, (m_sBaseUrl+sCmd).c_str());
    curl_easy_setopt(m_Curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(m_Curl, CURLOPT_HEADERDATA, &header_string);
    curl_easy_setopt(m_Curl, CURLOPT_FAILONERROR, 1);

    // Perform the request, res will get the return code
    res = curl_easy_perform(m_Curl);
    // Check for errors
    if(res != CURLE_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::domeCommandGET] Error = %s.\n", timestamp,  curl_easy_strerror(res));
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandGET] response = %s\n", timestamp,  response_string.c_str());
    fflush(Logfile);
#endif

    sResp.assign(cleanupResponse(response_string,'\n'));

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandGET] sResp = %s\n", timestamp,  sResp.c_str());
    fflush(Logfile);
#endif
    return nErr;
}

int CS7PLC::domeCommandPOST(std::string sCmd, std::string &sResp, std::string sParams)
{
    int nErr = PLUGIN_OK;
    CURLcode res;
    std::string response_string;
    std::string header_string;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST]\n", timestamp);
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST] Doing post on  %s\n", timestamp, sCmd.c_str());
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST] Sending %s\n", timestamp, sParams.c_str());
    fflush(Logfile);
#endif
    curl_easy_setopt(m_Curl, CURLOPT_URL, (m_sBaseUrl+sCmd).c_str());
    curl_easy_setopt(m_Curl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYSTATUS, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, sParams.c_str());
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDSIZE, sParams.size());
    curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(m_Curl, CURLOPT_HEADERDATA, &header_string);
    curl_easy_setopt(m_Curl, CURLOPT_FAILONERROR, 1);

    // Perform the request, res will get the return code
    res = curl_easy_perform(m_Curl);
    // Check for errors
    if(res != CURLE_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST] Error = %s.\n", timestamp,  curl_easy_strerror(res));
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST] response = %s\n", timestamp,  response_string.c_str());
    fflush(Logfile);
#endif

    sResp.assign(cleanupResponse(response_string,'\n'));
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::domeCommandPOST] sResp = %s\n", timestamp,  sResp.c_str());
    fflush(Logfile);
#endif
    return nErr;
}

int CS7PLC::getFirmware(std::string &sFirmware)
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    json jResp;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    sFirmware.clear();
    nErr = domeCommandGET("/awp/version.htm", response_string);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::getDomeStepPerRev] ERROR = %s\n", timestamp, response_string.c_str());
        fflush(Logfile);
#endif
        return nErr;
    }

    // process response_string
    try {
        jResp = json::parse(response_string);
        sFirmware.assign(jResp.at("VERSION").get<std::string>());
    }
    catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::getFirmware] json exception : %s - %d\n", timestamp, e.what(), e.id);
        fflush(Logfile);
#endif
        sFirmware = "Unknown";
        return ERR_CMDFAILED;
    }

    sFirmware.assign(response_string);
    return nErr;
}


int CS7PLC::calibrate()
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sParams;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    sParams="%22CALIBRATE%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/calibrate.htm", response_string, sParams);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::calibrate] ERROR = %s\n", timestamp, response_string.c_str());
        fflush(Logfile);
#endif
        return nErr;
    }

    return nErr;
}

int CS7PLC::isCalibratingComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    std::string response_string;

    bComplete = true;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(isDomeMoving()) {
        bComplete = false;
        return nErr;
    }


    return nErr;

}

int CS7PLC::getDomeAz(double &domeAz)
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    json jResp;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::getDomeAz]\n", timestamp);
    fflush(Logfile);
#endif


    // do http GET request to PLC got get current Az or Ticks .. TBD
    nErr = domeCommandGET("/awp/getAz.htm", response_string);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    // process response_string
    try {
        if(response_string.find("e-")!=-1) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CS7PLC::getDomeAz] response_string contains 'e-'\n", timestamp);
            fflush(Logfile);
#endif
            domeAz = m_dCurrentAzPosition;
            return nErr;
        }
        jResp = json::parse(response_string);
        m_dCurrentAzPosition = jResp.at("Az").get<float>();
        domeAz = m_dCurrentAzPosition;
    }
    catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::getDomeAz] json exception : %s - %d\n", timestamp, e.what(), e.id);
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

    return nErr;
}

int CS7PLC::getDomeEl(double &domeEl)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(!m_bShutterOpened)
    {
        domeEl = 0.0;
        return nErr;
    }

    // convert El string to double
    domeEl = m_dCurrentElPosition;

    return nErr;
}


int CS7PLC::getShutterState(int &state)
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    json jResp;

    m_nShutterState = SHUTTER_UNKNOWN;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::getShutterState]\n", timestamp);
    fflush(Logfile);
#endif

    // just return closed until we implement the hardware.
    m_nShutterState = CLOSED;
    return nErr;


    // do http GET request to PLC got get current the shutter state
    nErr = domeCommandGET("/awp/getShutter.htm", response_string);
    if(nErr) {
        state = SHUTTER_ERROR;
        return ERR_CMDFAILED;
    }

    // process response_string
    try {
        jResp = json::parse(response_string);
        m_nShutterState = jResp.at("SLIT").get<int>();
    }
    catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::getDomeAz] json exception : %s - %d\n", timestamp, e.what(), e.id);
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

    state = m_nShutterState;

    return nErr;
}


int CS7PLC::syncDome(double dAz, double dEl)
{

    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sParams;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    sParams="%22AZIMUTH%22="+std::to_string(dAz);
    // do http post to S7 to set current position ... Az or ticks .. TBD

    nErr = domeCommandPOST("/awp/syncToNewAz.htm", response_string, sParams);
    if(nErr)
        return nErr;

    m_dCurrentAzPosition = dAz;

    return nErr;
}

int CS7PLC::parkDome()
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    return nErr;

}

int CS7PLC::unparkDome()
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    return nErr;
}

int CS7PLC::goHome()
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sPostData;

    // call GOTO
    sPostData = "%GOHOME%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/goHome.htm", response_string, sPostData);

    return nErr;
}

int CS7PLC::gotoAzimuth(double newAz)
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sPostData;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // set target
    sPostData = "%22TARGET%22=" + std::to_string(newAz) + "&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/setTarget.htm ", response_string, sPostData);
    if(nErr)
        return nErr;

    // call GOTO
    sPostData = "%GOTO%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/goto.htm", response_string, sPostData);
    if(nErr)
        return nErr;

    m_dGotoAz = newAz;

    return nErr;
}

int CS7PLC::openShutter()
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sParams;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // send http post to S7 to open the shutter
    sParams="%22OPEN%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/open.htm", response_string, sParams);
    if(nErr)
        return nErr;
    return nErr;
}

int CS7PLC::closeShutter()
{
    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sParams;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // send http post to S7 to open the shutter
    sParams="%22CLOSE%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/close.htm", response_string, sParams);
    if(nErr)
        return nErr;
    return nErr;
}

bool CS7PLC::isDomeMoving()
{
    bool bIsMoving = false;
    int nErr = PLUGIN_OK;
    std::string response_string;
    json jResp;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // do http GET request to PLC got get current the dome rotation state .. TBD
    nErr = domeCommandGET("/awp/getState.htm", response_string);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    // process response_string
    try {
        jResp = json::parse(response_string);
        m_nDomeState = jResp.at("State").get<int>();
    }
    catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::isDomeMoving] json exception : %s - %d\n", timestamp, e.what(), e.id);
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

     switch(m_nDomeState) {
         case IDLE :
             bIsMoving = false;
             break;
         case MOVING :
             bIsMoving = true;
             break;
         default:
             bIsMoving = false;
             break;
     }

    return bIsMoving;
}

bool CS7PLC::isDomeAtHome()
{
    bool bAthome = false;
    int nErr = PLUGIN_OK;
    std::string response_string;
    json jResp;
    int nHomeState;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // do http GET request to PLC got get current the dome home state .. TBD
    nErr = domeCommandGET("/awp/getHome.htm", response_string);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    // process response_string
    try {
        jResp = json::parse(response_string);
        nHomeState = jResp.at("atHome").get<int>();
    }
    catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CS7PLC::isDomeMoving] json exception : %s - %d\n", timestamp, e.what(), e.id);
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }

    if(nHomeState)
        bAthome = true;

    return bAthome;
}


int CS7PLC::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    double dDomeAz = 0;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(isDomeMoving()) {
        bComplete = false;
        getDomeAz(dDomeAz);
        return nErr;
    }

    getDomeAz(dDomeAz);
    if(dDomeAz >0 && dDomeAz<1)
        dDomeAz = 0;

    while(ceil(m_dGotoAz) >= 360)
        m_dGotoAz = ceil(m_dGotoAz) - 360;

    while(ceil(dDomeAz) >= 360)
        dDomeAz = ceil(dDomeAz) - 360;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CS7PLC::isGoToComplete DomeAz = %3.2f\n", timestamp, dDomeAz);
    fflush(Logfile);
#endif

    // we need to test "large" depending on the heading error and movement coasting
    if ((ceil(m_dGotoAz) <= ceil(dDomeAz)+3) && (ceil(m_dGotoAz) >= ceil(dDomeAz)-3)) {
        bComplete = true;
        m_nGotoTries = 0;
    }
    else {
        // we're not moving and we're not at the final destination !!!
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CS7PLC::isGoToComplete ***** ERROR **** domeAz = %3.2f, m_dGotoAz = %3.2f\n", timestamp, dDomeAz, m_dGotoAz);
        fflush(Logfile);
#endif
        if(m_nGotoTries == 0) {
            bComplete = false;
            m_nGotoTries = 1;
            gotoAzimuth(m_dGotoAz);
        }
        else {
            m_nGotoTries = 0;
            nErr = ERR_CMDFAILED;
        }
    }

    return nErr;
}

int CS7PLC::isOpenComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CS7PLC::isOpenComplete] Checking Shutter state\n", timestamp);
	fflush(Logfile);
#endif

    nErr = getShutterState(m_nShutterState);
    if(nErr)
        return ERR_CMDFAILED;

    if(m_nShutterState == OPEN){
        m_bShutterOpened = true;
        bComplete = true;
        m_dCurrentElPosition = 90.0;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CS7PLC::isOpenComplete] Shutter is opened\n", timestamp);
		fflush(Logfile);
#endif
    }
    else {
        m_bShutterOpened = false;
        bComplete = false;
        m_dCurrentElPosition = 0.0;
    }

    return nErr;
}

int CS7PLC::isCloseComplete(bool &complete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CS7PLC::isCloseComplete] Checking Shutter state\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getShutterState(m_nShutterState);
    if(nErr)
        return ERR_CMDFAILED;

    if(m_nShutterState == CLOSED){
        m_bShutterOpened = false;
        complete = true;
        m_dCurrentElPosition = 0.0;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CS7PLC::isCloseComplete] Shutter is closed\n", timestamp);
		fflush(Logfile);
#endif
    }
    else {
        m_bShutterOpened = true;
        complete = false;
        m_dCurrentElPosition = 90.0;
    }

    return nErr;
}


int CS7PLC::isParkComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    bComplete = true;
    return nErr;
}

int CS7PLC::isUnparkComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    bComplete = true;

    return nErr;
}

int CS7PLC::isFindHomeComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    double dDomeAz;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    if(isDomeMoving()) {
        bComplete = false;
        getDomeAz(dDomeAz);
        return nErr;
    }
    else {
        bComplete = true;
    }
    return nErr;

}

int CS7PLC::abortCurrentCommand()
{


    int nErr = PLUGIN_OK;
    std::string response_string;
    std::string sParams;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CS7PLC::abortCurrentCommand]\n", timestamp);
	fflush(Logfile);
#endif

    // do http post to S7 to abort all motion
    sParams="%22ABORT%22=1&%22AUTO%22=1";
    nErr = domeCommandPOST("/awp/abort.htm", response_string, sParams);
    if(nErr)
        return nErr;

    m_nGotoTries = 1;

    return nErr;
}

#pragma mark - Getter / Setter


double CS7PLC::getCurrentAz()
{
    if(m_bIsConnected)
        getDomeAz(m_dCurrentAzPosition);

    return m_dCurrentAzPosition;
}

double CS7PLC::getCurrentEl()
{
    if(m_bIsConnected)
        getDomeEl(m_dCurrentElPosition);

    return m_dCurrentElPosition;
}

int CS7PLC::getCurrentShutterState()
{
    if(m_bIsConnected)
        getShutterState(m_nShutterState);

    return m_nShutterState;
}

#pragma mark - Getter / Setter


size_t CS7PLC::writeFunction(void* ptr, size_t size, size_t nmemb, void* data)
{
    std::string *sData = (std::string *)data;
    sData->append((char*)ptr, size * nmemb);

    return size * nmemb;
}


void CS7PLC::getIpAddress(std::string &IpAddress)
{
    IpAddress = m_sIpAddress;
}

void CS7PLC::setIpAddress(std::string IpAddress)
{
    m_sIpAddress = IpAddress;
    if(m_nTcpPort!=80 && m_nTcpPort!=443) {
        m_sBaseUrl = "http://"+m_sIpAddress+":"+std::to_string(m_nTcpPort);
    }
    else if (m_nTcpPort==443) {
        m_sBaseUrl = "https://"+m_sIpAddress;
    }
    else {
        m_sBaseUrl = "http://"+m_sIpAddress;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::setIpAddress] New base url : %s\n", timestamp, m_sBaseUrl.c_str());
    fflush(Logfile);
#endif

}

void CS7PLC::getTcpPort(int &nTcpPort)
{
    nTcpPort = m_nTcpPort;
}

void CS7PLC::setTcpPort(int nTcpPort)
{
    m_nTcpPort = nTcpPort;
    if(m_nTcpPort!=80) {
        m_sBaseUrl = "http://"+m_sIpAddress+":"+std::to_string(m_nTcpPort);
    }
    else {
        m_sBaseUrl = "http://"+m_sIpAddress;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CS7PLC::setTcpPort] New base url : %s\n", timestamp, m_sBaseUrl.c_str());
    fflush(Logfile);
#endif
}


std::string& CS7PLC::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CS7PLC::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CS7PLC::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}


std::string CS7PLC::cleanupResponse(const std::string InString, char cSeparator)
{
    int nErr = PLUGIN_OK;
    std::string sSegment;
    std::vector<std::string> svFields;

    if(!InString.size()) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CRTIDome::setDefaultDir] InString is empty\n", timestamp);
        fflush(Logfile);
#endif
        return InString;
    }


    std::stringstream ssTmp(InString);

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        if(sSegment.find("<!-") == -1)
            svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
        return std::string("");
    }

    sSegment.clear();
    for( std::string s : svFields)
        sSegment.append(trim(s,"\n\r "));
    return sSegment;
}


