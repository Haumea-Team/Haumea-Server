// SPDX-License-Identifier: GPL-2.0-or-later
// Once a day, in the background: is there a newer haumea-server release?
#pragma once
#include <string>

void UpdateCheckStart(void);	// asks GitHub now, without waiting for the answer
// The newer release's tag once the answer came, "" when there's none (or no answer).
std::string UpdateCheckNewer(void);
bool UpdateCheckDone(void);
