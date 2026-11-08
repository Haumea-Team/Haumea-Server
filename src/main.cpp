// SPDX-License-Identifier: GPL-2.0-or-later
// haumea-server: Haumea's multiplayer server, on a machine that stays up (the
// one your SA-MP or open.mp server runs on, say). Editors join it from File >
// Multiplayer > Join.
//
// With a state folder (--state-dir) it has accounts, invites, roles and rooms,
// and signs every handshake with its own identity; without one it keeps a
// single session behind one password, as an editor hosting does.
//
// Settings come from, in order: the built-in defaults, a .env file (the one
// in the working folder, or --env FILE), HAUMEA_SERVER_* environment variables
// and the command line. Logging follows open.mp's: log.txt, timestamps, and
// [Info]/[Warning]/[Error] before each line.
#include "mpprotocol.h"	// first: winsock2.h before windows.h
#include "mprelay.h"
#include "mpaccounts.h"
#include "update.h"
#include "version.h"

#include <random>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

static volatile sig_atomic_t gStop, gStopped;
static FILE *gLogFile;
static std::string gTimestampFormat = "[%Y-%m-%dT%H:%M:%S%z]";	// open.mp's logging.timestamp_format
static bool gUseTimestamp = true;

static void
onSignal(int)
{
	gStop = 1;
}

#ifdef _WIN32
// Ctrl+C, closing the window, logging off, shutting down: Windows ends the
// process when this returns, so it waits for the session to be saved.
static BOOL WINAPI
onConsole(DWORD)
{
	gStop = 1;
	while(!gStopped)
		Sleep(50);
	return TRUE;
}
#endif

// "[Info] ..." lines from the relay; ours get one too.
static void
printLine(const char *line)
{
	char stamp[64] = "";
	if(gUseTimestamp){
		time_t t = time(nullptr);
		struct tm local;
#ifdef _WIN32
		localtime_s(&local, &t);
#else
		localtime_r(&t, &local);
#endif
		strftime(stamp, sizeof(stamp), gTimestampFormat.c_str(), &local);
	}
	const char *prefix = line[0] == '[' ? "" : "[Info] ";
	printf("%s%s%s%s\n", stamp, stamp[0] ? " " : "", prefix, line);
	fflush(stdout);
	if(gLogFile){
		fprintf(gLogFile, "%s%s%s%s\n", stamp, stamp[0] ? " " : "", prefix, line);
		fflush(gLogFile);
	}
}

