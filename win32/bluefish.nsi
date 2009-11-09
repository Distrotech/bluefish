;----------------------------------------------
; Bluefish Windows NSIS Install Script
; 
;  The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
; 
;----------------------------------------------


; Defines
;----------------------------------------------
;!define PACKAGE
;!define LOCALE
;!define VERSION

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

SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show


; Installer version information
;----------------------------------------------
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${PRODUCT}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "1.3"
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
;!define MUI_ICON						".\bluefish-install.ico"
;!define MUI_UNICON						".\bluefish-install.ico"
;!define MUI_WELCOMEFINISHPAGE_BITMAP	".\bluefish-install.bmp"
;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP			".\bluefish-header.bmp"

;!define MUI_LANGDLL_REGISTRY_ROOT 		"HKCU"
;!define MUI_LANGDLL_REGISTRY_KEY 		${REG_USER_SET}
;!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_FINISHPAGE_NOREBOOTSUPPORT
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_LICENSEPAGE_BUTTON			"Next >"
!define MUI_LICENSEPAGE_TEXT_BOTTOM		"$(^Name) is released under the GNU General Public License (GPL). The license is provided here for information purposes only. $_CLICK"

!define MUI_STARTMENUPAGE_DEFAULTFOLDER		"${PRODUCT}"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT		"HKCU"
!define MUI_STARTMENUPAGE_REGISTRY_KEY 		${REG_USER_SET}
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME	"Start Menu Folder"

;!define MUI_FINISHPAGE_RUN					"$INSTDIR\${PROGRAM_EXE}"
;!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_LINK				"Visit the Bluefish Homepage"
!define MUI_FINISHPAGE_LINK_LOCATION	"http://bluefish.openoffice.nl"


; Macros
;----------------------------------------------
!macro InstallAspellDict LANG VER
	ReadRegStr $R0 HKCU "${REG_USER_SET}\Aspell\${LANG}" ""
	${If} $R0 == ${VER}
		DetailPrint "Latest ${LANG} dictionary is installed, skipping download. "
	${Else}
		DetailPrint "Downloading Spellchecker Dictionary... (${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2)"
		NSISdl::download "${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2" "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		Pop $R0
			StrCmp $R0 "success" +3
				MessageBox MB_OK "Dictionary Download failed: $R0"
				Quit
		DetailPrint "Extracting Dictionary... (aspell6-${LANG}-${VER}.tbz2)"
		untgz::extract -d $INSTDIR -zbz2 -u "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		WriteRegStr HKCU "${REG_USER_SET}\Aspell\${LANG}" "" "${VER}"
		Delete "$TEMP\aspell6-${LANG}-${VER}.tbz2"
	${EndIf}
!macroend
!define InstallAspellDict `!insertmacro InstallAspellDict`

!macro RegisterFileType HWND EXT TYPE PROG DESC
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
; Icon set coming soon
;			WriteRegStr HKCR "${PROG}\DefaultIcon" "" ""
			WriteRegStr HKCR "${PROG}\shell" "" "open"
			WriteRegStr HKCR "${PROG}\shell\open" "" "Open"
			WriteRegStr HKCR "${PROG}\shell\open\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
			; This is just so the un.UnRegisterFileTypes function removes all the bf*file entries
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\${PROG}" "" ""
		${EndIf}
	${EndIf}
!macroend
!define RegisterFileType `!insertmacro RegisterFileType`


; Pages
;----------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE				"..\COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU			"Bluefish"	$StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
Page custom FileAssociations SetFileAssociations
!define MUI_PAGE_CUSTOMFUNCTION_SHOW	DisableBackButton
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE 					"English"


; Sections
;----------------------------------------------
Section "Bluefish Editor" SecBluefish
	SectionIn 1 RO

	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "build\${PROGRAM_EXE}"
	File "bluefish.ico"
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

	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" 		"${PRODUCT} ${VERSION}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayIcon" 		"$INSTDIR\bluefish.ico"
	WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" "$INSTDIR\${UNINSTALL_EXE}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "Publisher" 		"${PUBLISHER}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "URLInfoAbout" 	"${HOMEPAGE}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "HelpLink" 			"${HELPURL}"
	WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayVersion" 	"${VERSION}"
	WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoModify" "1"
	WriteRegDWORD HKLM "${REG_UNINSTALL}" "NoRepair" "1"

	!insertmacro MUI_STARTMENU_WRITE_BEGIN "Bluefish"
		SetOverwrite on
		CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}" "" "$INSTDIR\bluefish.ico" 0
		CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall ${PRODUCT}.lnk" "$INSTDIR\${UNINSTALL_EXE}"
		SetOverwrite off
	!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "-GTK+ Installer" SecGTK
	${If} $GTK_STATUS == ""
		DetailPrint "Downloading GTK+... (${GTK_URL}/${GTK_FILENAME})"
		NSISdl::download "${GTK_URL}/${GTK_FILENAME}" "$TEMP\${GTK_FILENAME}"
		Pop $R0
			StrCmp $R0 "success" +3
				MessageBox MB_OK "GTK+ Download failed: $R0"
				Quit
		DetailPrint "Installing GTK+... (${GTK_FILENAME})"
		ExecWait '"$TEMP\${GTK_FILENAME}"'
		Delete "$TEMP\${GTK_FILENAME}"
		Call GtkInstallPath
	${Else}
		Call GtkInstallPath
	${EndIf}
