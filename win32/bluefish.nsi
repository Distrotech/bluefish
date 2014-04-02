;----------------------------------------------
; Bluefish Windows NSIS Install Script
; [bluefish.nsi]
; 
;  Copyright (C) 2009-2014 The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------


; Includes
;----------------------------------------------
!include "MUI2.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"
!include "WinMessages.nsh"
!include "LogicLib.nsh"
!define LOGICLIB_SECTIONCMP
!include "nsDialogs.nsh"
!include "StrFunc.nsh"
${StrLoc}
${StrTok}
${UnStrLoc}
${StrRep}
!include "includes\Checksums.nsh"


; External Defines
;----------------------------------------------
!ifndef PACKAGE
	!define PACKAGE "bluefish"
!endif
;!define LOCALE
!ifndef VERSION
	!define VERSION "2.0-nodef"
!endif
!ifndef BUILD
	!define BUILD "2.24.8"
!endif


; Defines
;----------------------------------------------
!define PRODUCT		"Bluefish"
!define PUBLISHER	"The Bluefish Developers"
!define HOMEPAGE	"http://bluefish.openoffice.nl/"
!define HELPURL		"http://bluefish.openoffice.nl/manual/"
!define PROGRAM_EXE	"${PACKAGE}.exe"
!define UNINSTALL_EXE	"bluefish-uninst.exe"

!if ${BUILD} == "2.16.6"
	!define GTK_MIN_VERSION	"2.14.7"
	!define GTK_VERSION	"${BUILD}"
	!define GTK_URL		"http://downloads.sourceforge.net/project/gtk-win/GTK%2B%20Runtime%20Environment/GTK%2B%202.16"
	!define GTK_FILENAME	"gtk2-runtime-2.16.6-2010-05-12-ash.exe"
	!define GTK_SIZE	"27183" ; Install size in Kilobytes
	OutFile		"${PRODUCT}-${VERSION}-classic-setup.exe"
	!define UNINSTALL_GTK	"gtk2_runtime_uninst.exe"
!else if ${BUILD} == "3.6.4"
	!define GTK_MIN_VERSION	"3.6.0"
	!define GTK_VERSION	"${BUILD}" 
	!define GTK_URL		"http://www.muleslow.net/files/gtk+/packages"
	!define GTK_FILENAME	"gtk+3.6.4_bin.tbz2"
	!define GTK_SIZE	"44651" ; Install size in Kilobytes
	OutFile		"${PRODUCT}-${VERSION}-gtk3-setup.exe"
!else ; Default 2.24.8
	!define GTK_MIN_VERSION	"2.22.0"
	!define GTK_VERSION	"${BUILD}" 
	!define GTK_URL		"http://downloads.sourceforge.net/project/gtk-win/GTK%2B%20Runtime%20Environment/GTK%2B%202.24"
	!define GTK_FILENAME	"gtk2-runtime-2.24.8-2011-12-03-ash.exe"
	!define GTK_SIZE	"14803" ; Install size in Kilobytes
	OutFile		"${PRODUCT}-${VERSION}-setup.exe"
	!define UNINSTALL_GTK	"gtk2_runtime_uninst.exe"
!endif

!define PYTHON_MIN_VERSION "2.7.0"
!define PYTHON_URL "http://www.python.org/ftp/python/2.7.3"
!define PYTHON_FILENAME "python-2.7.3.msi"
!define PYTHON_SIZE "15868"

!define AS_DICT_URL	"http://www.muleslow.net/files/aspell/lang"

!define REG_USER_SET	"Software\${PRODUCT}"
!define REG_CLASS_SET	"Software\Classes"
!define REG_UNINSTALL	"Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"


; Variables
;----------------------------------------------
Var HKEY
Var GTK_STATUS
Var PYTHON_STATUS
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
Var PROG
Var SHELLCMD


; Installer configuration settings
;----------------------------------------------
Name		"${PRODUCT} v${VERSION}"
InstallDir	"$PROGRAMFILES32\${PRODUCT}"

; Tell Windows Vista and Windows 7 that we want admin rights to install
RequestExecutionLevel admin

SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show


; Installer version information
;----------------------------------------------
VIProductVersion "2.2.6.0"
VIAddVersionKey "ProductName" "${PRODUCT}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "FileDescription" "Bluefish Installer"


; MUI configuration
;----------------------------------------------
;!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!define MUI_LICENSEPAGE_BUTTON			"$(LICENSEPAGE_BUTTON) >"
!define MUI_LICENSEPAGE_TEXT_BOTTOM		"$(LICENSEPAGE_FOOTER)"
!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_LINK		"$(FINISHPAGE_LINK)"
!define MUI_FINISHPAGE_LINK_LOCATION	"${HOMEPAGE}"
!define MUI_UNFINISHPAGE_NOAUTOCLOSE


; Macros
;----------------------------------------------
!include "includes\Macros.nsh"