static void
usage(FILE *out)
{
	fprintf(out,
		"Usage: haumea-server [options]\n"
		"       haumea-server --state-dir DIR COMMAND ...\n"
		"\n"
		"Haumea's multiplayer server. Editors join it from File > Multiplayer > Join.\n"
		"\n"
		"Options (each also a HAUMEA_SERVER_<NAME> variable, as in --state-dir: HAUMEA_SERVER_STATE_DIR;\n"
		"a .env file in this folder, or --env FILE, can set them):\n"
		"  --state-dir DIR   accounts, invites, roles and rooms, kept in DIR (rooms in DIR/rooms)\n"
		"  --port N          TCP port to listen on (default %d; open it in the firewall)\n"
		"  --bind ADDRESS    listen on one address only, IPv4 or IPv6 (default: all of both)\n"
		"  --name TEXT       the server's name in the editors (default \"Haumea server\")\n"
		"  --max-clients N   editors at once (default 32)\n"
		"  --maps-dir DIR    keep the map as Pawn whenever someone saves it: DIR/<room>/<name>.pwn,\n"
		"                    and a dated copy of each in .../history\n"
		"  --map-out FILE    also write it to FILE (your game server's gamemodes folder, say)\n"
		"  --map-out-room R  with rooms, the room --map-out is for (default main)\n"
		"  --autosave MIN    ask an editor for the map every MIN minutes it changed (default 10; 0: off)\n"
		"  --log-file FILE   the log, as open.mp's log.txt (default DIR/log.txt with --state-dir; \"\" none)\n"
		"  --no-log-chat     leave chat out of the log\n"
		"  --no-log-connections  leave who comes and goes out of the log\n"
		"  --timestamp-format F  strftime format of the log's timestamps (default [%%Y-%%m-%%dT%%H:%%M:%%S%%z])\n"
		"  --env FILE        read settings from FILE (KEY=VALUE lines)\n"
		"  --no-update-check don't ask GitHub once a day whether a newer release is out\n"
		"  --version         this build's version\n"
		"\n"
		"Without --state-dir: one session behind one password, as an editor hosting has it:\n"
		"  --password TEXT   what joining needs; without one a random password is made and printed\n"
		"  --state FILE      keep the session in FILE, so it survives restarts\n"
		"  --admin-password TEXT  only who gives it may load another map for everyone\n"
		"  --no-replace      nobody may\n"
		"\n"
		"Commands, on the state folder (while the server runs too: it takes them in):\n"
		"  invite [--role R] [--uses N] [--days N]   a code for someone to make an account with\n"
		"                    (R: viewer, builder, admin or owner; default builder, one use, 7 days)\n"
		"  users             the accounts and their roles\n"
		"  role NAME R       give an account a role everywhere\n"
		"  passwd NAME       set an account's password (read from the terminal)\n"
		"  remove NAME       delete an account\n"
		"  fingerprint       this server's identity, as editors show it\n"
		"  --help            this text\n",
		MP_DEFAULT_PORT_NUMBER);
}

// KEY=VALUE lines into the environment, without replacing what's already set.
static bool
readEnvFile(const char *path, bool required)
{
	FILE *f = fopen(path, "r");
	if(f == nullptr){
		if(required)
			fprintf(stderr, "haumea-server: can't read %s\n", path);
		return !required;
	}
	char line[2048];
	while(fgets(line, sizeof(line), f)){
		std::string s = line;
		while(!s.empty() && (s.back() == '\n' || s.back() == '\r'))
			s.pop_back();
		size_t start = s.find_first_not_of(" \t");
		if(start == std::string::npos || s[start] == '#')
			continue;
		s = s.substr(start);
		if(s.compare(0, 7, "export ") == 0)
			s = s.substr(7);
		size_t eq = s.find('=');
		if(eq == std::string::npos)
			continue;
		std::string key = s.substr(0, eq), value = s.substr(eq + 1);
		while(!key.empty() && key.back() == ' ')
			key.pop_back();
		if(value.size() >= 2 && (value[0] == '"' || value[0] == '\'') && value.back() == value[0])
			value = value.substr(1, value.size() - 2);
		if(getenv(key.c_str()) == nullptr){
#ifdef _WIN32
			_putenv_s(key.c_str(), value.c_str());
#else
			setenv(key.c_str(), value.c_str(), 0);
#endif
		}
	}
	fclose(f);
	return true;
}

static const char *
env(const char *name)
{
	std::string key = std::string("HAUMEA_SERVER_") + name;
	const char *value = getenv(key.c_str());
	return value && value[0] ? value : nullptr;
}

static bool
envFlag(const char *name, bool fallback)
{
	const char *value = env(name);
	if(value == nullptr)
		return fallback;
	return !(strcmp(value, "0") == 0 || strcmp(value, "false") == 0 || strcmp(value, "no") == 0 || strcmp(value, "off") == 0);
}

static std::string
readSecret(const char *prompt)
{
	fprintf(stderr, "%s", prompt);
	fflush(stderr);
	char line[256];
	if(!fgets(line, sizeof(line), stdin))
		return "";
	std::string s = line;
	while(!s.empty() && (s.back() == '\n' || s.back() == '\r'))
		s.pop_back();
	return s;
}