SectionEnd

SectionGroup "Plugins" SecPlugins
	SetOverwrite on
	Section "Charmap" SecPlCharmap
		SetOutPath "$INSTDIR"
		File "build\libgucharmap-7.dll"
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\charmap.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "Entities" SecPlEntities
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\entities.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "HTMLbar" SecPlHTMLbar
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\htmlbar.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_infbrowser.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "InfBrowser" SecPlInfBrowser
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\infbrowser.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_snippets.mo" "build\share\locale\*"
	SectionEnd
	Section "Snippets" SecPlSnippets
		SetOutPath "$INSTDIR\lib\${PACKAGE}"
		File "build\lib\${PACKAGE}\snippets.dll"
		SetOutPath "$INSTDIR\share\locale"
		File /r /x "${PACKAGE}.mo" /x "*_about.mo" /x "*_charmap.mo" /x "*_entities.mo" /x "*_htmlbar.mo" /x "*_infbrowser.mo" "build\share\locale\*"
	SectionEnd
	SetOverwrite off
SectionGroupEnd

Section "Desktop Shortcut" SecDesktopShortcut
	SetOverwrite on
	SetOutPath "$INSTDIR"
	CreateShortCut "$DESKTOP\${PRODUCT}.lnk" "$INSTDIR\${PROGRAM_EXE}" "" "$INSTDIR\bluefish.ico" 0
	SetOverwrite off
SectionEnd

SectionGroup /e "Spell-checking Languages" SecLang
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
	Section /o "German" SecLangDe
		${InstallAspellDict} "de" "20030222"
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
	Delete "$INSTDIR\bluefish.ico"
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
	Delete "$SMPROGRAMS\$R0\Uninstall ${PRODUCT}.lnk"
	RMDir "$SMPROGRAMS\$R0"
	${EndIf}
	DeleteRegValue HKCU ${REG_USER_SET} "Start Menu Folder"
	DeleteRegKey HKCU "${REG_USER_SET}\Aspell"
	DeleteRegKey /ifempty HKCU ${REG_USER_SET}

	Call un.UnRegisterFileTypes

	DeleteRegKey HKLM "${REG_UNINSTALL}"
SectionEnd


; Installer Functions
;----------------------------------------------
Function .onInit
	Call GtkVersionCheck
	${If} $GTK_STATUS == ""	
		SectionSetSize ${SecGTK} ${GTK_SIZE}	; 6.69MB Download
	${EndIf}

	SectionSetSize ${SecLangDa} 12394		; 3.86MB Download
	SectionSetSize ${SecLangNl} 4430		; 1.54MB Download
	SectionSetSize ${SecLangEn} 3557		; 1.21MB Download
	SectionSetSize ${SecLangFi} 2750		; 680KB Download
	SectionSetSize ${SecLangFr} 17762		; 5.55MB Download
	SectionSetSize ${SecLangDe} 11355		; 3.63MB Download
	SectionSetSize ${SecLangIt} 2119		; 690KB Download
	SectionSetSize ${SecLangNb} 11040		; 3.47MB Download
	SectionSetSize ${SecLangPt_PT} 734		; 273KB Download
	SectionSetSize ${SecLangPt_BR} 7946	; 2.54MB Download
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
FunctionEnd

Function FileAssociations
	!insertmacro MUI_HEADER_TEXT "File Associations" "Select the file types which you would like ${PRODUCT} to be the default editor."
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

	${NSD_CreateCheckBox} 40% 130u 30% 8u "Select All"
	Pop $FA_SelectAll
	${NSD_OnClick} $FA_SelectAll FileAssociations_SelectAll

	nsDialogs::Show
FunctionEnd

Function SetFileAssociations
	;                     HWND					Extension	Mime Type									Handler	Content Type
	${RegisterFileType} $FA_Ada 			"ada" 		"text/x-ada" 								"bfadafile"	"Ada Source File"
	${RegisterFileType} $FA_Asp 			"asp" 		"text/x-asp" 								"bfaspfile" "ActiveServer Page Script"
	${RegisterFileType} $FA_Sh 			"sh" 			"text/x-shellscript" 					"bfshfile"	"Bash Shell Script"
