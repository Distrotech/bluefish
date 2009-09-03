Name "Bluefish"
OutFile "bluefish-setup.exe"
RequestExecutionLevel admin

!define BLUEFISH_UNINST_EXE				"bluefish-uninst.exe"
!define BLUEFISH_REG_KEY					"SOFTWARE\Bluefish"

!define MSYS_PATH "E:\MSYS"

SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show

!include "MUI2.nsh"
!include "Sections.nsh"
!include "WinVer.nsh"
!include "LogicLib.nsh"

!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions
!insertmacro GetParent

!include "WordFunc.nsh"
!insertmacro VersionCompare
!insertmacro WordFind
!insertmacro un.WordFind

ReserveFile "${NSISDIR}\Plugins\InstallOptions.dll"
!insertmacro MUI_RESERVEFILE_LANGDLL

; MUI configuration
;!define MUI_ICON								".\bluefish-install.ico"
;!define MUI_UNICON							".\bluefish-install.ico"
;!define MUI_WELCOMEFINISHPAGE_BITMAP	".\bluefish-install.bmp"
;!define MUI_HEADERIMAGE
;!define MUI_HEADERIMAGE_BITMAP			".\bluefish-header.bmp"

!define MUI_LICENSEPAGE_BUTTON			"Next >"
!define MUI_LICENSEPAGE_TEXT_BOTTOM		"$(^Name) is released under the GNU General Public License (GPL). The license is provided here for information purposes only. $_CLICK"

;!define MUI_LANGDLL_REGISTRY_ROOT 		"HKCU"
;!define MUI_LANGDLL_REGISTRY_KEY 		${BLUEFISH_REG_KEY}
;!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

!define MUI_COMPONENTSPAGE_SMALLDESC
!define MUI_ABORTWARNING

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN					"$INSTDIR\bluefish-unstable.exe"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_LINK				"Visit the Bluefish Homepage"
!define MUI_FINISHPAGE_LINK_LOCATION	"http://bluefish.openoffice.nl"

; MUI pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE				"../COPYING"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "1.3.7.1"
VIAddVersionKey "ProductName" "Bluefish"
VIAddVersionKey "FileVersion" "1.3.7"
VIAddVersionKey "ProductVersion" "1.3"
VIAddVersionKey "LegalCopyright" ""
VIAddVersionKey "FileDescription" "Bluefish Installer (w/o GTK+ Installer)"

Section "Bluefish Editor" SecBluefish
	SectionIn 1 RO

	SetOutPath "$INSTDIR"
	SetOverwrite on
	File "${MSYS_PATH}\local\bin\bluefish-unstable.exe"
	File "bluefish.ico"
	File "${MSYS_PATH}\local\bin\libaspell-15.dll"
	File "${MSYS_PATH}\local\bin\libenchant-1.dll"
	File "${MSYS_PATH}\local\bin\libgnurx-0.dll"
	File "${MSYS_PATH}\local\bin\libpcre-0.dll"
	File "${MSYS_PATH}\local\bin\libxml2-2.dll"
	SetOutPath "$INSTDIR\lib\bluefish-unstable"
	File /r "${MSYS_PATH}\local\lib\bluefish-unstable\*.dll"
	SetOutPath "$INSTDIR\lib\enchant"
	File "${MSYS_PATH}\local\lib\enchant\libenchant_aspell.dll"
	SetOutPath "$INSTDIR\lib\aspell-0.60"
	File "${MSYS_PATH}\local\lib\aspell-0.60\*.cmap"
	File "${MSYS_PATH}\local\lib\aspell-0.60\*.cset"
	File "${MSYS_PATH}\local\lib\aspell-0.60\*.amf"
	File "${MSYS_PATH}\local\lib\aspell-0.60\*.kbd"
	File "${MSYS_PATH}\local\lib\aspell-0.60\spell"
	File "${MSYS_PATH}\local\lib\aspell-0.60\ispell"
	SetOutPath "$INSTDIR\share\bluefish-unstable"
	File /r "${MSYS_PATH}\local\share\bluefish-unstable\*"
	SetOutPath "$INSTDIR\share\enchant"
	File /r "${MSYS_PATH}\local\share\enchant\*"

	SetOutPath "$INSTDIR"

	SetOverwrite on
	WriteUninstaller "$INSTDIR\${BLUEFISH_UNINST_EXE}"
	SetOverwrite off
SectionEnd

SectionGroup /e "Bluefish shortcuts"  SecShortcuts
	Section /o "Desktop" SecDesktopShortcut
		SetOverwrite on
		CreateShortCut "$DESKTOP\Bluefish.lnk" "$INSTDIR\bluefish-unstable.exe" "" "$INSTDIR\bluefish.ico" 0
		SetOverwrite off
	SectionEnd
	Section "Startmenu" SecStartMenuShortcut
		SetOverwrite on
		CreateDirectory "$SMPROGRAMS\Bluefish"
		CreateShortCut "$SMPROGRAMS\Bluefish\Bluefish.lnk" "$INSTDIR\bluefish-unstable.exe" "" "$INSTDIR\bluefish.ico" 0
		CreateShortCut "$SMPROGRAMS\Bluefish\Uninstall Bluefish.lnk" "$INSTDIR\bluefish-uninst.exe"
		SetOverwrite off
	SectionEnd
SectionGroupEnd

Section "Uninstall"
	Delete "$INSTDIR\bluefish-unstable.exe"
	Delete "$INSTDIR\bluefish.ico"
	Delete "$INSTDIR\libaspell-15.dll"
	Delete "$INSTDIR\libenchant-1.dll"
	Delete "$INSTDIR\libgnurx-0.dll"
	Delete "$INSTDIR\libpcre-0.dll"
	Delete "$INSTDIR\libxml2-2.dll"
	RMDir /r "$INSTDIR\lib"
	RMDir /r "$INSTDIR\share"
	Delete "$INSTDIR\bluefish-uninst.exe"
	RMDir  "$INSTDIR"
	Delete "$SMPROGRAMS\Bluefish\Bluefish.lnk"
	Delete "$SMPROGRAMS\Bluefish\Uninstall Bluefish.lnk"
	RMDir "$SMPROGRAMS\Bluefish"
SectionEnd

Function .onInit
	StrCpy $INSTDIR "$PROGRAMFILES\Bluefish"
FunctionEnd