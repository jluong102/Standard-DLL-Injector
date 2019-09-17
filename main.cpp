#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
//Compile as console app

class ProcMem //Use class for easy expansions
{
private:
	HANDLE hProc;
	DWORD dwPID;
	HWND hwndProc;

public:
	~ProcMem()
	{
		CloseHandle(hProc);
	}

	template <class type>
	void Write(DWORD dwAddress, type data)
	{
		WriteProcessMemory(hProc, (void*)dwAddress, &data, sizeof(type), NULL);
	}

	template <class type>
	type Read(DWORD dwAddress)
	{
		type data;
		ReadProcessMemory(hProc, (void*)dwAddress, &data, sizeof(type), NULL);
		return data;
	}

	template <class type>
	void Read(DWORD dwAddress, type &data)
	{
		ReadProcessMemory(hProc, (void*)dwAddress, &data, sizeof(type), NULL);
	}

	bool ConnectProc(char* procName)
	{
		const WCHAR* procNameChar;
		int nChars = MultiByteToWideChar(CP_ACP, 0, procName, -1, NULL, 0);
		procNameChar = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, procName, -1, (LPWSTR)procNameChar, nChars);


		HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 ProcEntry;
		ProcEntry.dwSize = sizeof(ProcEntry);

		do
			if (!wcscmp(ProcEntry.szExeFile, procNameChar))
			{
				dwPID = ProcEntry.th32ProcessID;
				CloseHandle(hPID);


				hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
				delete[] procNameChar;
				return true;
			}
		while (Process32Next(hPID, &ProcEntry));

		delete[] procNameChar;
		std::cout << "Process Not Found: " << procName << std::endl;;
		Sleep(500);
		return false;
	}

	bool ConnectWindow(char* winName)
	{
		if (hwndProc == NULL)
			hwndProc = FindWindowA(NULL, winName);

		GetWindowThreadProcessId(hwndProc, &dwPID);
		hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

		if (hwndProc != NULL)
			return true;
		
		Sleep(500);
		std::cout << "Window Not Found: " << winName << std::endl;
		return false;
	}

	bool ConnectWindow(wchar_t* winName)
	{
		if (hwndProc == NULL)
			hwndProc = FindWindow(NULL, winName);

		GetWindowThreadProcessId(hwndProc, &dwPID);
		hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

		if (hwndProc != NULL)
			return true;

		Sleep(500);
		std::cout << "Window Not Found: " << winName << std::endl;
		return false;
	}

	DWORD Module(LPSTR modName)  //Not part of injector, but useful
	{
		const WCHAR *procNameChar;
		int nChars = MultiByteToWideChar(CP_ACP, 0, modName, -1, NULL, 0);
		procNameChar = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, modName, -1, (LPWSTR)procNameChar, nChars);

		HANDLE hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(mEntry);

		do
			if (!wcscmp(mEntry.szModule, procNameChar))
			{
				CloseHandle(hModule);
				return (DWORD)mEntry.modBaseAddr;
				delete[] procNameChar;
			}
		while (Module32Next(hModule, &mEntry));

		delete[] procNameChar;
		std::cout << "Process Platform Invalid: " << modName << std::endl;
		return 0;
	}

	bool Inject(char* dllPath)
	{
		void* GetLib = (void*)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
		void* commitMem = VirtualAllocEx(hProc, NULL, strlen(dllPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		WriteProcessMemory(hProc, commitMem, dllPath, strlen(dllPath), NULL);
		HANDLE hThread = CreateRemoteThread(hProc, NULL, NULL, (LPTHREAD_START_ROUTINE)GetLib, commitMem, 0, NULL);

		if (!hThread)
		{
			std::cout << "Could not create thread" << std::endl;
			return false;
		}

		VirtualFreeEx(hProc, commitMem, strlen(dllPath), MEM_RELEASE);
		CloseHandle(hThread);

		return true;
	}
};


int main()
{
	ProcMem Process;
	while (!Process.ConnectProc((char*)"Discord.exe"));  //Using Discord for example 
	//while (!Process.ConnectWindow((char*)"#general - Discord"));
	//Process.Inject((char*)"D:\\yourpath\\TestDll.dll"); //Change to Path of dll

	system("pause");
	return 0;
}