; Pages
;----------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE 		"..\COPYING"	; Possibly provide localized versions of the GPL
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU			"${PRODUCT}"	$StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
Page custom FileAssociations SetFileAssociations
!define MUI_PAGE_CUSTOMFUNCTION_SHOW 	DisableBackButton
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


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
!insertmacro MUI_LANGUAGE	"Danish" 	; 1030
${LoadLocalization}	"DANISH" 	"locale\Danish.nsh"
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
!insertmacro MUI_LANGUAGE	"Polish" 	; 1045
${LoadLocalization}	"POLISH"	"locale\Polish.nsh"
!insertmacro MUI_LANGUAGE	"Swedish" 	; 1053
${LoadLocalization}	"SWEDISH"	"locale\Swedish.nsh"
!insertmacro MUI_LANGUAGE	"SimpChinese" 	; 2052
${LoadLocalization}	"SIMPCHINESE"	"locale\SimpChinese.nsh"
!insertmacro MUI_LANGUAGE	"Spanish" 	; 1034
${LoadLocalization}	"SPANISH"	"locale\Spanish.nsh"
!insertmacro MUI_LANGUAGE	"Ukrainian" ; 1058
${LoadLocalization}	"UKRAINIAN"	"locale\Ukrainian.nsh"
; Translations needed for the following commented languages
;;!insertmacro MUI_LANGUAGE	"Bulgarian" 	; 1026
;;!insertmacro MUI_LANGUAGE	"Finnish" 	; 1035
;;!insertmacro MUI_LANGUAGE	"Greek" 	; 1032
;;!insertmacro MUI_LANGUAGE	"Korean" 	; 1042
;;!insertmacro MUI_LANGUAGE	"Polish" 	; 1045
;;!insertmacro MUI_LANGUAGE	"Portuguese" 	; 2070
;;!insertmacro MUI_LANGUAGE	"Romanian" 	; 1048
;;!insertmacro MUI_LANGUAGE	"Russian" 	; 1049
;;!insertmacro MUI_LANGUAGE	"Serbian" 	; 3098
;;!insertmacro MUI_LANGUAGE	"Slovak" 	; 1051
;;!insertmacro MUI_LANGUAGE	"TradChinese" 	; 1028
;;!insertmacro MUI_LANGUAGE	"Turkish" 	; 1055

; NSIS Lacks a translation for this language
;!insertmacro MUI_LANGUAGE	"Tamil" 	; 1097


; Sections
;----------------------------------------------
Section "$(SECT_BLUEFISH)" SecBluefish
	SectionIn 1 RO

	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "build\${PROGRAM_EXE}"
	File "build\libaspell-15.dll"
	File "build\libenchant-1.dll"
	File "build\libgnurx-0.dll"
	File "build\libxml2-2.dll"

	SetOutPath "$INSTDIR\share\doc\bluefish"
	File "..\COPYING"

	; Aspell/Enchant support files
	SetOutPath "$INSTDIR\lib\aspell-0.60"
	File /r "build\lib\aspell-0.60\*"
	SetOutPath "$INSTDIR\lib\enchant"
	File /r "build\lib\enchant\*"
	
	SetOutPath "$INSTDIR\lib\${PACKAGE}"
	File "build\lib\${PACKAGE}\about.dll"

	SetOutPath "$INSTDIR\share\${PACKAGE}"
	File /r /x "plugins"  "build\share\${PACKAGE}\*"
	SetOutPath "$INSTDIR\share\enchant"
	File /r "build\share\enchant\*"

	SetOutPath "$INSTDIR\share\locale"
	File /r /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"

	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\${UNINSTALL_EXE}"
	SetOverwrite off

	${If} $HKEY == "HKLM"
	${OrIf} $HKEY == "Classic"
		${If} $HKEY == "Classic"
			WriteRegStr HKCU "${REG_USER_SET}" "" "$INSTDIR" ; Replace InstallDirRegKey function
			WriteRegStr HKCU "${REG_USER_SET}" "Version" "${VERSION}"
			WriteRegStr HKCU "${REG_USER_SET}" "Package" "${PACKAGE}"
			WriteRegStr HKCU "${REG_USER_SET}" "Installer Language" $LANGUAGE ; Replace macro MUI_LANGDLL_SAVELANGUAGE
			WriteRegStr HKCU "${REG_USER_SET}" "Start Menu Folder" $StartMenuFolder ; Replace macros MUI_STARTMENU_WRITE_*
		${Else}
			WriteRegStr HKLM "${REG_USER_SET}" "" "$INSTDIR" ; Replace InstallDirRegKey function
			WriteRegStr HKLM "${REG_USER_SET}" "Version" "${VERSION}"
			WriteRegStr HKLM "${REG_USER_SET}" "Package" "${PACKAGE}"
			WriteRegStr HKLM "${REG_USER_SET}" "Installer Language" $LANGUAGE ; Replace macro MUI_LANGDLL_SAVELANGUAGE
			WriteRegStr HKLM "${REG_USER_SET}" "Start Menu Folder" $StartMenuFolder ; Replace macros MUI_STARTMENU_WRITE_*
		${EndIf}

		WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" 		"${PRODUCT} ${VERSION}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayIcon" 		"$INSTDIR\${PROGRAM_EXE}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" "$INSTDIR\${UNINSTALL_EXE}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "Publisher" 		"${PUBLISHER}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "URLInfoAbout" 	"${HOMEPAGE}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "HelpLink" 			"${HELPURL}"
		WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayVersion" 	"${VERSION}"
		WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify" "1"
		WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair" "1"
	${Else}
		WriteRegStr HKCU "${REG_USER_SET}" "" "$INSTDIR" ; Replace InstallDirRegKey function
		WriteRegStr HKCU "${REG_USER_SET}" "Version" "${VERSION}"
		WriteRegStr HKCU "${REG_USER_SET}" "Package" "${PACKAGE}"
		WriteRegStr HKCU "${REG_USER_SET}" "Installer Language" $LANGUAGE ; Replace macro MUI_LANGDLL_SAVELANGUAGE
		WriteRegStr HKCU "${REG_USER_SET}" "Start Menu Folder" $StartMenuFolder ; Replace macros MUI_STARTMENU_WRITE_*

		WriteRegStr HKCU "${REG_UNINSTALL}" "DisplayName" 		"${PRODUCT} ${VERSION}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "DisplayIcon" 		"$INSTDIR\${PROGRAM_EXE}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "UninstallString" "$INSTDIR\${UNINSTALL_EXE}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "Publisher" 		"${PUBLISHER}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "URLInfoAbout" 	"${HOMEPAGE}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "HelpLink" 			"${HELPURL}"
		WriteRegStr HKCU "${REG_UNINSTALL}" "DisplayVersion" 	"${VERSION}"
		WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoModify" "1"
		WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoRepair" "1"
	${EndIf}

	SetOverwrite on
	CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}" "-n"
	CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(UNINSTALL_SHORTCUT).lnk" "$INSTDIR\${UNINSTALL_EXE}"
	SetOverwrite off
SectionEnd

SectionGroup "$(SECT_DEPENDS)" SecDepends
	Section "GTK+ ${BUILD}" SecGTK
		SectionIn 1 RO
		${If} $GTK_STATUS == ""
!if ${GTK_VERSION} != 3.6.4
			IfFileExists "$INSTDIR\${UNINSTALL_GTK}" +3 0 ; Uninstall previous version
				DetailPrint "$(GTK_UNINSTALL)"
				ExecWait '"$INSTDIR\${UNINSTALL_GTK}" /S /sideeffects=no /dllpath=root /remove_config=yes'
	
			IfFileExists "$EXEDIR\redist\${GTK_FILENAME}" 0 GtkInstDown
				${StrRep} $R1 "$(DOWN_LOCAL)" "%s" "${GTK_FILENAME}"
				DetailPrint "$R1"
				md5dll::GetMD5File "$EXEDIR\redist\${GTK_FILENAME}"
	  			Pop $R0
	  			${If} $R0 == ${MD5_${GTK_FILENAME}}
	  				DetailPrint "$(DOWN_CHKSUM)"
	  				ExecWait '"$EXEDIR\redist\${GTK_FILENAME}" /S /sideeffects=no /dllpath=root /D=$INSTDIR'
	  				Goto GtkInstSetVer ; Jump to setting version
	  			${Else}
	  				DetailPrint "$(DOWN_CHKSUM_ERROR)"
	  				Goto GtkInstDown ; Jump to '${EndIf}+1
	  			${EndIf}
!else
			IfFileExists "$EXEDIR\redist\${GTK_FILENAME}" 0 GtkInstDown
				${StrRep} $R1 "$(DOWN_LOCAL)" "%s" "${GTK_FILENAME}"
				DetailPrint "$R1"
				md5dll::GetMD5File "$EXEDIR\redist\${GTK_FILENAME}"
	  			Pop $R0
	  			${If} $R0 == ${MD5_${GTK_FILENAME}}
	  				DetailPrint "$(DOWN_CHKSUM)"
					untgz::extract "-d" "$INSTDIR" "-u" "-zbz2" "$EXEDIR\redist\${GTK_FILENAME}"
  					Pop $R0
	  				Goto GtkInstSetVer ; Jump to setting version
	  			${Else}
	  				DetailPrint "$(DOWN_CHKSUM_ERROR)"
	  				Goto GtkInstDown ; Jump to '${EndIf}+1
	  			${EndIf}
!endif
	
	GtkInstDown:
			IntFmt $R1 "%u" 0
			IntCmp $R1 0 +3 +3 0
	GtkInstDownRetry:
				DetailPrint "Download Retry $R1 of 5..."
				DetailPrint "$(GTK_DOWNLOAD) (${GTK_URL}/${GTK_FILENAME})"
			Delete "$TEMP\${GTK_FILENAME}" ; Should never happen but just in case
			inetc::get /TRANSLATE "$(INETC_DOWN)" "$(INETC_CONN)" "$(INETC_TSEC)" "$(INETC_TMIN)" "$(INETC_THOUR)" "$(INETC_TPLUR)" "$(INETC_PROGRESS)" "$(INETC_REMAIN)" "${GTK_URL}/${GTK_FILENAME}" "$TEMP\${GTK_FILENAME}"
			Pop $R0
				StrCmp $R0 "OK" +14
				StrCmp $R0 "Terminated" +11
				StrCmp $R0 "Cancelled" +10
				StrCmp $R0 "Transfer Error" +6
				StrCmp $R0 "Connection Error" +5
				StrCmp $R0 "SendRequest Error" +4
				StrCmp $R0 "File Not Found (404)" +3
				StrCmp $R0 "Request Error" +2
				StrCmp $R0 "Server Error" +1
				IntCmp $R1 5 +3 0 0
					IntOp $R1 $R1 + 1
					Goto GtkInstDownRetry
					MessageBox MB_OK|MB_ICONEXCLAMATION "$(GTK_FAILED) $R0$\n$\n$(GTK_REQUIRED)"
					Return
			DetailPrint "$(GTK_INSTALL) (${GTK_FILENAME})"
!if ${GTK_VERSION} != 3.6.4
			ExecWait '"$TEMP\${GTK_FILENAME}" /S /sideeffects=no /dllpath=root /D=$INSTDIR'
			Delete "$TEMP\${GTK_FILENAME}"

	GtkInstSetVer:
			IfFileExists "$INSTDIR\${UNINSTALL_GTK}" 0 -7 ; If the uninstaller exists install completed successfully, otherwise display an error
!else
			untgz::extract "-d" "$INSTDIR" "-u" "-zbz2" "$TEMP\${GTK_FILENAME}"
			Pop $R0
			Delete "$TEMP\${GTK_FILENAME}"
			StrCmp $R0 "success" 0 +7
	GtkInstSetVer:
!endif
				${If} $HKEY == "Classic"
					WriteRegStr HKCU "${REG_USER_SET}" "GTK" "${GTK_VERSION}"
				${Else}
					WriteRegStr HKLM "${REG_USER_SET}" "GTK" "${GTK_VERSION}"
				${EndIf}
		${EndIf}
	SectionEnd
	Section /o "Python 2.7" SecPython
		${If} $PYTHON_STATUS == ""
			IfFileExists "$EXEDIR\redist\${PYTHON_FILENAME}" 0 PythonInstDown
				${StrRep} $R1 "$(DOWN_LOCAL)" "%s" "${PYTHON_FILENAME}"
				DetailPrint "$R1"
				md5dll::GetMD5File "$EXEDIR\redist\${PYTHON_FILENAME}"
	  			Pop $R0
	  			${If} $R0 == ${MD5_${PYTHON_FILENAME}}
	  				DetailPrint "$(DOWN_CHKSUM)"
	  				ExecWait 'msiexec /i "$EXEDIR\redist\${PYTHON_FILENAME}"'
	  			${Else}
	  				DetailPrint "$(DOWN_CHKSUM_ERROR)"
	  				Goto PythonInstDown ; Jump to '${EndIf}+1
	  			${EndIf}
	
	PythonInstDown:
			IntFmt $R1 "%u" 0
			IntCmp $R1 0 +3 +3 0
	PythonInstDownRetry:
				DetailPrint "Download Retry $R1 of 5..."
				DetailPrint "$(PYTHON_DOWNLOAD) (${PYTHON_URL}/${PYTHON_FILENAME})"
			Delete "$TEMP\${PYTHON_FILENAME}" ; Should never happen but just in case
			inetc::get /TRANSLATE "$(INETC_DOWN)" "$(INETC_CONN)" "$(INETC_TSEC)" "$(INETC_TMIN)" "$(INETC_THOUR)" "$(INETC_TPLUR)" "$(INETC_PROGRESS)" "$(INETC_REMAIN)" "${PYTHON_URL}/${PYTHON_FILENAME}" "$TEMP\${PYTHON_FILENAME}"
			Pop $R0
				StrCmp $R0 "OK" +14
				StrCmp $R0 "Terminated" +11
				StrCmp $R0 "Cancelled" +10
				StrCmp $R0 "Transfer Error" +6
				StrCmp $R0 "Connection Error" +5
				StrCmp $R0 "SendRequest Error" +4
				StrCmp $R0 "File Not Found (404)" +3
				StrCmp $R0 "Request Error" +2
				StrCmp $R0 "Server Error" +1
				IntCmp $R1 5 +3 0 0
					IntOp $R1 $R1 + 1
					Goto PythonInstDownRetry
					MessageBox MB_OK|MB_ICONEXCLAMATION "$(PYTHON_FAILED) $R0$\n$\n$(PYTHON_REQUIRED)"
					Return
			DetailPrint "$(PYTHON_INSTALL) (${PYTHON_FILENAME})"
			ExecWait 'msiexec /i "$TEMP\${PYTHON_FILENAME}"'
			Delete "$TEMP\${PYTHON_FILENAME}"
		${EndIf}
	SectionEnd
SectionGroupEnd

SectionGroup "$(SECT_PLUGINS)" SecPlugins
	SetOverwrite on
	Section "$(PLUG_CHARMAP)" SecPlCharmap
		SetOutPath "$INSTDIR"
		File "build\libgucharmap-7.dll"
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\charmap.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_ENTITIES)" SecPlEntities
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\entities.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_HTMLBAR)" SecPlHTMLbar
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\htmlbar.dll"
		SetOutPath "$INSTDIR\share\${PACKAGE}\plugins\htmlbar"
		File /r "build\share\${PACKAGE}\plugins\htmlbar\*"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_INFBROWSER)" SecPlInfBrowser
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\infbrowser.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_snippets.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_SNIPPETS)" SecPlSnippets
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\snippets.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_vcs.mo" /x "*_zencoding.mo" "build\share\locale\*"
	SectionEnd
;	Section "$(PLUG_VCS)" SecPlVcs
;		SetOutPath "$INSTDIR\lib\${PACKAGE}"
;		File "build\lib\${PACKAGE}\vcs.dll"
;		SetOutPath "$INSTDIR\share\locale"
;		File /nonfatal /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_zencoding.mo" "build\share\locale\*"
;	SectionEnd
	Section /o "$(PLUG_ZENCODING)" SecPlZencoding
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\zencoding.dll"
		SetOutPath "$INSTDIR\share\${PACKAGE}\plugins\zencoding"
		File /r "build\share\${PACKAGE}\plugins\zencoding\*"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" /x "*_vcs.mo" "build\share\locale\*"
	SectionEnd
	SetOverwrite off
SectionGroupEnd

Section "$(SECT_SHORTCUT)" SecDesktopShortcut
	SetOverwrite on
	SetOutPath "$INSTDIR"
	CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}" "-n"
	SetOverwrite off
SectionEnd

SectionGroup /e "$(SECT_DICT)" SecLang
	Section /o "Brazilian Portuguese" SecLangPt_BR
		${InstallAspellDict} "pt_BR" "20090702-0"
	SectionEnd
	Section /o "Bulgarian" SecLangBg
		${InstallAspellDict} "bg" "4.1-0"
	SectionEnd
	Section /o "Catalan" SecLangCa
		${InstallAspellDict} "ca" "2.1.5-1"
	SectionEnd
	Section /o "Czech" SecLangCs
		${InstallAspellDict} "cs" "20040614-1"
	SectionEnd
	Section /o "Danish" SecLangDa
		${InstallAspellDict} "da" "1.4.42-1"
	SectionEnd
	Section /o "Dutch" SecLangNl
		${InstallAspellDict} "nl" "0.50-2"
	SectionEnd
	Section /o "English" SecLangEn
		${InstallAspellDict} "en" "7.1-0"
	SectionEnd
	Section /o "Finnish" SecLangFi
		${InstallAspellDict} "fi" "0.7-0"
	SectionEnd
	Section /o "French" SecLangFr
		${InstallAspellDict} "fr" "0.50-3"
	SectionEnd
	Section /o "Galician" SecLangGl
		${InstallAspellDict} "gl" "0.5a-2"
	SectionEnd
	Section /o "German" SecLangDe
		${InstallAspellDict} "de" "20030222-1"
	SectionEnd
	Section /o "Greek" SecLangEl
		${InstallAspellDict} "el" "0.50-3"
	SectionEnd
	Section /o "Hungarian" SecLangHu
		${InstallAspellDict} "hu" "0.99.4.2-0"
	SectionEnd
	Section /o "Italian" SecLangIt
		${InstallAspellDict} "it" "2.2_20050523-0"
	SectionEnd
	Section /o "Norwegian Bokmål" SecLangNb
		${InstallAspellDict} "nb" "0.50.1-0"
	SectionEnd
	Section /o "Norwegian Nynorsk" SecLangNn
		${InstallAspellDict} "nn" "0.50.1-1"
	SectionEnd
	Section /o "Portuguese" SecLangPt_PT
		${InstallAspellDict} "pt_PT" "20070510-0"
	SectionEnd
	Section /o "Romanian" SecLangRo
		${InstallAspellDict} "ro" "3.1"
	SectionEnd
	Section /o "Russian" SecLangRu
		${InstallAspellDict} "ru" "0.99f7-1"
	SectionEnd
	Section /o "Serbian" SecLangSr
		${InstallAspellDict} "sr" "0.02"
	SectionEnd
	Section /o "Slovak" SecLangSk
		${InstallAspellDict} "sk" "2.01-2"
	SectionEnd
	Section /o "Spanish" SecLangEs
		${InstallAspellDict} "es" "1.9a-1"
	SectionEnd
	Section /o "Swedish" SecLangSv
		${InstallAspellDict} "sv" "0.51-0"
	SectionEnd
	Section /o "Tamil" SecLangTa
		${InstallAspellDict} "ta" "20040424-1"
	SectionEnd
	Section /o "Turkish" SecLangTr
		${InstallAspellDict} "tr" "0.50-0"
	SectionEnd
SectionGroupEnd

Section "Uninstall"
	Delete "$INSTDIR\${PROGRAM_EXE}"
	Delete "$INSTDIR\libaspell-15.dll"
	Delete "$INSTDIR\libenchant-1.dll"
	Delete "$INSTDIR\libgnurx-0.dll"
	Delete "$INSTDIR\libgucharmap-7.dll"
	Delete "$INSTDIR\libpcre-0.dll" ; Not longer shipped
	Delete "$INSTDIR\libxml2-2.dll"

!if ${GTK_VERSION} == 3.6.4
	Delete "$INSTDIR\gdk-pixbuf-query-loaders.exe"
	Delete "$INSTDIR\gspawn-win32-helper-console.exe"
	Delete "$INSTDIR\gspawn-win32-helper.exe"
	Delete "$INSTDIR\gtk-query-immodules-3.0.exe"
	Delete "$INSTDIR\gtk-update-icon-cache.exe"
	Delete "$INSTDIR\libatk-1.0-0.dll"
	Delete "$INSTDIR\libcairo-2.dll"
	Delete "$INSTDIR\libcairo-gobject-2.dll"
	Delete "$INSTDIR\libcairo-script-interpreter-2.dll"
	Delete "$INSTDIR\libcroco-0.6-3.dll"
	Delete "$INSTDIR\libffi-6.dll"
	Delete "$INSTDIR\libfreetype-6.dll"
	Delete "$INSTDIR\libgdk-3-0.dll"
	Delete "$INSTDIR\libgdk_pixbuf-2.0-0.dll"
	Delete "$INSTDIR\libgio-2.0-0.dll"
	Delete "$INSTDIR\libglib-2.0-0.dll"
	Delete "$INSTDIR\libgmodule-2.0-0.dll"
	Delete "$INSTDIR\libgobject-2.0-0.dll"
	Delete "$INSTDIR\libgthread-2.0-0.dll"
	Delete "$INSTDIR\libgtk-3-0.dll"
	Delete "$INSTDIR\libiconv-2.dll"
	Delete "$INSTDIR\libintl-8.dll"
	Delete "$INSTDIR\liblzma-5.dll"
	Delete "$INSTDIR\libpango-1.0-0.dll"
	Delete "$INSTDIR\libpangocairo-1.0-0.dll"
	Delete "$INSTDIR\libpangowin32-1.0-0.dll"
	Delete "$INSTDIR\libpixman-1-0.dll"
	Delete "$INSTDIR\libpng15-15.dll"
	Delete "$INSTDIR\librsvg-2-2.dll"
	Delete "$INSTDIR\pango-querymodules.exe"
	Delete "$INSTDIR\pthreadGC2.dll"
	Delete "$INSTDIR\zlib1.dll"
!else
	DetailPrint "$(GTK_UNINSTALL)"
	ExecWait '"$INSTDIR\${UNINSTALL_GTK}" /S /sideeffects=no /dllpath=root /remove_config=yes'
	Delete "$INSTDIR\${UNINSTALL_GTK}"
