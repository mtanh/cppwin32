#include <WinReg.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // toupper
#include <ctype.h> // toupper
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <algorithm>

#include "logger.h"

#define GET_THREAD_ID() (GetCurrentThreadId())
#define GET_PROCESS_ID() (GetProcessId())
//#define	vsnprintf _vsnprintf

static const int LOG_DATE_BUF_SIZE = 10;
static const int LOG_MESSAGE_OUTPUT_BUFFER_SIZE	= 5000;
static const int MAX_PATH_LEN = 1024;
static const int MAX_TAGS = 1024;
static const char* LOG_FILETYPE = ".log";
static int GlobalLogFileTable[LOG_COUNT];
static int singleLogFile;
static const char *logFileNames[] = {
	"IN",		// Info
	"ER",		// Error
	"WN",		// Warning
	"DB",		// Debug
	"TR"		// Trace
};
static int GlobalLogStatus[] = {
	false,
	false,
	false,
	false,
	true
};
static ULONG GlobalNumStrTags = 0; //number of enabled string tags
static LPCSTR GlobalStrTagPtrs[MAX_TAGS]; // sorted pointers to enabled string tags
static char GlobalStrTags[10240]; // enabled string tags: list of NUL terminated strings ended with a NUL
static ULONG GlobalTagMask1 = 0; // enabled mask tags
static ULONG GlobalTagMask2 = 0;
static ULONG GlobalNumTagBase = 0; // base for numeric tags (tags are in range base+1,...,base+8*MAX_TAGS
static ULONG GlobalNumTagMasks[MAX_TAGS]; // enabled numeric tags (tag used as index into array of bit masks)
static bool initialized = false;
static bool logToFile = false;
static bool useSingleFile = true;
static bool switchFiles = false;
static bool autoPurge = false;
static time_t autoPurgePeriod = 0;
static bool seekToEnd = false;
static bool logProcessId = false;
static bool logMillisecs = false;
static bool logDate = true;
static bool logFile = true;
static char autoPurgeRegExpList[MAX_AUTO_PURGE_REG_EXP_LEN];
static bool logThreadId = false;
static bool writeToConsole = false;

// Number of days' log files to keep.
// 0 means keep everything.
static int nKeep = DEFAULT_NKEEP;
static int nFiles = 0; // Keep track of number of files, for pruning
static time_t* timeStamps = NULL; // timestamps of previous files
static char strLogDir[MAX_PATH_LEN];
static char filenamePrefix[MAX_FILENAME_PREFIX_LEN];
static char	dateBuf[LOG_DATE_BUF_SIZE];

static const int MINLOGSIZE = 1; //1MB is the minimum log file size we are maintaining
static const int SIZE_METRIC = 1024*1024; //Mega Bytes
static const int MAXLOGFILESIZE = 20*SIZE_METRIC; //20MB is the maximum application will write regardless
//of any variable set in the registry
static bool bStopWrite = false;
static DWORD g_maxFileSize = MAXLOGFILESIZE;
static DWORD nCurrLogFileSize = 0;
static time_t nextLogSwitchTime; // Time when to close current log file and open a new one

static CRITICAL_SECTION TZInfoLogCriticalSection ;
static CRITICAL_SECTION LogCriticalSection;
static void OpenLogFiles();
static void CloseLogFiles();
static bool GetRegistryEntry(HKEY hKey,				// e.g. HKEY_LOCAL_MACHINE
							 LPCTSTR pszSubKey,		// e.g. _TEXT("SYSTEM\\CurrentControlSet\\Services\\NDIS\\")
							 LPCTSTR pszValueName,	// e.g. _TEXT("ImagePath")
							 DWORD dwType,			// REG_DWORD or REG_SZ
							 void *pBuf,
							 int nBufSize);


