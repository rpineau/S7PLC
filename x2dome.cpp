
#include "x2dome.h"

X2Dome::X2Dome(const char* pszSelection, 
							 const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface*	pTheSky,
					SleeperInterface*					pSleeper,
					BasicIniUtilInterface*			pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*						pIOMutex,
					TickCountInterface*					pTickCount)
{

    m_nPrivateISIndex                = nISIndex;
    m_pTheSkyX                        = pTheSky;
    m_pSleeper                        = pSleeper;
    m_pIniUtil                        = pIniUtil;
    m_pLogger                        = pLogger;
    m_pIOMutex                        = pIOMutex;
    m_pTickCount                    = pTickCount;

	m_bLinked = false;
    m_S7PLC.setSleeper(pSleeper);

    if (m_pIniUtil)
    {
        char szIpAddress[128];
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_IP, "192.168.0.10", szIpAddress, 128);
        m_S7PLC.setIpAddress(std::string(szIpAddress));
        m_S7PLC.setTcpPort(m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_PORT, 80));
    }
}


X2Dome::~X2Dome()
{
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;

}


int X2Dome::establishLink(void)					
{
    int nErr;

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    nErr = m_S7PLC.Connect();
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

	return nErr;
}

int X2Dome::terminateLink(void)					
{
    X2MutexLocker ml(GetMutex());
    m_S7PLC.Disconnect();
	m_bLinked = false;
	return SB_OK;
}

 bool X2Dome::isLinked(void) const				
{
	return m_bLinked;
}


int X2Dome::queryAbstraction(const char* pszName, void** ppVal)
{
    *ppVal = NULL;

    if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();
    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    return SB_OK;
}

#pragma mark - UI binding

int X2Dome::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    char szTmpBuf[LOG_BUFFER_SIZE];
    std::string sIpAddress;
    int nTcpPort;

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("S7PLC.ui", deviceType(), m_nPrivateISIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    m_S7PLC.getIpAddress(sIpAddress);
    dx->setPropertyString("IPAddress", "text", sIpAddress.c_str());
    m_S7PLC.getTcpPort(nTcpPort);
    snprintf(szTmpBuf, LOG_BUFFER_SIZE, "%d", nTcpPort );
    dx->setPropertyString("tcpPort", "text", szTmpBuf);

    if(m_bLinked) { // we can't change the value for the ip and port if we're connected
        dx->setEnabled("IPAddress", false);
        dx->setEnabled("tcpPort", false);
        dx->setEnabled("pushButton", true);
    }
    else {
        dx->setEnabled("IPAddress", true);
        dx->setEnabled("tcpPort", true);
        dx->setEnabled("pushButton", false);
    }


    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        if(!m_bLinked) {
            // save the values to persistent storage
            dx->propertyString("IPAddress", "text", szTmpBuf, 128);
            nErr |= m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_IP, szTmpBuf);
            m_S7PLC.setIpAddress(std::string(szTmpBuf));

            dx->propertyString("tcpPort", "text", szTmpBuf, 128);
            nErr |= m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_PORT, atoi(szTmpBuf));
            m_S7PLC.setTcpPort( atoi(szTmpBuf));
        }
    }
    return nErr;

}

void X2Dome::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    bool bComplete = false;
    int nErr;
    char szErrorMessage[LOG_BUFFER_SIZE];

    if (!strcmp(pszEvent, "on_pushButtonCancel_clicked") && m_bCalibratingDome)
        m_S7PLC.abortCurrentCommand();

    if (!strcmp(pszEvent, "on_timer"))
    {
        if(m_bLinked) {
            if(m_bCalibratingDome) {
                // are we still calibrating ?
                bComplete = false;
                nErr = m_S7PLC.isCalibratingComplete(bComplete);
                if(nErr) {
                    uiex->setEnabled("pushButtonOK",true);
                    uiex->setEnabled("pushButtonCancel", true);
                    snprintf(szErrorMessage, LOG_BUFFER_SIZE, "Error calibrating dome : Error %d", nErr);
                    uiex->messageBox("S7-1200 Calibrate", szErrorMessage);
                    m_bCalibratingDome = false;
                    return;
                }

                if(!bComplete) {
                    return;
                }

                // enable buttons
                uiex->setEnabled("pushButtonOK",true);
                uiex->setEnabled("pushButtonCancel", true);
                m_bCalibratingDome = false;
                uiex->setText("pushButton", "Calibrate");
            }
        }
    }

    else if (!strcmp(pszEvent, "on_pushButton_clicked"))
    {
        if(m_bLinked) {
            if( m_bCalibratingDome) { // Abort
                // enable buttons
                uiex->setEnabled("pushButtonOK", true);
                uiex->setEnabled("pushButtonCancel", true);
                // stop everything
                m_S7PLC.abortCurrentCommand();
                m_bCalibratingDome = false;
                // set button text the Calibrate
                uiex->setText("pushButton", "Calibrate");
            } else {                                // Calibrate
                // disable buttons
                uiex->setEnabled("pushButtonOK", false);
                uiex->setEnabled("pushButtonCancel", false);
                // change "Calibrate" to "Abort"
                uiex->setText("pushButton", "Abort");
                m_S7PLC.calibrate();
                m_bCalibratingDome = true;
            }
        }
    }

}


