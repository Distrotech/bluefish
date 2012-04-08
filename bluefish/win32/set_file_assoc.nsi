;----------------------------------------------
; Bluefish Set File Associations
; [set_file_assoc.nsi]
; 
;  This is just the file association portion
;   of the main install script.
; 
;  Copyright (C) 2009-2012 The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------


; External Defines
;----------------------------------------------
!ifndef PACKAGE
	!define PACKAGE "bluefish"
!endif
;!define LOCALE
!ifndef VERSION
	!define VERSION "2.2"
!endif


; Defines
;----------------------------------------------
!define PRODUCT		"Bluefish"
!define PUBLISHER	"The Bluefish Developers"
!define HOMEPAGE	"null"
!define HELPURL		"null"
!define PROGRAM_EXE	"bluefish.exe"
!define UNINSTALL_EXE	"null"

!define GTK_MIN_VERSION	"null"
!define GTK_URL		"null"
!define GTK_FILENAME 	"null"
!define GTK_SIZE	"null" ; Install size in Kilobytes

!define AS_DICT_URL	"null"

!define REG_USER_SET	"Software\${PRODUCT}"
!define REG_CLASS_SET	"Software\Classes"
!define REG_UNINSTALL	"null"


; Variables
;----------------------------------------------
Var HKEY
Var GTK_STATUS
Var StartMenuFolder

Var FA_Dialog
Var FA_Ada
Var FA_Asp
Var FA_Sh
Var FA_BFProject
Var FA_BFLang2
Var FA_C
Var FA_Cpp
Var FA_Css
Var FA_D
Var FA_Diff
Var FA_Po
Var FA_Html
Var FA_Java
Var FA_Js
Var FA_Jsp
Var FA_Mw
Var FA_Nsi
Var FA_Pl
Var FA_Php
Var FA_Txt
Var FA_Py
Var FA_Rb
Var FA_Smarty
Var FA_Vbs
Var FA_Xhtml
Var FA_Xml
Var FA_SelectAll


; Installer configuration settings
;----------------------------------------------
Name		"${PRODUCT} File Associations"
OutFile		"set_file_assoc.exe"

; Tell Windows Vista and Windows 7 that we want admin rights to install
RequestExecutionLevel admin


; Installer version information
;----------------------------------------------
VIProductVersion "2.2.0.0"
VIAddVersionKey "ProductName" "${PRODUCT}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "FileDescription" "Bluefish Installer"


; Includes
;----------------------------------------------
!include "MUI2.nsh"
!include "WinVer.nsh"
!include "WinMessages.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"
!include "StrFunc.nsh"
${StrLoc}
${StrStr}
${StrTok}
${UnStrLoc}


; MUI configuration
;----------------------------------------------
;!define MUI_LANGDLL_ALLLANGUAGES


; Macros
;----------------------------------------------
!include "includes\Macros.nsh"


; Pages
;----------------------------------------------
Page custom FileAssociations SetFileAssociations


; Locale
;----------------------------------------------
!insertmacro MUI_RESERVEFILE_LANGDLL

; English goes first as the default language
!insertmacro MUI_LANGUAGE	"English" 	; 1033
${LoadLocalization}	"ENGLISH"	"locale\English.nsh"
; Localization Notes
;   MUI_LANGUAGE definitions are defined by NSIS and should not be changed
;    see 'nsisu\Contrib\Language files' for appropriate definitions for missing entries
;   LoadLocalization entries should match the MUI_LANGUAGE name but be in all caps, if they
;    don't match they cannot contain spaces for proper operation
!insertmacro MUI_LANGUAGE	"Basque" 	; 1069
${LoadLocalization}	"BASQUE"	"locale\Basque.nsh"
!insertmacro MUI_LANGUAGE	"Catalan" 	; 1027
${LoadLocalization}	"CATALAN"	"locale\Catalan.nsh"
!insertmacro MUI_LANGUAGE	"PortugueseBR" 	; 1046
${LoadLocalization}	"PORTUGUESEBR"	"locale\PortugueseBR.nsh"
!insertmacro MUI_LANGUAGE	"Czech" 	; 1029
${LoadLocalization}	"CZECH"		"locale\Czech.nsh"
!insertmacro MUI_LANGUAGE	"Dutch" 	; 1043
${LoadLocalization}	"DUTCH" 	"locale\Dutch.nsh"
!insertmacro MUI_LANGUAGE	"French" 	; 1036
${LoadLocalization}	"FRENCH"	"locale\French.nsh"
!insertmacro MUI_LANGUAGE	"Galician" 	; 1110
${LoadLocalization}	"GALICIAN"	"locale\Galician.nsh"
!insertmacro MUI_LANGUAGE	"German" 	; 1031
${LoadLocalization}	"GERMAN"	"locale\German.nsh"
!insertmacro MUI_LANGUAGE	"Hungarian" 	; 1038
${LoadLocalization}	"HUNGARIAN"	"locale\Hungarian.nsh"
!insertmacro MUI_LANGUAGE	"Italian" 	; 1040
${LoadLocalization}	"ITALIAN"	"locale\Italian.nsh"
!insertmacro MUI_LANGUAGE	"Japanese" 	; 1041
${LoadLocalization}	"JAPANESE"	"locale\Japanese.nsh"
!insertmacro MUI_LANGUAGE	"Norwegian" ; 1044
${LoadLocalization}	"NORWEGIAN"	"locale\NorwegianBokmaal.nsh"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"  ; 2068
${LoadLocalization}	"NORWEGIANNYNORSK" "locale\NorwegianNynorsk.nsh"
!insertmacro MUI_LANGUAGE	"Swedish" 	; 1053
${LoadLocalization}	"SWEDISH"	"locale\Swedish.nsh"
!insertmacro MUI_LANGUAGE	"SimpChinese" 	; 2052
${LoadLocalization}	"SIMPCHINESE"	"locale\SimpChinese.nsh"
!insertmacro MUI_LANGUAGE	"Ukrainian" ; 1058
${LoadLocalization}	"UKRAINIAN"	"locale\Ukrainian.nsh"
; Translations needed for the following commented languages
;;!insertmacro MUI_LANGUAGE	"Bulgarian" 	; 1026
;;!insertmacro MUI_LANGUAGE	"Danish" 	; 1030
;;!insertmacro MUI_LANGUAGE	"Finnish" 	; 1035
;;!insertmacro MUI_LANGUAGE	"Greek" 	; 1032
;;!insertmacro MUI_LANGUAGE	"Korean" 	; 1042
;;!insertmacro MUI_LANGUAGE	"Polish" 	; 1045
;;!insertmacro MUI_LANGUAGE	"Portuguese" 	; 2070
;;!insertmacro MUI_LANGUAGE	"Romanian" 	; 1048
;;!insertmacro MUI_LANGUAGE	"Russian" 	; 1049
;;!insertmacro MUI_LANGUAGE	"Serbian" 	; 3098
;;!insertmacro MUI_LANGUAGE	"Slovak" 	; 1051
;;!insertmacro MUI_LANGUAGE	"Spanish" 	; 1034
;;!insertmacro MUI_LANGUAGE	"TradChinese" 	; 1028
;;!insertmacro MUI_LANGUAGE	"Turkish" 	; 1055