; How do we open projects?
	${RegisterFileType} $FA_BFProject 	"bfproject" "application/x-bluefish-project" 	"bfprojectfile"	"Bluefish Project"
	${RegisterFileType} $FA_BFLang2 		"bflang2" 	"application/x-bluefish-language2" 	"bflang2file"	"Bluefish Language Definition File Version 2"
	${RegisterFileType} $FA_C 				"c" 			"text/x-csrc" 								"bfcfile"	"C Source File"
	${RegisterFileType} $FA_C 				"h" 			"text/x-chdr" 								"bfhfile"	"C Header File"
	${RegisterFileType} $FA_Cpp 			"cpp" 		"text/x-c++src" 							"bfcppfile"	"C++ Source File"
	${RegisterFileType} $FA_Cpp 			"cxx" 		"text/x-c++src" 							"bfcppfile"	"C++ Source File"
	${RegisterFileType} $FA_Cpp 			"cc" 			"text/x-c++src" 							"bfcppfile"	"C++ Source File"
	${RegisterFileType} $FA_Cpp 			"hpp" 		"text/x-c++hdr" 							"bfhppfile"	"C++ Header File"
	${RegisterFileType} $FA_Css 			"cpp" 		"text/css" 									"bfcssfile" "Cascading Stylesheet"
	${RegisterFileType} $FA_D 				"d" 			"text/x-dsrc" 								"bfdfile"	"D Source File"
	${RegisterFileType} $FA_Po 			"po" 			"text/x-gettext-translation" 			"bfpofile"	"Gettext Translation"
; HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Default HTML Editor	Name
; HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command	@
	${RegisterFileType} $FA_Html 			"htm" 		"text/html" 								"0"	"0"
	${RegisterFileType} $FA_Html 			"html" 		"text/html" 								"0"	"0"
	${RegisterFileType} $FA_Java 			"java" 		"text/x-java" 								"bfjavafile"	"Java Source File"	
	${RegisterFileType} $FA_Js 			"js" 			"application/javascript" 				"bfjsfile"	"JavaScript Script"
	${RegisterFileType} $FA_Jsp 			"jsp" 		"application/x-jsp" 						"bfjspfile"	"JavaServer Pages Script"
	${RegisterFileType} $FA_Nsi 			"nsi" 		"text/x-nsi" 								"bfnsifile"	"NSIS Script"
	${RegisterFileType} $FA_Nsi 			"nsh" 		"text/x-nsh" 								"bfnshfile"	"NSIS Header File"
	${RegisterFileType} $FA_Pl 			"pl" 			"application/x-perl" 					"bfplfile"	"Perl Script"
	${RegisterFileType} $FA_Php 			"php" 		"application/x-php" 						"bfphpfile"	"PHP Script"
	${RegisterFileType} $FA_Php 			"php3" 		"application/x-php" 						"bfphpfile"	"PHP Script"
	${RegisterFileType} $FA_Txt 			"txt" 		"text/plain" 								"bftxtfile"	"Plain Text"
	${RegisterFileType} $FA_Py 			"py" 			"text/x-python" 							"bfpyfile"	"Python Script"
	${RegisterFileType} $FA_Rb 			"rb" 			"text/x-ruby" 								"bfrbfile"	"Ruby Script"
	${RegisterFileType} $FA_Smarty 		"smarty" 	"application/x-smarty" 					"bfsmartyfile"	"Smarty Script"
	${RegisterFileType} $FA_Vbs 			"vbs" 		"application/x-vbscript" 				"bfvbsfile"	"VisualBasic Script"
	${RegisterFileType} $FA_Vbs 			"vb" 			"application/x-vbscript" 				"bfvbsfile" "VisualBasic Script"
	${RegisterFileType} $FA_Xhtml 		"xhtml" 		"application/xhtml+xml" 				"bfxhtmlfile"	"XHTML File"
	${RegisterFileType} $FA_Xml 			"xml" 		"text/xml" 									"bfxmlfile"	"XML File"
	${RegisterFileType} $FA_Xml 			"xsl" 		"text/xml" 									"bfxslfile"	"XML Stylesheet"
FunctionEnd

Function FileAssociations_SelectAll
	${NSD_GetState} $FA_SelectAll $R0
	${If} $R0 == ${BST_CHECKED}
		${NSD_SetText} $FA_SelectAll "UnSelect All"
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
		${NSD_SetText} $FA_SelectAll "Select All"
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
	DetailPrint "Installing GTK+ in the system path."
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
