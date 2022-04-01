/*
  Copyright 2022 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

/** @file
 * @brief Access to Policy Server IP Address, hostname and port number.
 *
 * Provides a simple get/set interface for the policy server variables.
 * Does hostname resolution behind the scenes.
 */

#ifndef CFENGINE_POLICYSERVER_H
#define CFENGINE_POLICYSERVER_H

#include <platform.h>

// GET/SET FUNCTIONS:
void PolicyServerSet(const char *new_policy_server);
const char *PolicyServerGet();
const char *PolicyServerGetIP();
const char *PolicyServerGetHost();
const char *PolicyServerGetPort();

// POLICY SERVER FILE FUNCTIONS:
char* PolicyServerReadFile(const char *workdir);
bool PolicyServerParseFile(const char *workdir, char **host, char **port);
bool PolicyServerLookUpFile(const char *workdir, char **ipaddr, char **port);
bool PolicyServerWriteFile(const char *workdir, const char *new_policy_server);
bool PolicyServerRemoveFile(const char *workdir);

#endif
