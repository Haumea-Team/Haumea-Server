// SPDX-License-Identifier: GPL-2.0-or-later
// Asks GitHub for haumea-server's latest release with curl (on Linux and on
// Windows 10 and later), in a thread of its own so the server never waits.
#include "update.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <pthread.h>
#endif

static volatile int gDone;
static char gNewer[64];

// "v1.2.3" (or "1.2.3", "v1.2.3-4-gabc") as one number; -1 for anything else.
static long long
versionNumber(const char *text)
{
	if(*text == 'v') text++;
	int a = 0, b = 0, c = 0;
	if(sscanf(text, "%d.%d.%d", &a, &b, &c) != 3)
		return -1;
	return (long long)a * 1000000 + b * 1000 + c;
}

static void
check(void)
{
	FILE *p = popen("curl -fsSL --max-time 15 -H \"Accept: application/vnd.github+json\" "
		"https://api.github.com/repos/Haumea-Team/Haumea-Server/releases/latest", "r");
	if(p == nullptr)
		return;
	std::string body;
	char buffer[4096];
	size_t n;
	while((n = fread(buffer, 1, sizeof(buffer), p)) > 0 && body.size() < (1 << 20))
		body.append(buffer, n);
	pclose(p);
	size_t at = body.find("\"tag_name\"");
	if(at == std::string::npos)
		return;
	size_t open = body.find('"', body.find(':', at) + 1);
	size_t close = open == std::string::npos ? open : body.find('"', open + 1);
	if(close == std::string::npos || close - open > 40)
		return;
	std::string tag = body.substr(open + 1, close - open - 1);
	long long latest = versionNumber(tag.c_str()), mine = versionNumber(HAUMEA_SERVER_VERSION);
	if(latest > 0 && (mine < 0 || latest > mine))
		snprintf(gNewer, sizeof(gNewer), "%s", tag.c_str());
}

#ifdef _WIN32
static DWORD WINAPI run(LPVOID) { check(); gDone = 1; return 0; }
#else
static void *run(void *) { check(); gDone = 1; return nullptr; }
#endif

void
UpdateCheckStart(void)
{
	gDone = 0;
	gNewer[0] = '\0';
#ifdef _WIN32
	HANDLE thread = CreateThread(nullptr, 0, run, nullptr, 0, nullptr);
	if(thread) CloseHandle(thread); else gDone = 1;
#else
	pthread_t thread;
	if(pthread_create(&thread, nullptr, run, nullptr) == 0) pthread_detach(thread); else gDone = 1;
#endif
}

bool UpdateCheckDone(void) { return gDone != 0; }
std::string UpdateCheckNewer(void) { return gNewer; }
