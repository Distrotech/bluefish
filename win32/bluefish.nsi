;----------------------------------------------
; Bluefish Windows NSIS Install Script
; 
;  The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------


; External Defines
;----------------------------------------------
;!define PACKAGE
;!define LOCALE
;!define VERSION


; Defines
;----------------------------------------------
!define PRODUCT			"Bluefish"
!define PUBLISHER			"The Bluefish Developers"
!define HOMEPAGE			"http://bluefish.openoffice.nl/"
!define HELPURL			"http://bluefish.openoffice.nl/manual/"
!define PROGRAM_EXE		"${PACKAGE}.exe"
!define UNINSTALL_EXE	"bluefish-uninst.exe"

!define GTK_URL			"http://internap.dl.sourceforge.net/project/pidgin/GTK%2B%20for%20Windows/2.14.7%20Rev%20A"
!define GTK_FILENAME 	"gtk-runtime-2.14.7-rev-a.exe"
!define GTK_SIZE			"34549" ; Install size in Kilobytes
!define AS_DICT_URL		"http://www.muleslow.net/files/aspell/lang"

!define REG_USER_SET		"Software\${PRODUCT}"
!define REG_UNINSTALL	"Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT}"

!define GTK_MIN_VERSION	"2.14.7"


; Variables
;----------------------------------------------
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
Var FA_Po
Var FA_Html
Var FA_Java
Var FA_Js
Var FA_Jsp
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
Name						"${PRODUCT} v${VERSION}"
OutFile					"${PRODUCT}-${VERSION}-setup.exe"
InstallDir				"$PROGRAMFILES\${PRODUCT}"

; Tell Windows Vista and Windows 7 that we need admin rights to install
RequestExecutionLevel admin

InstallDirRegKey HKCU "${REG_USER_SET}" ""
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show


; Installer version information
;----------------------------------------------
VIProductVersion "2.0.0.0"
VIAddVersionKey "ProductName" "${PRODUCT}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "2.0"
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "FileDescription" "Bluefish Installer"


; Includes
;----------------------------------------------
!include "MUI2.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"
!include "WinMessages.nsh"
!include "LogicLib.nsh"
!include "nsDialogs.nsh"

!include "StrFunc.nsh"
${StrStr}
${StrTok}


; MUI configuration
;----------------------------------------------
!define MUI_LANGDLL_ALLLANGUAGES
!define MUI_LANGDLL_REGISTRY_ROOT 		"HKCU"
!define MUI_LANGDLL_REGISTRY_KEY 		${REG_USER_SET}
!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_LICENSEPAGE_BUTTON			"$(LICENSEPAGE_BUTTON) >"
!define MUI_LICENSEPAGE_TEXT_BOTTOM		"$(LICENSEPAGE_FOOTER)"

!define MUI_STARTMENUPAGE_DEFAULTFOLDER		"${PRODUCT}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT		"HKCU"
!define MUI_STARTMENUPAGE_REGISTRY_KEY 		${REG_USER_SET}
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME	"Start Menu Folder"

!define MUI_FINISHPAGE_LINK				"$(FINISHPAGE_LINK)"
!define MUI_FINISHPAGE_LINK_LOCATION	"http://bluefish.openoffice.nl"


; Macros
;----------------------------------------------
!macro InstallAspellDict LANG VER
	ReadRegStr $R0 HKCU "${REG_USER_SET}\Aspell\${LANG}" ""
	${If} $R0 == ${VER}
		DetailPrint "$(DICT_INSTALLED) ${LANG}"
	${Else}
		DetailPrint "$(DICT_DOWNLOAD) (${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2)"
		NSISdl::download "${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2" "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		Pop $R0
			StrCmp $R0 "success" +3
				MessageBox MB_OK "$(DICT_FAILED) $R0"
				Quit
		DetailPrint "$(DICT_EXTRACT) (aspell6-${LANG}-${VER}.tbz2)"
		untgz::extract -d $INSTDIR -zbz2 -u "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		WriteRegStr HKCU "${REG_USER_SET}\Aspell\${LANG}" "" "${VER}"
		Delete "$TEMP\aspell6-${LANG}-${VER}.tbz2"
	${EndIf}
!macroend
!define InstallAspellDict `!insertmacro InstallAspellDict`

!macro RegisterFileType HWND EXT TYPE PROG DESC ICON
	ReadRegStr $R1 HKCR ".${EXT}" "Content Type"
	${If} $R1 != "${TYPE}"
		WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "Content Type" $R1
		WriteRegStr HKCR ".${EXT}" "Content Type" "${TYPE}"
	${EndIf}
	${NSD_GetState} ${HWND} $R0
	${If} ${PROG} != "0"
		${If} $R0 == ${BST_CHECKED}
			ReadRegStr $R1 HKCR ".${EXT}" ""
			${If} $R1 != "${PROG}"
				WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "" $R1
				WriteRegStr HKCR ".${EXT}" "" "${PROG}"
			${EndIf}
			WriteRegStr HKCR "${PROG}" "" "${DESC}"
			WriteRegStr HKCR "${PROG}\DefaultIcon" "" "$INSTDIR\${PROGRAM_EXE},${ICON}"
			WriteRegStr HKCR "${PROG}\shell" "" "open"
			WriteRegStr HKCR "${PROG}\shell\open" "" "Open"
			WriteRegStr HKCR "${PROG}\shell\open\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
			; This is just so the un.UnRegisterFileTypes function removes all the bf*file entries
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\${PROG}" "" ""
		${EndIf}
	${EndIf}
!macroend
!define RegisterFileType `!insertmacro RegisterFileType`

!macro RegisterHTMLType HWND
	${NSD_GetState} ${HWND} $R0
	${If} $R0 == ${BST_CHECKED}
		ReadRegStr $R1 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description"
		${If} $R1 != "${PRODUCT}"
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "Description" $R1
			WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" "${PRODUCT}"

			ReadRegStr $R2 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" ""
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "command" $R2
			WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""

			ReadRegStr $R3 HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" ""
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "command2" $R3
			WriteRegStr HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
		${EndIf}
	${EndIf}
!macroend
!define RegisterHTMLType `!insertmacro RegisterHTMLType`

!macro Localize DEF LANG
	LangString "${DEF}" "${LANG_{$LANG}}" "${${DEF}}"
	!undef "${DEF}"
!macroend

!macro LoadLocalization LANG INC
	!include "${INC}"

	!insertmacro Localize "LICENSEPAGE_BUTTON" "${LANG}"
	!insertmacro Localize "LICENSEPAGE_FOOTER" "${LANG}"

	!insertmacro Localize "FINISHPAGE_LINK" "${LANG}"

	!insertmacro Localize "DICT_INSTALLED" "${LANG}"
	!insertmacro Localize "DICT_DOWNLOAD" "${LANG}"
	!insertmacro Localize "DICT_FAILED" "${LANG}"
	!insertmacro Localize "DICT_EXTRACT" "${LANG}"

	!insertmacro Localize "SECT_BLUEFISH" "${LANG}"
	!insertmacro Localize "UNINSTALL_SHORTCUT" "${LANG}"

	!insertmacro Localize "GTK_DOWNLOAD" "${LANG}"
	!insertmacro Localize "GTK_FAILED" "${LANG}"
	!insertmacro Localize "GTK_INSTALL" "${LANG}"

	!insertmacro Localize "SECT_PLUGINS" "${LANG}"
	!insertmacro Localize "PLUG_CHARMAP" "${LANG}"
	!insertmacro Localize "PLUG_ENTITIES" "${LANG}"
	!insertmacro Localize "PLUG_HTMLBAR" "${LANG}"
	!insertmacro Localize "PLUG_INFBROWSER" "${LANG}"
	!insertmacro Localize "PLUG_SNIPPETS" "${LANG}"

	!insertmacro Localize "SECT_SHORTCUT" "${LANG}"

	!insertmacro Localize "SECT_DICT" "${LANG}"

	!insertmacro Localize "FA_TITLE" "${LANG}"
	!insertmacro Localize "FA_HEADER" "${LANG}"
	!insertmacro Localize "FA_SELECT" "${LANG}"
	!insertmacro Localize "FA_UNSELECT" "${LANG}"

	!insertmacro Localize "GTK_PATH" "${LANG}"

	!insertmacro Localize "CT_ADA" "${LANG}"
	!insertmacro Localize "CT_ASP" "${LANG}"
	!insertmacro Localize "CT_SH" "${LANG}"
	!insertmacro Localize "CT_BFPROJECT" "${LANG}"
	!insertmacro Localize "CT_BFLANG2" "${LANG}"
	!insertmacro Localize "CT_C" "${LANG}"
	!insertmacro Localize "CT_H" "${LANG}"
	!insertmacro Localize "CT_CPP" "${LANG}"
	!insertmacro Localize "CT_HPP" "${LANG}"
	!insertmacro Localize "CT_CSS" "${LANG}"
	!insertmacro Localize "CT_D" "${LANG}"
	!insertmacro Localize "CT_PO" "${LANG}"
	!insertmacro Localize "CT_JAVA" "${LANG}"
	!insertmacro Localize "CT_JS" "${LANG}"
	!insertmacro Localize "CT_JSP" "${LANG}"
	!insertmacro Localize "CT_NSI" "${LANG}"
	!insertmacro Localize "CT_NSH" "${LANG}"
	!insertmacro Localize "CT_PL" "${LANG}"
	!insertmacro Localize "CT_PHP" "${LANG}"
	!insertmacro Localize "CT_TXT" "${LANG}"
	!insertmacro Localize "CT_PY" "${LANG}"
	!insertmacro Localize "CT_RB" "${LANG}"
	!insertmacro Localize "CT_SMARTY" "${LANG}"
	!insertmacro Localize "CT_VBS" "${LANG}"
	!insertmacro Localize "CT_XHTML" "${LANG}"
	!insertmacro Localize "CT_XML" "${LANG}"
	!insertmacro Localize "CT_XSL" "${LANG}"
