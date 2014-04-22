/* Bluefish HTML Editor
 * win32utils.c - non-portable win32 support functions
 *
 * Copyright (C) 2014 Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WIN32

#include "bluefish.h"
#include "win32utils.h"

gboolean
isadmin_win32()
{
	gboolean bRet;
	PACL pACL;
	DWORD dwLen, dwStatus;
	PSID psidAdmin;
	HANDLE hTok, hItok;
	PRIVILEGE_SET ps;
	PSECURITY_DESCRIPTOR psdAdmin;

	GENERIC_MAPPING gmap = {ACCESS_READ, ACCESS_WRITE, 0, ACCESS_READ | ACCESS_WRITE};
	SID_IDENTIFIER_AUTHORITY SSidAuth = {SECURITY_NT_AUTHORITY};

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &hTok)) {
		if (GetLastError() != ERROR_NO_TOKEN)
			return FALSE;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &hTok))
			return FALSE;
	}

	if (!DuplicateToken(hTok, SecurityImpersonation, &hItok))
		return FALSE;

	if (!AllocateAndInitializeSid(&SSidAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin))
		return FALSE;

	if (!(psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH)))
		return FALSE;

	if (!InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
		return FALSE;

	dwLen = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdmin) - sizeof(DWORD);
	if (!(pACL = (PACL)LocalAlloc(LPTR, dwLen)))
		return FALSE;

	if (!InitializeAcl(pACL, dwLen, ACL_REVISION2))
		return FALSE;

	if (!AddAccessAllowedAce(pACL, ACL_REVISION2, ACCESS_READ | ACCESS_WRITE, psidAdmin))
		return FALSE;

	if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
		return FALSE;

	SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
	SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

	if (!IsValidSecurityDescriptor(psdAdmin))
		return FALSE;

	dwLen = sizeof(PRIVILEGE_SET);
	if (!AccessCheck(psdAdmin, hItok, ACCESS_READ, &gmap, &ps, &dwLen, &dwStatus, &(BOOL)bRet)) {
		bRet = FALSE;
		g_print("ERROR: %lu\n", GetLastError());
	}

	if (pACL) LocalFree(pACL);
 	if (psdAdmin) LocalFree(psdAdmin);
	if (psidAdmin) FreeSid(psidAdmin);
	if (hItok) CloseHandle(hItok);
	if (hTok) CloseHandle(hTok);

	return bRet;
}

gint
getauthlevel_win32()
{
	DWORD dwSize;
	HKEY regchk;
	gchar buffer[128];
	if (isadmin_win32()) {
		RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Bluefish", 0, KEY_READ, &regchk);
		dwSize = sizeof(buffer);
		RegQueryValueEx(regchk, "package", NULL, NULL, buffer, &dwSize);
		RegCloseKey(regchk);
		if (!g_strcmp0(PACKAGE, buffer))
			return AUTH_CLASSIC;
		else
			return AUTH_ADMIN;
	}
	else
		return AUTH_USER;
}

gboolean
pythoncheck_win32()
{
	HKEY hPython;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython) == ERROR_SUCCESS) {
		RegCloseKey(hPython);
		return TRUE;
	} else
		if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath", 0, KEY_QUERY_VALUE, &hPython) == ERROR_SUCCESS) {
		RegCloseKey(hPython);
		return TRUE;
	} else if (g_file_test("python.exe", G_FILE_TEST_EXISTS)) {
		// For a possible bundled copy of Python for portable installations
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif
