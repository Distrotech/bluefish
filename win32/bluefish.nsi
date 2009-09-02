Name "Bluefish"
OutFile "bluefish-setup.exe"

!define BLUEFISH_UNINST_EXE				"bluefish-uninst.exe"
!define BLUEFISH_REG_KEY					"SOFTWARE\bluefish"

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
	File "local\bin\bluefish-unstable.exe"

	SetOverwrite on
	WriteUninstaller "$INSTDIR\${BLUEFISH_UNINST_EXE}"
	SetOverwrite off
SectionEnd

SectionGroup /e "Bluefish shortcuts"  SecShortcuts
	Section /o "Desktop" SecDesktopShortcut
		SetOverwrite on
		CreateShortCut "$DESKTOP\Bluefish.lnk" "$INSTDIR\bluefish.exe"
		SetOverwrite off
	SectionEnd
	Section "Startmenu" SecStartMenuShortcut
		SetOverwrite on
		CreateShortCut "$SMPROGRAMS\Bluefish.lnk" "$INSTDIR\bluefish.exe"
		SetOverwrite off
	SectionEnd
SectionGroupEnd

Section "Uninstall Bluefish" un.SecUninstall
	Delete "$INSTDIR\bluefish-unstable.exe"
	RMDir  "$INSTDIR"
	Delete "$DESKTOP\Bluefish.lnk"
SectionEnd

Function .onInit
	StrCpy $INSTDIR "$PROGRAMFILES\Bluefish"
FunctionEnd