!macroend
!define LoadLocalization `!insertmacro LoadLocalization`


; Pages
;----------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE				"..\COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU			"${PRODUCT}"	$StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
Page custom FileAssociations SetFileAssociations
!define MUI_PAGE_CUSTOMFUNCTION_SHOW	DisableBackButton
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH


; Locale
;----------------------------------------------
!insertmacro MUI_RESERVEFILE_LANGDLL

; English goes first as the default language
!insertmacro MUI_LANGUAGE	"English"
${LoadLocalization}	"ENGLISH"	"locale\English.nsh"
!insertmacro MUI_LANGUAGE	"Basque"
${LoadLocalization}	"BASQUE"	"locale\Basque.nsh"
!insertmacro MUI_LANGUAGE	"Czech"
${LoadLocalization}	"CZECH"	"locale\Czech.nsh"
!insertmacro MUI_LANGUAGE	"French"
${LoadLocalization}	"FRENCH"	"locale\French.nsh"
!insertmacro MUI_LANGUAGE	"Galician"
${LoadLocalization}	"GALICIAN"	"locale\Galician.nsh"
!insertmacro MUI_LANGUAGE	"German"
${LoadLocalization}	"GERMAN"	"locale\German.nsh"
!insertmacro MUI_LANGUAGE	"Japanese"
${LoadLocalization}	"JAPANESE"	"locale\Japanese.nsh"
!insertmacro MUI_LANGUAGE	"Norwegian"
${LoadLocalization}	"NORWEGIAN"	"locale\Norwegian.nsh"
!insertmacro MUI_LANGUAGE	"Swedish"
${LoadLocalization}	"SWEDISH"	"locale\Swedish.nsh"
; Translations needed for the following commented languages
;;!insertmacro MUI_LANGUAGE	"Bulgarian"
;;!insertmacro MUI_LANGUAGE	"Danish"
;;!insertmacro MUI_LANGUAGE	"Dutch"
;;!insertmacro MUI_LANGUAGE	"Finnish"
;;!insertmacro MUI_LANGUAGE	"Greek"
;;!insertmacro MUI_LANGUAGE	"Hungarian"
;;!insertmacro MUI_LANGUAGE	"Italian"
;;!insertmacro MUI_LANGUAGE	"Korean"
;;!insertmacro MUI_LANGUAGE	"Polish"
;;!insertmacro MUI_LANGUAGE	"Portuguese"
;;!insertmacro MUI_LANGUAGE	"PortugueseBR"
;;!insertmacro MUI_LANGUAGE	"Romanian"
;;!insertmacro MUI_LANGUAGE	"Russian"
;;!insertmacro MUI_LANGUAGE	"Serbian"
;;!insertmacro MUI_LANGUAGE	"SimpChinese"
;;!insertmacro MUI_LANGUAGE	"Slovak"
;;!insertmacro MUI_LANGUAGE	"Spanish"
; NSIS Lacks a translation for this language
;!insertmacro MUI_LANGUAGE	"Tamil"
;;!insertmacro MUI_LANGUAGE	"TradChinese"
;;!insertmacro MUI_LANGUAGE	"Turkish"


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
	File "build\libpcre-0.dll"
	File "build\libxml2-2.dll"

	SetOutPath "$INSTDIR\docs"
	File "..\COPYING"

	; Aspell/Enchant support files
	SetOutPath "$INSTDIR\lib\aspell-0.60"
	File /r "build\lib\aspell-0.60\*"
	SetOutPath "$INSTDIR\lib\enchant"
	File /r "build\lib\enchant\*"
	
	SetOutPath "$INSTDIR\lib\${PACKAGE}"
	File "build\lib\${PACKAGE}\about.dll"

	SetOutPath "$INSTDIR\share\${PACKAGE}"
	File /r "build\share\${PACKAGE}\*"
	SetOutPath "$INSTDIR\share\enchant"
	File /r "build\share\enchant\*"

	SetOutPath "$INSTDIR\share\locale"
	File /r /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"

	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\${UNINSTALL_EXE}"
	SetOverwrite off

	WriteRegStr HKCU "${REG_USER_SET}" "" "$\"$INSTDIR$\""
	WriteRegStr HKCU "${REG_USER_SET}" "Version" "${VERSION}"
	WriteRegStr HKCU "${REG_USER_SET}" "Package" "${PACKAGE}"

	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" 		"${PRODUCT} ${VERSION}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayIcon" 		"$INSTDIR\${PROGRAM_EXE}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" "$INSTDIR\${UNINSTALL_EXE}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "Publisher" 		"${PUBLISHER}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "URLInfoAbout" 	"${HOMEPAGE}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "HelpLink" 			"${HELPURL}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayVersion" 	"${VERSION}"
	WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify" "1"
	WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair" "1"

	!insertmacro MUI_LANGDLL_SAVELANGUAGE
	!insertmacro MUI_STARTMENU_WRITE_BEGIN "${PRODUCT}"
		SetOverwrite on
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\$(UNINSTALL_SHORTCUT).lnk" "$INSTDIR\${UNINSTALL_EXE}"
		SetOverwrite off
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "-GTK+ Installer" SecGTK
	${If} $GTK_STATUS == ""
		DetailPrint "$(GTK_DOWNLOAD) (${GTK_URL}/${GTK_FILENAME})"
		NSISdl::download "${GTK_URL}/${GTK_FILENAME}" "$TEMP\${GTK_FILENAME}"
		Pop $R0
			StrCmp $R0 "success" +3
				MessageBox MB_OK "$(GTK_FAILED) $R0"
				Quit 
		DetailPrint "$(GTK_INSTALL) (${GTK_FILENAME})"
		ExecWait '"$TEMP\${GTK_FILENAME}"'
		Delete "$TEMP\${GTK_FILENAME}"
		Call GtkInstallPath
	${Else}
		Call GtkInstallPath
	${EndIf}
SectionEnd

SectionGroup "$(SECT_PLUGINS)" SecPlugins
	SetOverwrite on
	Section "$(PLUG_CHARMAP)" SecPlCharmap
		SetOutPath "$INSTDIR"
		File "build\libgucharmap-7.dll"
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\charmap.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_ENTITIES)" SecPlEntities
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\entities.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_HTMLBAR)" SecPlHTMLbar
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\htmlbar.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_INFBROWSER)" SecPlInfBrowser
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\infbrowser.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "$(PLUG_SNIPPETS)" SecPlSnippets
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\snippets.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" "build\share\locale\*"
	SectionEnd
	SetOverwrite off
SectionGroupEnd

Section "$(SECT_SHORTCUT)" SecDesktopShortcut
	SetOverwrite on
	SetOutPath "$INSTDIR"
	CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}"
	SetOverwrite off
SectionEnd

SectionGroup /e "$(SECT_DICT)" SecLang
	Section /o "Bulgarian" SecLangBg
		${InstallAspellDict} "bg" "4.1-0"
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
	Section "English" SecLangEn
		${InstallAspellDict} "en" "6.0-0"
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
		${InstallAspellDict} "de" "20030222"
	SectionEnd
	Section /o "Greek" SecLangEl
		${InstallAspellDict} "el" "0.50-3"
	SectionEnd
	Section /o "Hungarian" SecLangHu
		${InstallAspellDict} "hu" "0.99.4.2-0"
	SectionEnd
	Section /o "Italian" SecLangIt
		${InstallAspellDict} "it" "2.2_20050523"
	SectionEnd
	Section /o "Norwegian" SecLangNb
		${InstallAspellDict} "nb" "0.50.1-0"
	SectionEnd
	Section /o "Portuguese" SecLangPt_PT
		${InstallAspellDict} "pt_PT" "20070510-0"
	SectionEnd
	Section /o "Portuguese (Brazilian)" SecLangPt_BR
		${InstallAspellDict} "pt_BR" "20090702-0"
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
	Delete "$INSTDIR\libpcre-0.dll"
	Delete "$INSTDIR\libxml2-2.dll"
	RMDir /r "$INSTDIR\docs"
	RMDir /r "$INSTDIR\lib"
	RMDir /r "$INSTDIR\share"
	Delete "$INSTDIR\${UNINSTALL_EXE}"
	RMDir  "$INSTDIR"
	Delete "$DESKTOP\${PRODUCT}.lnk"

	ReadRegStr $R0 HKCU ${REG_USER_SET} "Start Menu Folder"
	${If} $R0 != ""
	Delete "$SMPROGRAMS\$R0\${PRODUCT}.lnk"
	Delete "$SMPROGRAMS\$R0\$(UNINSTALL_SHORTCUT).lnk"
	RMDir "$SMPROGRAMS\$R0"
	${EndIf}
	DeleteRegValue HKCU ${REG_USER_SET} ""
	DeleteRegValue HKCU ${REG_USER_SET} "Installer Language"
	DeleteRegValue HKCU ${REG_USER_SET} "Package"
	DeleteRegValue HKCU ${REG_USER_SET} "Start Menu Folder"
	DeleteRegValue HKCU ${REG_USER_SET} "Version"
	DeleteRegKey HKCU "${REG_USER_SET}\Aspell"
	DeleteRegKey /ifempty HKCU ${REG_USER_SET}

	Call un.UnRegisterFileTypes	
	Call un.UnRegisterHTML

	DeleteRegKey HKLM "${REG_UNINSTALL}"
SectionEnd


; Installer Functions
;----------------------------------------------
Function .onInit
	Call GtkVersionCheck
	${If} $GTK_STATUS == ""	
		SectionSetSize ${SecGTK} ${GTK_SIZE}	; 6.69MB Download
	${EndIf}

	SectionSetSize ${SecLangBg} 2501		; 842KB Download
	SectionSetSize ${SecLangCs}	6640		; 2.21MB Download
	SectionSetSize ${SecLangDa} 12394		; 3.86MB Download
	SectionSetSize ${SecLangNl} 4430		; 1.54MB Download
	SectionSetSize ${SecLangEn} 3557		; 1.21MB Download
	SectionSetSize ${SecLangFi} 2750		; 680KB Download
	SectionSetSize ${SecLangFr} 17762		; 5.55MB Download
	SectionSetSize ${SecLangGl}	814		; 299KB Download
	SectionSetSize ${SecLangDe} 11355		; 3.63MB Download
	SectionSetSize ${SecLangEl} 12975		; 4.03MB Download
	SectionSetSize ${SecLangHu} 7098		; 1.74MB Download
	SectionSetSize ${SecLangIt} 2119		; 690KB Download
	SectionSetSize ${SecLangNb} 11040		; 3.47MB Download
	SectionSetSize ${SecLangPt_PT} 734		; 273KB Download
	SectionSetSize ${SecLangPt_BR} 7946	; 2.54MB Download
	SectionSetSize ${SecLangRo} 30943		; 9.64MB Download
	SectionSetSize ${SecLangRu} 5904		; 1.87MB Download
	SectionSetSize ${SecLangSr} 12196		; 4.35MB Download
	SectionSetSize ${SecLangSk} 3810		; 1.34MB Download
	SectionSetSize ${SecLangEs} 1198		; 403KB Download
	SectionSetSize ${SecLangSv} 3522		; 1.19MB Download
	SectionSetSize ${SecLangTa} 283			; 112KB Download
	SectionSetSize ${SecLangTr} 1554		; 534KB Download

; Fix a bug from the 1.3.7 installers, Path should be REG_EXPAND_SZ or variable expansion breaks
	Push $R1
	Push $R2
	ReadRegStr $R1 HKLM "${REG_UNINSTALL}" "DisplayVersion"
	${If} $R1 == "1.3.7"
		ReadRegStr $R2 HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path"
		WriteRegExpandStr HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path" $R2
	${EndIf}
	Pop $R2
	Pop $R1

; If we're updating from bluefish-unstable to bluefish we should uninstall first
  Push $R1
  Push $R2
    ReadRegStr $R1 HKCU ${REG_USER_SET} "Package"
    ${If} $R1 == "bluefish-unstable"
      MessageBox MB_OKCANCEL "An unstable release of ${PRODUCT} is installed. Should previous versions be removed before we continue (Recommended)." IDCANCEL +3
      ReadRegStr $R2 HKLM ${REG_UNINSTALL} "UninstallString"
      ExecWait '"C:\Program Files (x86)\Bluefish\${UNINSTALL_EXE}"'
    ${EndIf}
  Pop $R2
  Pop $R1

	!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function FileAssociations
	!insertmacro MUI_HEADER_TEXT "$(FA_TITLE)" "$(FA_HEADER)"
	nsDialogs::Create 1018
	Pop $FA_Dialog
	${If} $FA_Dialog == "error"
		Abort
	${EndIf}

	${NSD_CreateCheckBox} 5% 0u 40% 8u "Ada (.ada)"
	Pop $FA_Ada
	${NSD_CreateCheckBox} 5% 10u 40% 8u "ActiveServer Pages (.asp)"
	Pop $FA_Asp
	${NSD_CreateCheckBox} 5% 20u 40% 8u "Bash (.sh)"
	Pop $FA_Sh
	${NSD_CreateCheckBox} 5% 30u 40% 8u "Bluefish Project (.bfproject)"
	Pop $FA_BFProject
	${NSD_CreateCheckBox} 5% 40u 40% 8u "Bluefish Language (.bflang2)"
	Pop $FA_BFLang2
	${NSD_CreateCheckBox} 5% 50u 40% 8u "C (.c; .h)"
	Pop $FA_C
	${NSD_CreateCheckBox} 5% 60u 40% 8u "C++ (.cpp; .cxx; .cc; .hpp)"
	Pop $FA_Cpp
	${NSD_CreateCheckBox} 5% 70u 40% 8u "CSS (.css)"
	Pop $FA_Css
	${NSD_CreateCheckBox} 5% 80u 40% 8u "D (.d)"
	Pop $FA_D
	${NSD_CreateCheckBox} 5% 90u 40% 8u "Gettext PO (.po)"
	Pop $FA_Po
	${NSD_CreateCheckBox} 5% 100u 40% 8u "HTML (.htm; .html)"
	Pop $FA_Html
	${NSD_CreateCheckBox} 5% 110u 40% 8u "Java (.java)"
	Pop $FA_Java
	
	${NSD_CreateCheckBox} 55% 0u 40% 8u "JavaScript (.js)"
	Pop $FA_Js
	${NSD_CreateCheckBox} 55% 10u 40% 8u "JavaServer Pages (.jsp)"
	Pop $FA_Jsp
	${NSD_CreateCheckBox} 55% 20u 40% 8u "NSIS (.nsi; .nsh)"
	Pop $FA_Nsi
	${NSD_CreateCheckBox} 55% 30u 40% 8u "Perl (.pl)"
	Pop $FA_Pl
	${NSD_CreateCheckBox} 55% 40u 40% 8u "PHP (.php; .php3)"
	Pop $FA_Php
	${NSD_CreateCheckBox} 55% 50u 40% 8u "Plain Text (.txt)"
	Pop $FA_Txt
	${NSD_CreateCheckBox} 55% 60u 40% 8u "Python (.py)"
	Pop $FA_Py
	${NSD_CreateCheckBox} 55% 70u 40% 8u "Ruby (.rb)"
	Pop $FA_Rb
	${NSD_CreateCheckBox} 55% 80u 40% 8u "Smarty (.smarty)"
	Pop $FA_Smarty
	${NSD_CreateCheckBox} 55% 90u 40% 8u "VisualBasic Script (.vbs; .vb)"
	Pop $FA_Vbs
	${NSD_CreateCheckBox} 55% 100u 40% 8u "XHTML (.xhtml)"
	Pop $FA_Xhtml
	${NSD_CreateCheckBox} 55% 110u 40% 8u "XML (.xml; .xsl)"
	Pop $FA_Xml
	
	${NSD_Check} $FA_BFProject
	${NSD_Check} $FA_BFLang2

	${NSD_CreateCheckBox} 40% 130u 30% 8u "$(FA_SELECT)"
	Pop $FA_SelectAll
	${NSD_OnClick} $FA_SelectAll FileAssociations_SelectAll

	nsDialogs::Show
FunctionEnd

Function SetFileAssociations
	;                     HWND					Extension	Mime Type									Handler	Content Type	ICON Id
	${RegisterFileType} $FA_Ada 			"ada" 		"text/x-ada" 								"bfadafile"	"$(CT_ADA)"	1
	${RegisterFileType} $FA_Asp 			"asp" 		"text/x-asp" 								"bfaspfile" "$(CT_ASP)"	2
	${RegisterFileType} $FA_Sh 			"sh" 			"text/x-shellscript" 					"bfshfile"	"$(CT_SH)"	22
	${RegisterFileType} $FA_BFProject 	"bfproject" "application/x-bluefish-project" 	"bfprojectfile"	"$(CT_BFPROJECT)"	4
	${RegisterFileType} $FA_BFLang2 		"bflang2" 	"application/x-bluefish-language2" 	"bflang2file"	"$(CT_BFLANG2)"	3
	${RegisterFileType} $FA_C 				"c" 			"text/x-csrc" 								"bfcfile"	"$(CT_C)"	5
	${RegisterFileType} $FA_C 				"h" 			"text/x-chdr" 								"bfhfile"	"$(CT_H)"	10
	${RegisterFileType} $FA_Cpp 			"cpp" 		"text/x-c++src" 							"bfcppfile"	"$(CT_CPP)"	6
	${RegisterFileType} $FA_Cpp 			"cxx" 		"text/x-c++src" 							"bfcppfile"	"$(CT_CPP)"	6
	${RegisterFileType} $FA_Cpp 			"cc" 			"text/x-c++src" 							"bfcppfile"	"$(CT_CPP)"	6
	${RegisterFileType} $FA_Cpp 			"hpp" 		"text/x-c++hdr" 							"bfhppfile"	"$(CT_HPP)"	11
	${RegisterFileType} $FA_Css 			"css" 		"text/css" 									"bfcssfile" "$(CT_CSS)"	7
	${RegisterFileType} $FA_D 				"d" 			"text/x-dsrc" 								"bfdfile"	"$(CT_D)"	8
	${RegisterFileType} $FA_Po 			"po" 			"text/x-gettext-translation" 			"bfpofile"	"$(CT_PO)"	9

	${RegisterHTMLType} $FA_Html
	${RegisterFileType} $FA_Html 			"htm" 		"text/html" 								"0"	"0"	"0"
	${RegisterFileType} $FA_Html 			"html" 		"text/html" 								"0"	"0"	"0"

	${RegisterFileType} $FA_Java 			"java" 		"text/x-java" 								"bfjavafile"	"$(CT_JAVA)"	13
	${RegisterFileType} $FA_Js 			"js" 			"application/javascript" 				"bfjsfile"	"$(CT_JS)"	14
	${RegisterFileType} $FA_Jsp 			"jsp" 		"application/x-jsp" 						"bfjspfile"	"$(CT_JSP)"	15
	${RegisterFileType} $FA_Nsi 			"nsi" 		"text/x-nsi" 								"bfnsifile"	"$(CT_NSI)"	17
	${RegisterFileType} $FA_Nsi 			"nsh" 		"text/x-nsh" 								"bfnshfile"	"$(CT_NSH)"	16
	${RegisterFileType} $FA_Pl 			"pl" 			"application/x-perl" 					"bfplfile"	"$(CT_PL)"	18
	${RegisterFileType} $FA_Php 			"php" 		"application/x-php" 						"bfphpfile"	"$(CT_PHP)"	19
	${RegisterFileType} $FA_Php 			"php3" 		"application/x-php" 						"bfphpfile"	"$(CT_PHP)"	19
	${RegisterFileType} $FA_Txt 			"txt" 		"text/plain" 								"bftxtfile"	"$(CT_TXT)"	24
	${RegisterFileType} $FA_Py 			"py" 			"text/x-python" 							"bfpyfile"	"$(CT_PY)"	20
	${RegisterFileType} $FA_Rb 			"rb" 			"text/x-ruby" 								"bfrbfile"	"$(CT_RB)"	21
	${RegisterFileType} $FA_Smarty 		"smarty" 	"application/x-smarty" 					"bfsmartyfile"	"$(CT_SMARTY)"	23
	${RegisterFileType} $FA_Vbs 			"vbs" 		"application/x-vbscript" 				"bfvbsfile"	"$(CT_VBS)"	25
	${RegisterFileType} $FA_Vbs 			"vb" 			"application/x-vbscript" 				"bfvbsfile" "$(CT_VBS)"	25
	${RegisterFileType} $FA_Xhtml 		"xhtml" 		"application/xhtml+xml" 				"bfxhtmlfile"	"$(CT_XHTML)"	26
	${RegisterFileType} $FA_Xml 			"xml" 		"text/xml" 									"bfxmlfile"	"$(CT_XML)"	27
	${RegisterFileType} $FA_Xml 			"xsl" 		"text/xml" 									"bfxslfile"	"$(CT_XSL)"	28
	${RegisterFileType} $FA_Xml 			"xslt" 		"text/xml" 									"bfxslfile"	"$(CT_XSL)"	28
FunctionEnd

Function FileAssociations_SelectAll
	${NSD_GetState} $FA_SelectAll $R0
	${If} $R0 == ${BST_CHECKED}
		${NSD_SetText} $FA_SelectAll "$(FA_UNSELECT)"
		${NSD_Check} $FA_Ada
		${NSD_Check} $FA_Asp
		${NSD_Check} $FA_Sh
		${NSD_Check} $FA_BFProject
		${NSD_Check} $FA_BFLang2
		${NSD_Check} $FA_C
		${NSD_Check} $FA_Cpp
		${NSD_Check} $FA_Css
		${NSD_Check} $FA_D
		${NSD_Check} $FA_Po
		${NSD_Check} $FA_Html
		${NSD_Check} $FA_Java
		${NSD_Check} $FA_Js
		${NSD_Check} $FA_Jsp
		${NSD_Check} $FA_Nsi
		${NSD_Check} $FA_Pl
		${NSD_Check} $FA_Php
		${NSD_Check} $FA_Txt
		${NSD_Check} $FA_Py
		${NSD_Check} $FA_Rb
		${NSD_Check} $FA_Smarty
		${NSD_Check} $FA_Vbs
		${NSD_Check} $FA_Xhtml
		${NSD_Check} $FA_Xml
	${Else}
		${NSD_SetText} $FA_SelectAll "$(FA_SELECT)"
		${NSD_Uncheck} $FA_Ada
		${NSD_Uncheck} $FA_Asp
		${NSD_Uncheck} $FA_Sh
		${NSD_Uncheck} $FA_BFProject
		${NSD_Uncheck} $FA_BFLang2
		${NSD_Uncheck} $FA_C
		${NSD_Uncheck} $FA_Cpp
		${NSD_Uncheck} $FA_Css
		${NSD_Uncheck} $FA_D
		${NSD_Uncheck} $FA_Po
		${NSD_Uncheck} $FA_Html
		${NSD_Uncheck} $FA_Java
		${NSD_Uncheck} $FA_Js
		${NSD_Uncheck} $FA_Jsp
		${NSD_Uncheck} $FA_Nsi
		${NSD_Uncheck} $FA_Pl
		${NSD_Uncheck} $FA_Php
		${NSD_Uncheck} $FA_Txt
		${NSD_Uncheck} $FA_Py
		${NSD_Uncheck} $FA_Rb
		${NSD_Uncheck} $FA_Smarty
		${NSD_Uncheck} $FA_Vbs
		${NSD_Uncheck} $FA_Xhtml
		${NSD_Uncheck} $FA_Xml
	${EndIf}
