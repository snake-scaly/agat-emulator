#include <windows.h>
#include <stdio.h>

static const char*srv_states[]={
	NULL,
	"SERVICE_STOPPED",
	"SERVICE_START_PENDING",
	"SERVICE_STOP_PENDING",
	"SERVICE_RUNNING",
	"SERVICE_CONTINUE_PENDING",
	"SERVICE_PAUSE_PENDING",
	"SERVICE_PAUSED"
};

#define SERVICE_START_MODE SERVICE_AUTO_START

static const char*win_errors[]={
	"No error",
	"Invalid function",
	"File not found",
	"Path not found",
	"Too many open files",
	"Access denied",
	"Invalid handle",
	"Arena trashed",
	"Not enough memory",
	"Invalid block",

	"Bad environment",
	"Bad format",
	"Invalid access",
	"Invalid data",
	"Out of memory",
	"Invalid drive",
	"Current directory",
	"Not same device",
	"No more files",
	"Write protect",

	"Bad unit",
	"Not ready",
	"Bad command",
	"CRC failed",
	"Bad length",
	"Seek failed",
	"Not DOS disk",
	"Sector not found",
	"Out of paper",
	"Write fault",

	"Read fault",
	"General failure",
	"Sharing violation",
	"Lock violation",
	"Wrong disk",
	NULL,
	"Sharing buffer exceeded",
	NULL,
	"End of file",
	"Disk is full",
	
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	"Not supported request",
	"Cannot find network path",
	"Duplicate name",
	"Bad network path",
	"Network is busy",
	"Network device not exist",
	"Too many network commands",
	"Network adapter hardware error",
	"Bad server response",
	"Unexpected network error",

	"Bad remote adapter",
	"Print queue is full",
	"No spooler space",
	"Print cancelled",
	"Network name deleted",
	"Network access denied",
	"Bad network device type",
	"Bad network name",
	"Too many network names",
	"Too many network sessions",
};


const char*get_win_err(int code)
{
	static char buf[32];
	if (code < 0 || code >= sizeof(win_errors) / sizeof(win_errors[0]) || !win_errors[code]) {
		wsprintf(buf, "code %i", code);
		return buf;
	}
	return win_errors[code];
}



int start_service(LPCTSTR svcname)
{
	SC_HANDLE hsc, h;
	BOOL b = FALSE;
	SERVICE_STATUS st;

	printf("Starting %s service...", svcname);
	hsc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hsc) {
		puts("OpenSCManager failed!");
		goto finish3;
	}	
	h = OpenService(hsc, svcname, SERVICE_START | SERVICE_QUERY_STATUS);
	if (!h) {
		puts("OpenService failed!");
		goto finish2;
	}

retry:
	b = QueryServiceStatus(h, &st);
	if (!b) goto finish;

	if (st.dwCurrentState == SERVICE_RUNNING) {
		puts("Service running.");
		goto finish;
	}	
	if (st.dwCurrentState == SERVICE_START_PENDING) {
		puts("Service starting...");
		Sleep(1000);
		goto retry;
	}
	if (st.dwCurrentState != SERVICE_STOPPED) {
		printf("Service state: %i (%s).\n", st.dwCurrentState,
			srv_states[st.dwCurrentState]);
	}

	b = StartService(h, 0, NULL);
	if (!b) {
		printf("(%s) ", get_win_err(GetLastError()));
	}

finish:
	CloseServiceHandle(h);
finish2:
	CloseServiceHandle(hsc);
finish3:
	if (b) puts("done"); else puts("failed");
	return b ? 0 : -1;
}


int stop_service(LPCTSTR svcname)
{
	SC_HANDLE hsc, h;
	BOOL b = FALSE;
	SERVICE_STATUS st;
	int ntry = 40;

	printf("Stopping %s service...", svcname);

	hsc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hsc) goto finish3;
	h = OpenService(hsc, svcname, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (!h) goto finish2;

retry:
	b = QueryServiceStatus(h, &st);
	if (!b) goto finish;

	if (st.dwCurrentState == SERVICE_STOPPED) goto finish;
	if (st.dwCurrentState == SERVICE_STOP_PENDING) goto finish;
	if (st.dwCurrentState == SERVICE_START_PENDING) {
		Sleep(1000);
		goto retry;
	}

	b = ControlService(h, SERVICE_CONTROL_STOP, &st);
	if (!b) goto finish;
	if (st.dwWin32ExitCode != NO_ERROR) { b = FALSE; goto finish; }

	for (; ntry; --ntry) {
		b = QueryServiceStatus(h, &st);
		if (!b) goto finish;
	
		if (st.dwCurrentState == SERVICE_STOPPED) goto finish;
		Sleep(100);
	}
	b = FALSE;

finish:
	CloseServiceHandle(h);
finish2:
	CloseServiceHandle(hsc);
finish3:
	if (b) puts("done"); else puts("failed");
	return b ? 0 : -1;
}



int install_driver_service(LPCTSTR svcname, LPCTSTR svcuname, LPCTSTR svcdesc, LPCTSTR drvname)
{
	SC_HANDLE hsc, h;
	TCHAR buf[MAX_PATH];
	BOOL b = FALSE;
	SERVICE_DESCRIPTION desc = {(LPTSTR)svcdesc};


	hsc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
	if (!hsc) goto finish3;

	wsprintf(buf, TEXT("%%SystemRoot%%\\System32\\drivers\\%s"), drvname);
	h = CreateService(hsc, svcname, svcuname, SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER, SERVICE_START_MODE, SERVICE_ERROR_NORMAL,
		buf, NULL, NULL, NULL, NULL, NULL);
	if (!h) goto finish2;

	b = ChangeServiceConfig2(h, 1, &desc);
	if (!b) goto finish;

finish:
	if (!b) DeleteService(h);
	CloseServiceHandle(h);
finish2:
	CloseServiceHandle(hsc);
finish3:
	return b ? 0 : -1;
}


int uninstall_driver_service(LPCTSTR svcname)
{
	SC_HANDLE hsc, h;
	BOOL b = FALSE;


	hsc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hsc) goto finish3;

	h = OpenService(hsc, svcname, DELETE );
	if (!h) goto finish2;

	b = DeleteService(h);
	if (!b) goto finish;
finish:
	CloseServiceHandle(h);
finish2:
	CloseServiceHandle(hsc);
finish3:
	return b ? 0 : -1;
}


int main(int argc, const char * argv[])
{
	int r;
	if (argc < 2) {
		printf("use %s /i | /u\n", argv[0]);
		return 1;
	}
	if (!_stricmp(argv[1], "/i")) {
		puts("Installing GiveIO driver...");
		r = install_driver_service(TEXT("giveio"), TEXT("Give I/O"), TEXT("Gives Direct I/O access for user application"), TEXT("GIVEIO.SYS"));
		if (r) { puts("Service installation failed"); return 2; }
		puts("Starting GiveIO driver...");
		r = start_service(TEXT("giveio"));
		if (r) { puts("Service start failed"); uninstall_driver_service(TEXT("giveio")); return 3; }
		puts("Done.");
		return 0;
	}
	if (!_stricmp(argv[1], "/u")) {
		puts("Uninstalling GiveIO driver...");
		r = stop_service(TEXT("giveio"));
		r = uninstall_driver_service(TEXT("giveio"));
		puts("Done.");
		return 0;
	}
	puts("Unknown command");
	return 5;
}