!endif

	RMDir /r "$INSTDIR\etc"
	RMDir /r "$INSTDIR\lib"
	RMDir /r "$INSTDIR\share"

	Delete "$INSTDIR\${UNINSTALL_EXE}"
	RMDir  "$INSTDIR"

	Delete "$DESKTOP\${PRODUCT}.lnk"

	Delete "$SMPROGRAMS\$StartMenuFolder\${PRODUCT}.lnk"
	Delete "$SMPROGRAMS\$StartMenuFolder\$(UNINSTALL_SHORTCUT).lnk"
	RMDir "$SMPROGRAMS\$StartMenuFolder"

	${If} $HKEY == "HKCU"
	${OrIf} $HKEY == "Classic"
		DeleteRegValue HKCU ${REG_USER_SET} ""
		DeleteRegValue HKCU ${REG_USER_SET} "Installer Language"
		DeleteRegValue HKCU ${REG_USER_SET} "Package"
		DeleteRegValue HKCU ${REG_USER_SET} "Start Menu Folder"
		DeleteRegValue HKCU ${REG_USER_SET} "Version"
		DeleteRegValue HKCU ${REG_USER_SET} "GTK"
		DeleteRegKey HKCU "${REG_USER_SET}\Aspell"
		DeleteRegKey HKCU "${REG_USER_SET}\Plugins"
		DeleteRegKey /ifempty HKCU ${REG_USER_SET}
	${Else}
		DeleteRegValue HKLM ${REG_USER_SET} ""
		DeleteRegValue HKLM ${REG_USER_SET} "Installer Language"
		DeleteRegValue HKLM ${REG_USER_SET} "Package"
		DeleteRegValue HKLM ${REG_USER_SET} "Start Menu Folder"
		DeleteRegValue HKLM ${REG_USER_SET} "Version"
		DeleteRegValue HKLM ${REG_USER_SET} "GTK"
		DeleteRegKey HKLM "${REG_USER_SET}\Aspell"
		DeleteRegKey HKLM "${REG_USER_SET}\Plugins"
		DeleteRegKey /ifempty HKLM ${REG_USER_SET}
	${EndIf}

	Call un.UnRegisterFileTypes	
	Call un.UnRegisterHTML

	${If} $HKEY == "HKLM"
	${OrIf} $HKEY == "Classic"
		DeleteRegKey HKLM "${REG_UNINSTALL}"
	${Else}
		DeleteRegKey HKCU "${REG_UNINSTALL}"
	${EndIf}
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
	
	Call GtkVersionCheck
	${If} $GTK_STATUS == ""	
		SectionSetSize ${SecGTK} ${GTK_SIZE}	; 7.54MB Download
	${EndIf}

	Call PythonVersionCheck
	${If} $PYTHON_STATUS == ""
		SectionSetSize ${SecPython} ${PYTHON_SIZE}	; 15.1MB Download
	${Else} ; Mark as enabled and readonly if already installed
		!insertmacro SelectSection ${SecPython}
		Push $0
			SectionGetFlags ${SecPython} $0
			IntOp $0 $0 | ${SF_RO}
			SectionSetFlags ${SecPython} $0
		Pop $0
	${EndIf}

	SectionSetSize ${SecLangBg} 2501		; 842KB Download
	SectionSetSize ${SecLangCs} 6640		; 2.21MB Download
	SectionSetSize ${SecLangDa} 12394		; 3.86MB Download
	SectionSetSize ${SecLangDe} 11355		; 3.63MB Download
	SectionSetSize ${SecLangEl} 12975		; 4.03MB Download
	SectionSetSize ${SecLangEn} 3557		; 1.21MB Download
	SectionSetSize ${SecLangEs} 1198		; 403KB Download
	SectionSetSize ${SecLangFi} 2750		; 680KB Download
	SectionSetSize ${SecLangFr} 17762		; 5.55MB Download
	SectionSetSize ${SecLangGl} 814		; 299KB Download
	SectionSetSize ${SecLangHu} 7098		; 1.74MB Download
	SectionSetSize ${SecLangIt} 2119		; 690KB Download
	SectionSetSize ${SecLangNb} 11040		; 3.47MB Download
	SectionSetSize ${SecLangNn} 6321		;  2.0M B Download 	
	SectionSetSize ${SecLangNl} 4430		; 1.54MB Download
	SectionSetSize ${SecLangPt_BR} 7946	; 2.54MB Download
	SectionSetSize ${SecLangPt_PT} 734		; 273KB Download
	SectionSetSize ${SecLangRo} 30943		; 9.64MB Download
	SectionSetSize ${SecLangRu} 5904		; 1.87MB Download
	SectionSetSize ${SecLangSk} 3810		; 1.34MB Download
	SectionSetSize ${SecLangSr} 12196		; 4.35MB Download
	SectionSetSize ${SecLangSv} 3522		; 1.19MB Download
	SectionSetSize ${SecLangTa} 283 		; 112KB Download
	SectionSetSize ${SecLangTr} 1554		; 534KB Download


; If we're updating from bluefish-unstable to bluefish we should uninstall first
	Push $R0
	Push $R1
	Push $R2
	${If} $HKEY == "HKLM"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R1 HKLM "${REG_UNINSTALL}" "DisplayVersion"
		${If} $HKEY == "Classic"
			ReadRegStr $R2 HKCU ${REG_USER_SET} "Package"
		${Else}
			ReadRegStr $R2 HKLM ${REG_USER_SET} "Package"
		${EndIf}
	${Else}
		ReadRegStr $R1 HKCU "${REG_UNINSTALL}" "DisplayVersion"
		ReadRegStr $R2 HKCU ${REG_USER_SET} "Package"
	${EndIf}
	${If} $R2 == "bluefish-unstable"
		MessageBox MB_OKCANCEL "$(UNSTABLE_UPGRADE)" IDCANCEL +7
			${If} $HKEY == "HKCU"
			${OrIf} $HKEY == "Classic"
				ReadRegStr $R2 HKCU ${REG_UNINSTALL} "UninstallString"
			${Else}
				ReadRegStr $R2 HKLM ${REG_UNINSTALL} "UninstallString"
			${EndIf}
			ExecWait '"$R2"'
	${EndIf}