FunctionEnd

Function DisableBackButton
	Push $0
	GetDlGItem $0 $HWNDPARENT 3
	EnableWindow $0 0
	Pop $0
FunctionEnd

Function GtkVersionCheck
	Push $R0
	Push $R1
	Push $R2
	; Get the current installed version of GTK+ from the registry
	ReadRegStr $R0 HKLM "Software\GTK\2.0" "Version"
	; If we were unable to retrieve the current GTK+ version from the registry
	;  we can assume that it is not currently installed
	StrLen $R1 $R0
	${If} $R1 == 0
		StrCpy $GTK_STATUS ""
	${Else}
		; Check the major version
		${StrTok} $R1 $R0 "." "0" "1"
		${StrTok} $R2 ${GTK_MIN_VERSION} "." "0" "1"
		${If} $R1 >= $R2
			; Check the minor version
			${StrTok} $R1 $R0 "." "1" "1"
			${StrTok} $R2 ${GTK_MIN_VERSION} "." "1" "1"
			${If} $R1 >= $R2
				; Check the patch level
				${StrTok} $R1 $R0 "." "2" "1"
				${StrTok} $R2 ${GTK_MIN_VERSION} "." "2" "1"
				${If} $R1 >= $R2
					StrCpy $GTK_STATUS "installed"
				${Else}
					StrCpy $GTK_STATUS ""
				${EndIf}
			${Else}
				StrCpy $GTK_STATUS ""
			${EndIf}
		${Else}
			StrCpy $GTK_STATUS ""
		${EndIf}
	${EndIf}
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Function GtkInstallPath
	Push $R0
	Push $R1
	Push $R2
	Push $R3
	DetailPrint "$(GTK_PATH)"
	; Get the current system path variable from the registry
	ReadRegStr $R2 HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path"
	; Get the installation path of GTK+ from the registry
	ReadRegStr $R3 HKLM "Software\GTK\2.0" "DllPath"
	; Check if the GTK+ path is found in the current system path
	${StrStr} $R0 $R2 $R3
	; If the GTK+ path was not found in the system path ${StrStr} will return an
	;  empty string and we should update the system path
	StrLen $R1 $R0
	${If} $R1 == 0
		StrCpy $R2 "$R2;$R3"
		; Write the updated system path to the registry
		WriteRegExpandStr HKLM "System\CurrentControlSet\Control\Session Manager\Environment" "Path" $R2
		; Alert the system that an environment variable has been changed so it propagates
		SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} "0" "STR:Environment" /TIMEOUT=5000
	${EndIf}
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd


; Uninstaller Functions
;----------------------------------------------
Function un.onInit
	!insertmacro MUI_UNGETLANGUAGE
FunctionEnd

Function un.UnRegisterFileTypes
	Push $0
	Push $1
	Push $R0
	Push $R1
	
	StrCpy $1 0
	StrCpy $0 "init"
	${While} $0 != ""
		EnumRegKey $0 HKLM "${REG_UNINSTALL}\Backup\HKCR" $1
		IntOp $1 $1 + 1
		${If} $0 != ""
			ReadRegStr $R0 HKLM "${REG_UNINSTALL}\Backup\HKCR\$0" ""
			${If} $R0 == ""
				DeleteRegKey HKCR $0
			${Else}
				WriteRegStr HKCR $0 "" $R0
			${EndIf}
			ReadRegStr $R1 HKLM "{REG_UNINSTALL\Backup\HKCR\$0" "Content Type"
			${If} $R1 != ""
				WriteRegStr HKCR $0 "Content Type" $R1
			${EndIf}
		${EndIf}
	${EndWhile}	
	
	Pop $R1
	Pop $R0
	Pop $1
	Pop $0
FunctionEnd

Function un.UnRegisterHTML
	Push $R1
	Push $R2
	Push $R3
	
	ReadRegStr $R1 HKLM "${REG_UNINSTALL}\Backup\HTML" "Description"
	${If} $R1 != ""
		WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" $R1
	${EndIf}
	ReadRegStr $R2 HKLM "${REG_UNINSTALL}\Backup\HTML" "command"
	${If} $R2 != ""
		WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" $R2
	${EndIf}
	ReadRegStr $R3 HKLM "${REG_UNINSTALL}\Backup\HTML" "command2"
	${If} $R3 != ""
		WriteRegStr HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" $R3
	${EndIf}

	Pop $R3
	Pop $R2
	Pop $R1
FunctionEnd