; NSIS Lacks a translation for this language
;!insertmacro MUI_LANGUAGE	"Tamil" 	; 1097


; Sections
;----------------------------------------------
Section "$(SECT_BLUEFISH)" SecBluefish
; Placeholder
SectionEnd

; Installer Functions
;----------------------------------------------
!include "includes\Functions.nsh"
Function .onInit
	UserInfo::GetAccountType
	Pop $0
	UserInfo::GetName
	Pop $1
	${If} $1 == ""
	${OrIf} $0 == "Admin"
	${OrIf} $0 == "Power"
		ReadRegStr $R0 HKCU "${REG_USER_SET}" ""
		${If} $R0 != ""
			StrCpy $HKEY "Classic"
		${Else}
			StrCpy $HKEY "HKLM"
		${EndIf}
		SetShellVarContext all
		${If} $HKEY != "Classic"
			ReadRegStr $R0 HKLM "${REG_USER_SET}" "" ; Replace InstallDirRegKey function
		${EndIf}
		${If} $R0 == "" ; If Bluefish hasn't been installed set the default privileged path
			StrCpy $INSTDIR "$PROGRAMFILES32\${PRODUCT}"
		${Else} ; Otherwise load the stored path of the previous installation
			StrCpy $INSTDIR $R0
		${EndIf}
		${If} $HKEY == "Classic"
			ReadRegStr $R0 HKCU "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		${Else}
			ReadRegStr $R0 HKLM "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		${EndIf}
		${If} $R0 == "" ; Bluefish hasn't been installed so display the language selection dialog
			!insertmacro MUI_LANGDLL_DISPLAY
		${Else} ; Else load the stored language
			StrCpy $LANGUAGE $R0
		${EndIf}
		${If} $HKEY == "Classic"
			ReadRegStr $R0 HKCU "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		${Else}
			ReadRegStr $R0 HKLM "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		${EndIf}
		${If} $R0 == "" ; Set default folder
			StrCpy $StartMenuFolder "${PRODUCT}"
		${Else} ; Load stored folder
			StrCpy $StartMenuFolder $R0
		${EndIf}
	${Else}
		StrCpy $HKEY "HKCU"
		SetShellVarContext current
		ReadRegStr $R0 HKCU "${REG_USER_SET}" "" ; Replace InstallDirRegKey function
		${If} $R0 == "" ; If Bluefish hasn't been installed set the default privileged path
			StrCpy $INSTDIR "$PROFILE\Programs\${PRODUCT}"
		${Else} ; Otherwise load the stored path of the previous installation
			StrCpy $INSTDIR $R0
		${EndIf}
		ReadRegStr $R0 HKCU "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		${If} $R0 == "" ; Bluefish hasn't been installed so display the language selection dialog
			!insertmacro MUI_LANGDLL_DISPLAY
		${Else} ; Else load the stored language
			StrCpy $LANGUAGE $R0
		${EndIf}
		ReadRegStr $R0 HKCU "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		${If} $R0 == "" ; Set default folder
			StrCpy $StartMenuFolder "${PRODUCT}"
		${Else} ; Load stored folder
			StrCpy $StartMenuFolder $R0
		${EndIf}
	${EndIf}
FunctionEnd


; Uninstaller Functions
;----------------------------------------------