//
//HardwareInfoInterface
//
#pragma mark - HardwareInfoInterface

void X2Dome::deviceInfoNameShort(BasicStringInterface& str) const					
{
	str = "S7 1200 PLC";
}

void X2Dome::deviceInfoNameLong(BasicStringInterface& str) const					
{
    str = "S7 1200 PLC";
}

void X2Dome::deviceInfoDetailedDescription(BasicStringInterface& str) const		
{
    str = "S7 1200 PLC";
}

 void X2Dome::deviceInfoFirmwareVersion(BasicStringInterface& str)					
{
    str = "N/A";
     if(m_bLinked) {
         X2MutexLocker ml(GetMutex());
         std::string sFirmware;
         m_S7PLC.getFirmware(sFirmware);
         if(sFirmware.size())
             str = sFirmware.c_str();
     }
}

void X2Dome::deviceInfoModel(BasicStringInterface& str)
{
    str = "S7 1200 PLC";
}

//
//DriverInfoInterface
//
#pragma mark - DriverInfoInterface

 void	X2Dome::driverInfoDetailedInfo(BasicStringInterface& str) const	
{
    str = "S7 1200 PLC X2 plugin by Rodolphe Pineau";
}

double	X2Dome::driverInfoVersion(void) const
{
	return PLUGIN_VERSION;
}

//
//DomeDriverInterface
//
#pragma mark - DomeDriverInterface

int X2Dome::dapiGetAzEl(double* pdAz, double* pdEl)
{

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    *pdAz = m_S7PLC.getCurrentAz();
    *pdEl = m_S7PLC.getCurrentEl();
    return SB_OK;
}

int X2Dome::dapiGotoAzEl(double dAz, double dEl)
{
    int err;


    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.gotoAzimuth(dAz);
    if(err)
        return ERR_CMDFAILED;

    else
        return SB_OK;
}

int X2Dome::dapiAbort(void)
{


    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    m_S7PLC.abortCurrentCommand();
	return SB_OK;
}

int X2Dome::dapiOpen(void)
{
    int err;

    if(!m_bLinked) {
        return ERR_NOLINK;
    }

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.openShutter();
    if(err)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiClose(void)
{
    int err;

    if(!m_bLinked) {
        return ERR_NOLINK;
    }

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.closeShutter();
    if(err)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiPark(void)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.closeShutter();
    if(err)
        return ERR_CMDFAILED;

    err = m_S7PLC.parkDome();
    if(err)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiUnpark(void)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.openShutter();
    if(err)
        return ERR_CMDFAILED;

    err = m_S7PLC.unparkDome();
    if(err)
        return ERR_CMDFAILED;

	return SB_OK;
}

int X2Dome::dapiFindHome(void)
{
    int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = m_S7PLC.goHome();

    return nErr;
}

int X2Dome::dapiIsGotoComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isGoToComplete(*pbComplete);
    if(err)
        return ERR_CMDFAILED;
    return SB_OK;
}

int X2Dome::dapiIsOpenComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isOpenComplete(*pbComplete);
    if(err) {
        return ERR_CMDFAILED;
    }
    return SB_OK;
}

int	X2Dome::dapiIsCloseComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isCloseComplete(*pbComplete);
    if(err) {
        return ERR_CMDFAILED;
    }
    return SB_OK;
}

int X2Dome::dapiIsParkComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isParkComplete(*pbComplete);
    if(err)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiIsUnparkComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isUnparkComplete(*pbComplete);
    if(err)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiIsFindHomeComplete(bool* pbComplete)
{
    int err;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.isFindHomeComplete(*pbComplete);
    if(err)
        return ERR_CMDFAILED;

    return SB_OK;
}

int X2Dome::dapiSync(double dAz, double dEl)
{
    int err;


    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    err = m_S7PLC.syncDome(dAz, dEl);
    if (err)
        return ERR_CMDFAILED;
	return SB_OK;
}