// haumea-server --state-dir DIR <command>
static int
command(const std::string &stateDir, int argc, char **argv, int at)
{
	if(stateDir.empty()){
		fprintf(stderr, "haumea-server: %s needs --state-dir (or HAUMEA_SERVER_STATE_DIR)\n", argv[at]);
		return 2;
	}
	MpAccounts accounts;
	std::string error;
	if(!accounts.open(stateDir, &error)){
		fprintf(stderr, "haumea-server: %s\n", error.c_str());
		return 1;
	}
	std::string what = argv[at];
	auto arg = [&](int i) -> std::string { return at + i < argc ? argv[at + i] : ""; };
	if(what == "invite"){
		int role = MP_ROLE_BUILDER, uses = 1, days = 7;
		for(int i = 1; at + i < argc; i++){
			std::string option = arg(i);
			if(option == "--role" && !arg(i + 1).empty() && (role = MpParseRole(arg(++i))) == MP_ROLE_NONE){
				fprintf(stderr, "haumea-server: a role is viewer, builder, admin or owner\n");
				return 2;
			}else if(option == "--uses")
				uses = atoi(arg(++i).c_str());
			else if(option == "--days")
				days = atoi(arg(++i).c_str());
		}
		std::string code = accounts.invite(role, uses, days, "the server");
		printf("Invite for %s %s (%d use%s, %d day%s): %s\n", role == MP_ROLE_ADMIN || role == MP_ROLE_OWNER ? "an" : "a", MpRoleName(role), uses, uses == 1 ? "" : "s", days,
			days == 1 ? "" : "s", code.c_str());
		printf("In Haumea: File > Multiplayer > Join, Invite code, then a name and a password of their own.\n");
		return 0;
	}
	if(what == "users"){
		for(const MpAccount &a : accounts.all())
			printf("%-24s %s\n", a.name.c_str(), MpRoleName(a.role));
		if(accounts.all().empty())
			printf("No accounts yet: haumea-server --state-dir %s invite --role owner\n", stateDir.c_str());
		return 0;
	}
	if(what == "role"){
		int role = MpParseRole(arg(2));
		if(role == MP_ROLE_NONE || !accounts.setRole(arg(1), role)){
			fprintf(stderr, "haumea-server: role NAME viewer|builder|admin|owner, for an account that exists\n");
			return 2;
		}
		printf("%s is now a %s.\n", arg(1).c_str(), MpRoleName(role));
		return 0;
	}
	if(what == "passwd"){
		if(accounts.find(arg(1)) == nullptr){
			fprintf(stderr, "haumea-server: no account called %s\n", arg(1).c_str());
			return 2;
		}
		std::string password = readSecret("New password: ");
		if(password.size() < 8){
			fprintf(stderr, "haumea-server: a password is 8 characters or more\n");
			return 2;
		}
		accounts.setPassword(arg(1), password);
		printf("Done.\n");
		return 0;
	}
	if(what == "remove"){
		if(!accounts.remove(arg(1))){
			fprintf(stderr, "haumea-server: no account called %s\n", arg(1).c_str());
			return 2;
		}
		printf("Removed %s.\n", arg(1).c_str());
		return 0;
	}
	if(what == "fingerprint"){
		printf("%s\n", MpFingerprint(accounts.identityPublic()).c_str());
		return 0;
	}
	fprintf(stderr, "haumea-server: unknown command %s\n\n", what.c_str());
	usage(stderr);
	return 2;
}

