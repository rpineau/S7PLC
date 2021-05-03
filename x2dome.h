#include <stdio.h>
#include <string.h>

#include <string>

#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/domedriverinterface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"

#include "S7PLC.h"


class SerXInterface;		
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;


#define PARENT_KEY			"S7PLC"
#define CHILD_KEY_IP    	"IPAddress"
#define CHILD_KEY_PORT      "IPPort"


#define LOG_BUFFER_SIZE 8192
/*!
\brief The X2Dome example.

\ingroup Example

Use this example to write an X2Dome driver.
*/
class X2Dome: public DomeDriverInterface, public ModalSettingsDialogInterface, public X2GUIEventInterface
{
public:

	/*!Standard X2 constructor*/
	X2Dome(	const char* pszSelectionString, 
					const int& nISIndex,
					SerXInterface*						pSerX,
					TheSkyXFacadeForDriversInterface* pTheSkyXForMounts,
					SleeperInterface*				pSleeper,
					BasicIniUtilInterface*			pIniUtil,
					LoggerInterface*					pLogger,
					MutexInterface*					pIOMutex,
					TickCountInterface*				pTickCount);
	virtual ~X2Dome();

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual DeviceType							deviceType(void) {return DriverRootInterface::DT_DOME;}
	virtual int									queryAbstraction(const char* pszName, void** ppVal);
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void)						;
	virtual int									terminateLink(void)						;
	virtual bool								isLinked(void) const					;
	//@} 


    virtual int initModalSettingsDialog(void){return 0;}
    virtual int execModalSettingsDialog(void);


	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void deviceInfoNameShort(BasicStringInterface& str) const					;
	virtual void deviceInfoNameLong(BasicStringInterface& str) const					;
	virtual void deviceInfoDetailedDescription(BasicStringInterface& str) const		;
	virtual void deviceInfoFirmwareVersion(BasicStringInterface& str)					;
	virtual void deviceInfoModel(BasicStringInterface& str)							;
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const	;
	virtual double								driverInfoVersion(void) const								;
	//@} 

	//DomeDriverInterface
	virtual int dapiGetAzEl(double* pdAz, double* pdEl);
	virtual int dapiGotoAzEl(double dAz, double dEl);
	virtual int dapiAbort(void);
	virtual int dapiOpen(void);
	virtual int dapiClose(void);
	virtual int dapiPark(void);
	virtual int dapiUnpark(void);
	virtual int dapiFindHome(void);
	virtual int dapiIsGotoComplete(bool* pbComplete);
	virtual int dapiIsOpenComplete(bool* pbComplete);
	virtual int	dapiIsCloseComplete(bool* pbComplete);
	virtual int dapiIsParkComplete(bool* pbComplete);
	virtual int dapiIsUnparkComplete(bool* pbComplete);
	virtual int dapiIsFindHomeComplete(bool* pbComplete);
	virtual int dapiSync(double dAz, double dEl);

    virtual void uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);


private:

    TheSkyXFacadeForDriversInterface                *    GetTheSkyXFacadeForDrivers() {return m_pTheSkyX;}

	SleeperInterface								*	GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface							*	GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface									*	GetLogger() {return m_pLogger; }
	MutexInterface									*	GetMutex()  {return m_pIOMutex;}
	TickCountInterface								*	GetTickCountInterface() {return m_pTickCount;}

    TheSkyXFacadeForDriversInterface                *   m_pTheSkyX;
	SleeperInterface								*	m_pSleeper;
	BasicIniUtilInterface							*	m_pIniUtil;
	LoggerInterface									*	m_pLogger;
	MutexInterface									*	m_pIOMutex;
	TickCountInterface								*	m_pTickCount;


	int         m_nPrivateISIndex;
	bool        m_bLinked;

    bool        m_bCalibratingDome;

    int         m_nSavedTicksPerRev;
    
    CS7PLC      m_S7PLC;
};