void InitLog(const char* logDir, const char* filenamePrefixA, bool useSingleFileA, bool switchFilesA, int nKeepA)
{
	if(initialized) {
		return;
	}

	InitializeCriticalSection(&LogCriticalSection);
	InitializeCriticalSection(&TZInfoLogCriticalSection);

	char GetTZFromReg[32];
	HKEY theKey;
	bool theResult = false;
	if(RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,			// root key handle
		_TEXT("Software\\DiCentral\\Config"), // sub key name
		0,							// reserved
		KEY_QUERY_VALUE,			// access mask
		&theKey						// result
		) == ERROR_SUCCESS)
	{
		// retrieve the key value
		DWORD theSize;
		theSize = sizeof(GetTZFromReg);

		if(RegQueryValueEx(
			theKey,						// key handle
			_TEXT("VAG_GETTZ_REG"),		// name of value
			NULL,						// reserved
			NULL,						// type
			(unsigned char*)GetTZFromReg,	// buffer
			&theSize					// buffer size
			) == ERROR_SUCCESS)
		{
			theResult = true;
		}
		RegCloseKey(theKey);
	}

	// Making sure that TZ is set to null before calling tzset().
	_putenv( "TZ=" );
	if (stricmp(GetTZFromReg, "true")==0)
	{
		DWORD Bias,ActiveTimeBias, DayLightBias ;
		bool GotInfo = true;

		EnterCriticalSection(&TZInfoLogCriticalSection);
		TIME_ZONE_INFORMATION tzinfo;
		if(GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID) {
			GotInfo = false; //Got invalid timezone info
		}
		LeaveCriticalSection(&TZInfoLogCriticalSection);

		if (GetRegistryEntry(
			HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
			"Bias",
			REG_DWORD,
			&Bias,
			sizeof(Bias))==false) {
				GotInfo = false; //Could Not Get Bias info From the registry
		}

		if(GetRegistryEntry(
			HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
			"ActiveTimeBias",
			REG_DWORD,
			&ActiveTimeBias,
			sizeof(ActiveTimeBias))==false)
		{
			GotInfo = false; //Could Not Get Active Time Bias info From the registry
		}

		if(GetRegistryEntry(
			HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation",
			"DayLightBias",
			REG_DWORD,
			&DayLightBias,
			sizeof(DayLightBias))==false) {
				GotInfo = false; //Could Not Get Daylight Bias Time Zone info From the registry
		}

		EnterCriticalSection(&TZInfoLogCriticalSection);
		if(GotInfo)
		{
			if((DayLightBias != 0) && (ActiveTimeBias != Bias)) {
				_daylight = 1;
			}
			else {
				_daylight = 0;
			}

			_timezone = Bias*60;
			tzinfo.Bias = Bias;
			if(SetTimeZoneInformation( &tzinfo ) == 0) {
				_timezone = Bias*60;
			}
		}
		else {
			_tzset(); //Could Not Get  Time Zone info From the registry using tzset
		}
		LeaveCriticalSection(&TZInfoLogCriticalSection);
	}
	else {
		_tzset();
	}

	memset(&GlobalNumTagMasks[0], 0, sizeof(GlobalNumTagMasks));
	std::

		singleLogFile = -1;
	std::fill(GlobalLogFileTable, GlobalLogFileTable+LOG_COUNT, -1);
	/*for(int i = 0; i < LOG_COUNT; ++i) {
	GlobalLogFileTable[i] = -1;
	}*/

	if(logDir)
	{
		strcpy(strLogDir, (char*)logDir);
		if(filenamePrefixA)
		{
			strncpy(filenamePrefix, filenamePrefixA, sizeof(filenamePrefix));
			filenamePrefix[sizeof(filenamePrefix) - 1] = '\0';
		}
		useSingleFile = useSingleFileA;
		logToFile = true;
		switchFiles = switchFilesA;
		if(nKeep == nKeepA)
		{
			timeStamps = new time_t[nKeep];

			// NOTE: Important to initialize to 0.
			memset(timeStamps, 0, sizeof(*timeStamps) * nKeep);
		}
		else
		{
			timeStamps = new time_t[nKeepA];

			// NOTE: Important to initialize to 0.
			memset(timeStamps, 0, sizeof(*timeStamps) * nKeepA);
		}
		OpenLogFiles();
	}
	else
	{
		logToFile = false;
		switchFiles = false;
		autoPurge = false;
	}

#ifdef _DEBUG
	if (autoPurge && !switchFiles)
	{
		assert(false);
	}
#endif	// _DEBUG

	initialized = true;
}

void InitLogEx(const char* logDir, const char* filenamePrefixA, DWORD optionsA, int nKeepA)
{
	if (initialized)
	{
		return;
	}

	bool useSingleFileLocal;
	bool switchFilesLocal;
	bStopWrite = false;
	DWORD regFileSize = 0;
	bool bRes = false;
	HKEY hKey;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"Software\\DiCentral\\Config",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS)
	{
		// retrieve the key value
		DWORD dwSize = 0;
		DWORD dwType = 0;

		dwType = REG_SZ;
		char tempbuf[32];
		dwSize = sizeof(tempbuf);

		char szLogFileSize[] = "LOGFILESIZE";
		if (RegQueryValueEx(
			hKey,
			szLogFileSize,
			NULL,
			&dwType,
			(unsigned char*)tempbuf,
			&dwSize) == ERROR_SUCCESS)
		{
			regFileSize = atoi(tempbuf);
			bRes = true;
		}
		RegCloseKey(hKey);
	}

	if(bRes)
	{
		if(regFileSize > 0 ) //default value of this key in the registry is greater than 0,if it is present
		{
			if(regFileSize >= MINLOGSIZE) //should be at least Min log size
			{
				g_maxFileSize = regFileSize * SIZE_METRIC;
				//g_maxfilesize is in bytes
				//and the registry value is in Megs
			}
		}
	}
	else {
		g_maxFileSize = MAXLOGFILESIZE;
	}

	useSingleFileLocal = (optionsA & LOG_OPTION_USE_SINGLE_FILE) ? true :false;
	switchFilesLocal = (optionsA & LOG_OPTION_SWITCH_FILES) ? true : false;

	logThreadId = (optionsA & LOG_OPTION_LOG_THREAD_ID) ? true : false;
	writeToConsole = (optionsA & LOG_OPTION_WRITE_TO_CONSOLE) ? true : false;
	autoPurge = (optionsA & LOG_OPTION_AUTO_PURGE) ? true : false;
	seekToEnd = (optionsA & LOG_OPTION_SEEK_TO_END) ? true : false;
	logProcessId = (optionsA & LOG_OPTION_LOG_PROCESS_ID) ? true : false;
	logMillisecs = (optionsA & LOG_OPTION_LOG_MILLISECS) ? true : false;
	logDate = (optionsA & LOG_OPTION_LOG_DATE) ? true : false;
	logFile = (optionsA & LOG_OPTION_LOG_FILENAME) ? true : false;
	autoPurgePeriod = DEFAULT_AUTO_PURGE_PERIOD;

	// Default regular expression is derived from filename prefix
	sprintf(autoPurgeRegExpList, "%s*%s", filenamePrefixA, LOG_FILETYPE);

	// Chain to the original InitLog
	InitLog(logDir, filenamePrefixA, useSingleFileLocal, switchFilesLocal, nKeepA);
}