int
main(int argc, char **argv)
{
	// A .env first: the one given, else this folder's.
	bool envGiven = false;
	for(int i = 1; i + 1 < argc; i++)
		if(strcmp(argv[i], "--env") == 0){
			if(!readEnvFile(argv[i + 1], true))
				return 2;
			envGiven = true;
		}
	if(!envGiven)
		readEnvFile(".env", false);

	MpRelayOptions options;
	options.port = MP_DEFAULT_PORT_NUMBER;
	options.log = printLine;
	double autosaveMinutes = 10.0;
	std::string logFile;
	bool logFileSet = false;
	std::string bindAddress;
	if(const char *v = getenv("HAUMEA_SERVER_PASSWORD")) options.password = v;
	if(const char *v = getenv("HAUMEA_SERVER_ADMIN_PASSWORD")) options.adminPassword = v;
	if(const char *v = env("PORT")) options.port = atoi(v);
	if(const char *v = env("BIND")) bindAddress = v;
	if(const char *v = env("STATE")) options.statePath = v;
	if(const char *v = env("STATE_DIR")) options.stateDir = v;
	if(const char *v = env("NAME")) options.name = v;
	if(const char *v = env("MAX_CLIENTS")) options.maxClients = atoi(v);
	if(const char *v = env("MAPS_DIR")) options.mapsDir = v;
	if(const char *v = env("MAP_OUT")) options.mapOut = v;
	if(const char *v = env("MAP_OUT_ROOM")) options.mapOutRoom = v;
	if(const char *v = env("AUTOSAVE")) autosaveMinutes = atof(v);
	if(const char *v = getenv("HAUMEA_SERVER_LOG_FILE")){ logFile = v; logFileSet = true; }
	if(const char *v = env("TIMESTAMP_FORMAT")) gTimestampFormat = v;
	options.logChat = envFlag("LOG_CHAT", true);
	options.logConnections = envFlag("LOG_CONNECTIONS", true);
	gUseTimestamp = envFlag("USE_TIMESTAMP", true);
	options.allowReplace = !envFlag("NO_REPLACE", false);
	bool updateCheck = envFlag("UPDATE_CHECK", true);

	for(int i = 1; i < argc; i++){
		const char *arg = argv[i];
		const char *value = i + 1 < argc ? argv[i + 1] : nullptr;
		auto need = [&](void) -> const char* {
			if(value == nullptr){
				fprintf(stderr, "haumea-server: %s needs a value\n", arg);
				exit(2);
			}
			i++;
			return value;
		};
		if(arg[0] != '-')
			return command(options.stateDir, argc, argv, i);
		if(strcmp(arg, "--version") == 0){
			printf("haumea-server %s (multiplayer protocol %u)\n", HAUMEA_SERVER_VERSION, MP_PROTOCOL);
			return 0;
		}
		if(strcmp(arg, "--no-update-check") == 0){
			updateCheck = false;
			continue;
		}
		if(strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0){
			usage(stdout);
			return 0;
		}else if(strcmp(arg, "--env") == 0)
			need();
		else if(strcmp(arg, "--port") == 0)
			options.port = atoi(need());
		else if(strcmp(arg, "--bind") == 0)
			bindAddress = need();
		else if(strcmp(arg, "--password") == 0)
			options.password = need();
		else if(strcmp(arg, "--state") == 0)
			options.statePath = need();
		else if(strcmp(arg, "--state-dir") == 0)
			options.stateDir = need();
		else if(strcmp(arg, "--name") == 0)
			options.name = need();
		else if(strcmp(arg, "--max-clients") == 0)
			options.maxClients = atoi(need());
		else if(strcmp(arg, "--maps-dir") == 0)
			options.mapsDir = need();
		else if(strcmp(arg, "--map-out") == 0)
			options.mapOut = need();
		else if(strcmp(arg, "--map-out-room") == 0)
			options.mapOutRoom = need();
		else if(strcmp(arg, "--autosave") == 0)
			autosaveMinutes = atof(need());
		else if(strcmp(arg, "--admin-password") == 0)
			options.adminPassword = need();
		else if(strcmp(arg, "--no-replace") == 0)
			options.allowReplace = false;
		else if(strcmp(arg, "--log-file") == 0){
			logFile = need();
			logFileSet = true;
		}else if(strcmp(arg, "--no-log-chat") == 0)
			options.logChat = false;
		else if(strcmp(arg, "--no-log-connections") == 0)
			options.logConnections = false;
		else if(strcmp(arg, "--timestamp-format") == 0)
			gTimestampFormat = need();
		else{
			fprintf(stderr, "haumea-server: unknown option %s\n\n", arg);
			usage(stderr);
			return 2;
		}
	}
	options.bindAddress = bindAddress.c_str();
	if(options.port < 0 || options.port > 65535 || options.maxClients < 1 || options.maxClients > 1000){
		fprintf(stderr, "haumea-server: port must be 0-65535 and max-clients 1-1000\n");
		return 2;
	}
	if(autosaveMinutes < 0.0 || autosaveMinutes > 24 * 60){
		fprintf(stderr, "haumea-server: --autosave takes 0 to 1440 minutes\n");
		return 2;
	}
	options.autosaveSeconds = autosaveMinutes * 60.0;
	if(!logFileSet && !options.stateDir.empty())
		logFile = options.stateDir + "/log.txt";
	if(!logFile.empty()){
		if(!options.stateDir.empty()){
			// The folder it goes in may not be there yet.
			MpAccounts folder;
			std::string ignored;
			folder.open(options.stateDir, &ignored);
		}
		gLogFile = fopen(logFile.c_str(), "a");
		if(gLogFile == nullptr)
			fprintf(stderr, "haumea-server: can't write the log %s\n", logFile.c_str());
	}
	bool generated = false;
	if(options.stateDir.empty()){
		if(options.password.size() >= MP_PASSWORD_LEN){
			fprintf(stderr, "haumea-server: the password can be at most %d characters\n", MP_PASSWORD_LEN - 1);
			return 2;
		}
		generated = options.password.empty();
		if(generated){
			static const char letters[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789";
			std::random_device random;
			for(int i = 0; i < 16; i++)
				options.password += letters[random() % (sizeof(letters) - 1)];
		}
	}
	{
		char line[128];
		snprintf(line, sizeof(line), "Starting haumea-server %s (multiplayer protocol %u)", HAUMEA_SERVER_VERSION, MP_PROTOCOL);
		printLine(line);
	}
	std::string error;
	if(!MpRelayStart(options, &error)){
		printLine(("[Error] " + error).c_str());
		return 1;
	}
	if(generated){
		char line[128];
		snprintf(line, sizeof(line), "password: %s (set one with --password)", options.password.c_str());
		printLine(line);
	}
	if(options.stateDir.empty() && options.statePath.empty())
		printLine("[Warning] no --state or --state-dir: the session ends when the server stops");
	if(!options.mapsDir.empty() || !options.mapOut.empty()){
		char line[200];
		if(options.autosaveSeconds > 0.0)
			snprintf(line, sizeof(line), "keeping the map as Pawn when it's saved, and every %g minutes it changed",
				options.autosaveSeconds / 60.0);
		else
			snprintf(line, sizeof(line), "keeping the map as Pawn when it's saved");
		printLine(line);
	}
	signal(SIGINT, onSignal);
	signal(SIGTERM, onSignal);
#ifdef _WIN32
	SetConsoleCtrlHandler(onConsole, TRUE);
#else
	signal(SIGPIPE, SIG_IGN);
#endif
	// A newer release: said in the log once a day, after a first look a minute in.
	time_t nextCheck = updateCheck ? time(nullptr) + 60 : 0;
	bool checking = false;
	while(!gStop){
		MpRelayWait(0.25);
		MpRelayUpdate();
		if(nextCheck && !checking && time(nullptr) >= nextCheck){
			UpdateCheckStart();
			checking = true;
		}
		if(checking && UpdateCheckDone()){
			checking = false;
			nextCheck = time(nullptr) + 24 * 60 * 60;
			std::string newer = UpdateCheckNewer();
			if(!newer.empty()){
				char line[200];
				snprintf(line, sizeof(line), "[Warning] haumea-server %s is out (this is %s): update it, "
					"e.g. with the release's install.sh", newer.c_str(), HAUMEA_SERVER_VERSION);
				printLine(line);
			}
		}
	}
	MpRelayStop();
	if(gLogFile)
		fclose(gLogFile);
	gStopped = 1;
	return 0;
}