;; Bugfix section-- :( 
; Fix a bug from the 1.3.7 installers, Path should be REG_EXPAND_SZ or variable expansion breaks
	${If} $R1 == "1.3.7"
		${If} $HKEY == "HKLM"
		${OrIf} $HKEY == "Classic"
			ReadRegStr $R2 HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path"
			WriteRegExpandStr HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path" $R2
		${Else}
			ReadRegStr $R2 HKCU "System\CurrentControlSet\Control\Session Manager\Environment" "Path"
			WriteRegExpandStr HKCU "System\CurrentControlSet\Control\Session Manager\Environment" "Path" $R2
		${EndIf}
	${EndIf}

	${If} $HKEY == "HKCU"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R1 HKCU ${REG_USER_SET} "Version"
	${Else}
		ReadRegStr $R1 HKLM ${REG_USER_SET} "Version"
	${EndIf}

; Fix would be uninstall problems
	${If} $R1 == "1.3.7"
	${OrIf} $R1 == "1.3.8"
	${OrIf} $R1 == "1.3.9"
	${OrIf} $R1 == "2.0.0-rc1"
	${OrIf} $R1 == "2.0.0-rc2"
	${OrIf} $R1 == "2.0.0-rc3"
	${OrIf} $R1 == "2.0.0-rc3-1"
	${OrIf} $R1 == "2.0.0"
	${OrIf} $R1 == "2.0.0-1"
	${OrIf} $R1 == "2.0.1"
	${OrIf} $R1 == "2.0.2"
	${OrIf} $R1 == "2.0.3"
	${OrIf} $R1 == "2.2.0"
	${OrIf} $R1 == "2.2.1"
	${OrIf} $R1 == "2.2.2"
	${OrIf} $R1 == "2.2.3"
	${OrIf} $R1 == "2.2.4"
	${OrIf} $R1 == "2.2.5"
		ReadRegStr $R0 HKCU "${REG_UNINSTALL}\Backup\HKCR\.vbs" "" ; Read stored class
		ReadRegStr $R2 HKCR ".vbs" "" ; Read current class
		${If} $R2 == "" ; This class should never be empty, may indicated a previous Bluefish uninstallation
			WriteRegStr HKCR ".vbs" "" "VBSFile"
		${EndIf}
		${If} $R0 == "" ; Update our stored class so we can restore it properly when uninstalling
			WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HKCR\.vbs" "" "VBSFile"
		${EndIf}

		ReadRegStr $R0 HKCU "${REG_UNINSTALL}\Backup\HKCR\.js" "" ; Read stored class
		ReadRegStr $R2 HKCR ".js" "" ; Read current class
		${If} $R2 == "" ; This class should never be empty, may indicated a previous Bluefish uninstallation
			WriteRegStr HKCR ".js" "" "JSFile"
		${EndIf}
		${If} $R0 == "" ; Update our stored class so we can restore it properly when uninstalling
			WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HKCR\.js" "" "JSFile"
		${EndIf}
	${EndIf}

; Get installed dictionaries and select them or if Bluefish isn't installed selected the
;  dictionary that matches the language chosen by the user defaulting to English if there
;  is no corresponding dictionary
	${If} $R1 != ""
		StrCpy $R2 0
		StrCpy $R0 "init"
		${While} $R0 != ""
			${If} $HKEY == "HKCU"
			${OrIf} $HKEY == "Classic"
				EnumRegKey $R0 HKCU "${REG_USER_SET}\Aspell" $R2
			${Else}
				EnumRegKey $R0 HKLM "${REG_USER_SET}\Aspell" $R2
			${EndIf}
			IntOp $R2 $R2 + 1
			${Switch} $R0
				${Case} "bg"
					SectionSetFlags ${SecLangBg} ${SF_SELECTED}
					${Break}
				${Case} "cs"
					SectionSetFlags ${SecLangCs} ${SF_SELECTED}
					${Break}
				${Case} "da"
					SectionSetFlags ${SecLangDa} ${SF_SELECTED}
					${Break}
				${Case} "de"
					SectionSetFlags ${SecLangDe} ${SF_SELECTED}
					${Break}
				${Case} "el"
					SectionSetFlags ${SecLangEl} ${SF_SELECTED}
					${Break}
				${Case} "en"
					SectionSetFlags ${SecLangEn} ${SF_SELECTED}
					${Break}
				${Case} "es"
					SectionSetFlags ${SecLangEs} ${SF_SELECTED}
					${Break}
				${Case} "fi"
					SectionSetFlags ${SecLangFi} ${SF_SELECTED}
					${Break}
				${Case} "fr"
					SectionSetFlags ${SecLangFr} ${SF_SELECTED}
					${Break}
				${Case} "hu"
					SectionSetFlags ${SecLangHu} ${SF_SELECTED}
					${Break}
				${Case} "it"
					SectionSetFlags ${SecLangIt} ${SF_SELECTED}
					${Break}
				${Case} "nl"
					SectionSetFlags ${SecLangNl} ${SF_SELECTED}
					${Break}
				${Case} "pt_br"
					SectionSetFlags ${SecLangPt_BR} ${SF_SELECTED}
					${Break}
				${Case} "ro"
					SectionSetFlags ${SecLangRo} ${SF_SELECTED}
					${Break}
				${Case} "ru"
					SectionSetFlags ${SecLangRu} ${SF_SELECTED}
					${Break}
				${Case} "sk"
					SectionSetFlags ${SecLangSk} ${SF_SELECTED}
					${Break}
				${Case} "sv"
					SectionSetFlags ${SecLangSv} ${SF_SELECTED}
					${Break}
				${Case} "tr"
					SectionSetFlags ${SecLangTr} ${SF_SELECTED}
					${Break}
				${Case} "ta"
					SectionSetFlags ${SecLangTa} ${SF_SELECTED}
					${Break}
				${Case} "gl"
					SectionSetFlags ${SecLangGl} ${SF_SELECTED}
					${Break}
				${Case} "nb"
					SectionSetFlags ${SecLangNb} ${SF_SELECTED}
					${Break}
				${Case} "nn"
					SectionSetFlags ${SecLangNn} ${SF_SELECTED}
					${Break}
				${Case} "pt_pt"
					SectionSetFlags ${SecLangPt_PT} ${SF_SELECTED}
					${Break}
				${Case} "sr"
					SectionSetFlags ${SecLangSr} ${SF_SELECTED}
					${Break}
				${Default}
					${Break}
			${EndSwitch}
		${EndWhile}
	${Else}
		${Switch} $LANGUAGE
			${Case} 1026 ; Bulgarian
				SectionSetFlags ${SecLangBg} ${SF_SELECTED}
				${Break}
			${Case} 1027 ; Catalan
				SectionSetFlags ${SecLangCa} ${SF_SELECTED}
				${Break}
			${Case} 1029 ; Czech
				SectionSetFlags ${SecLangCs} ${SF_SELECTED}
				${Break}
			${Case} 1030 ; Danish
				SectionSetFlags ${SecLangDa} ${SF_SELECTED}
				${Break}
			${Case} 1031 ; German
				SectionSetFlags ${SecLangDe} ${SF_SELECTED}
				${Break}
			${Case} 1032 ; Greek
				SectionSetFlags ${SecLangEl} ${SF_SELECTED}
				${Break}
			${Case} 1033 ; English
				SectionSetFlags ${SecLangEn} ${SF_SELECTED}
				${Break}
			${Case} 1034 ; Spanish
				SectionSetFlags ${SecLangEs} ${SF_SELECTED}
				${Break}
			${Case} 1035 ; Finnish
				SectionSetFlags ${SecLangFi} ${SF_SELECTED}
				${Break}
			${Case} 1036 ; French
				SectionSetFlags ${SecLangFr} ${SF_SELECTED}
				${Break}
			${Case} 1038 ; Hungarian
				SectionSetFlags ${SecLangHu} ${SF_SELECTED}
				${Break}
			${Case} 1040 ; Italian
				SectionSetFlags ${SecLangIt} ${SF_SELECTED}
				${Break}
			${Case} 1043 ; Dutch
				SectionSetFlags ${SecLangNl} ${SF_SELECTED}
				${Break}
			${Case} 1046 ; Brazilion Portuguese
				SectionSetFlags ${SecLangPt_BR} ${SF_SELECTED}
				${Break}
			${Case} 1048 ; Romanian
				SectionSetFlags ${SecLangRo} ${SF_SELECTED}
				${Break}
			${Case} 1049 ; Russian
				SectionSetFlags ${SecLangRu} ${SF_SELECTED}
				${Break}
			${Case} 1051 ; Slovak
				SectionSetFlags ${SecLangSk} ${SF_SELECTED}
				${Break}
			${Case} 1053 ; Swedish
				SectionSetFlags ${SecLangSv} ${SF_SELECTED}
				${Break}
			${Case} 1055 ; Turkish
				SectionSetFlags ${SecLangTr} ${SF_SELECTED}
				${Break}
			${Case} 1097 ; Tamil
				SectionSetFlags ${SecLangTa} ${SF_SELECTED}
				${Break}
			${Case} 1110 ; Galician
				SectionSetFlags ${SecLangGl} ${SF_SELECTED}
				${Break}
			${Case} 1044 ; Norwegian Bokmål
				SectionSetFlags ${SecLangNb} ${SF_SELECTED}
				${Break}
			${Case} 2068 ; Norwegian Nynorsk
				SectionSetFlags ${SecLangNn} ${SF_SELECTED}
				${Break}
			${Case} 2070 ; Portuguese
				SectionSetFlags ${SecLangPt_PT} ${SF_SELECTED}
				${Break}
			${Case} 3098 ; Serbian
				SectionSetFlags ${SecLangSr} ${SF_SELECTED}
				${Break}
			${Default}
				SectionSetFlags ${SecLangEn} ${SF_SELECTED}
				${Break}
		${EndSwitch}
	${EndIf}

	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Function .onSelChange
	${If} ${SectionIsSelected} ${SecPlZencoding}
	${AndIfNot} ${SectionIsSelected} ${SecPython}
		!insertmacro SelectSection ${SecPython}
	${EndIf}
FunctionEnd


; Uninstaller Functions
;----------------------------------------------
!include "includes\UnFunctions.nsh"
Function un.onInit
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
		StrCpy $INSTDIR $R0

		${If} $HKEY == "Classic"
			ReadRegStr $R0 HKCU "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		${Else}
			ReadRegStr $R0 HKLM "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		${EndIf}
		StrCpy $LANGUAGE $R0 ; Replace macro MUI_UNGETLANGUAGE

		${If} $HKEY == "Classic"
			ReadRegStr $R0 HKCU "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		${Else}
			ReadRegStr $R0 HKLM "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		${EndIf}
		StrCpy $StartMenuFolder $R0 ; Replace macro MUI_STARTMENU_GETFOLDER
	${Else}
		StrCpy $HKEY "HKCU"
		SetShellVarContext current

		ReadRegStr $R0 HKCU "${REG_USER_SET}" "" ; Replace InstallDirRegKey function
		StrCpy $INSTDIR $R0

		ReadRegStr $R0 HKCU "${REG_USER_SET}" "Installer Language" ; Replace defining MUI_LANGDLL_REGISTRY_*
		StrCpy $LANGUAGE $R0 ; Replace macro MUI_UNGETLANGUAGE

		ReadRegStr $R0 HKCU "${REG_USER_SET}" "Start Menu Folder" ; Replace MUI_STARTMENUPAGE_REGISTRY_*
		StrCpy $StartMenuFolder $R0 ; Replace macro MUI_STARTMENU_GETFOLDER
	${EndIf}
FunctionEnd
