;----------------------------------------------
; Bluefish Windows NSIS Macros Header
; [Macros.nsh]
; 
;  Copyright (C) 2009-2014 The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------

!macro InstallAspellDict LANG VER
	${If} $HKEY == "HKCU"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R0 HKCU "${REG_USER_SET}\Aspell\${LANG}" ""
	${Else}
		ReadRegStr $R0 HKLM "${REG_USER_SET}\Aspell\${LANG}" ""
	${EndIf}

	${If} $R0 == ${VER}
		DetailPrint "$(DICT_INSTALLED) ${LANG}"
	${Else}
		IfFileExists "$EXEDIR\redist\aspell6-${LANG}-${VER}.tbz2" 0 +15
			${StrRep} $R1 "$(DOWN_LOCAL)" "%s" "aspell6-${LANG}-${VER}.tbz2"
			DetailPrint "$R1"
			md5dll::GetMD5File "$EXEDIR\redist\aspell6-${LANG}-${VER}.tbz2"
  			Pop $R0
  			${If} $R0 == ${MD5_${LANG}_${VER}}
  				DetailPrint "$(DOWN_CHKSUM)"
  				untgz::extract "-d" "$INSTDIR" "-u" "-zbz2" "$EXEDIR\redist\aspell6-${LANG}-${VER}.tbz2"
  				Pop $R0
  				Goto +17 ; Jump to 'StrCmp $R0 "success"'
  			${Else}
  				DetailPrint "$(DOWN_CHKSUM_ERROR)"
  				Goto +2 ; Jump to '${EndIf}'+1
  			${EndIf}

		DetailPrint "$(DICT_DOWNLOAD) (${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2)"
		Delete "$TEMP\aspell6-${LANG}-${VER}.tbz2" ; Should never happen but just in case
		inetc::get /TRANSLATE "$(INETC_DOWN)" "$(INETC_CONN)" "$(INETC_TSEC)" "$(INETC_TMIN)" "$(INETC_THOUR)" "$(INETC_TPLUR)" "$(INETC_PROGRESS)" "$(INETC_REMAIN)" "${AS_DICT_URL}/aspell6-${LANG}-${VER}.tbz2" "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		Pop $R0
		StrCmp $R0 "OK" +4
			MessageBox MB_OK|MB_ICONEXCLAMATION "$(DICT_FAILED) $R0" ; Alert the user that the download failed
			Delete "$TEMP\aspell6-${LANG}-${VER}.tbz2" ; Remove possible partially downloaded file
			Return ; Failed to download a dictionary, leave gracefully
		DetailPrint "$(DICT_EXTRACT) (aspell6-${LANG}-${VER}.tbz2)"
		untgz::extract "-d" "$INSTDIR" "-u" "-zbz2" "$TEMP\aspell6-${LANG}-${VER}.tbz2"
		Pop $R0
		StrCmp $R0 "success" 0 +7
			${If} $HKEY == "HKCU"
			${OrIf} $HKEY == "Classic"
				WriteRegStr HKCU "${REG_USER_SET}\Aspell\${LANG}" "" "${VER}"
			${Else}
				WriteRegStr HKLM "${REG_USER_SET}\Aspell\${LANG}" "" "${VER}"
			${EndIf}
		Delete "$TEMP\aspell6-${LANG}-${VER}.tbz2"
	${EndIf}
!macroend
!define InstallAspellDict `!insertmacro InstallAspellDict`

!macro SelectIfRegistered HWND EXT
	${If} $HKEY == "HKLM"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R0 HKCR ".${EXT}" ""
	${Else}
		ReadRegStr $R0 HKCU "${REG_CLASS_SET}\.${EXT}" ""
	${EndIf}

	${If} $R0 != ""
		${StrLoc} $R1 ${BF_FILE_CLASSES} ",$R0" "<"

		StrLen $R2 ${BF_FILE_CLASSES}
		${If} $R1 > 0
		${AndIf} $R1 < $R2
		${AndIf} $R1 != $R2
			${NSD_Check} ${HWND}
		${EndIf}
	${EndIf}
!macroend
!define SelectIfRegistered `!insertmacro SelectIfRegistered`

!macro SelectIfRegisteredHTML
	${If} $HKEY == "HKCU"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R0 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description"
	${Else}
		ReadRegStr $R0 HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description"
	${EndIf}
	
	${If} $R0 == "${PRODUCT}"
		${NSD_Check} $FA_Html
	${EndIf}
!macroend
!define SelectIfRegisteredHTML `!insertmacro SelectIfRegisteredHTML`

!macro RegisterFileType HWND EXT TYPE PROG DESC ICON
	StrCpy $PROG "${PROG}"

	${If} $HKEY == "HKLM"
	${OrIf} $HKEY == "Classic"
		ReadRegStr $R1 HKCR ".${EXT}" "Content Type" ; Read the current mimetype
		ReadRegStr $R2 HKCR ".${EXT}" "" ; Read the current class
		${If} $R1 != "${TYPE}" ; If the current mimetype is the same as what we want skip changing it
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "Content Type" $R1
			WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "" $R2
			WriteRegStr HKCR ".${EXT}" "Content Type" "${TYPE}"
		${EndIf} ; Proper mimetype has been set
;		${NSD_GetState} ${HWND} $R0 ; Read the status of the checkbox for this file type
		${If} $PROG != "0" ; If set to 0 this denotes a special case such as for HTML and we need to skip this section
;			${If} $R0 == ${BST_CHECKED} ; The user has selected to associate this file type with Bluefish
			${If} ${HWND} == 1
;				DetailPrint "$(FILETYPE_REGISTERING)${DESC}..." ; Let the user know we're registering this file type
				; The following should only be needed once and could be in the above IF block with some more checks
				${If} $PROG == "bfvbsfile"
					StrCpy $PROG "VBSFile"
					StrCpy $SHELLCMD "Edit"
				${ElseIf} $PROG == "bfjsfile"
					StrCpy $PROG "JSFile"
					StrCpy $SHELLCMD "Edit"
				${Else}
					StrCpy $SHELLCMD "Open"
					${If} $R2 != "$PROG" ; If the current class is different that ours set it for Bluefish
						WriteRegStr HKCR ".${EXT}" "" "$PROG"
					${EndIf} ; Else the class is already set for Bluefish so we needn't change it
					WriteRegStr HKCR "$PROG" "" "${DESC}"
					WriteRegStr HKCR "$PROG\DefaultIcon" "" "$INSTDIR\${PROGRAM_EXE},${ICON}"
					WriteRegStr HKCR "$PROG\shell" "" "$SHELLCMD"
				${EndIf}
				WriteRegStr HKCR "$PROG\shell\$SHELLCMD" "" "$SHELLCMD"
				WriteRegStr HKCR "$PROG\shell\$SHELLCMD\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""

				; This is just so the un.UnRegisterFileTypes function removes all the bf*file entries
				WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HKCR\$PROG" "" ""
			${EndIf} ; Bluefish will not be associated with this file type
		${EndIf}
	${Else}
		ReadRegStr $R1 HKCU "${REG_CLASS_SET}\.${EXT}" "Content Type" ; Read the current mimetype
		ReadRegStr $R2 HKCU "${REG_CLASS_SET}\.${EXT}" "" ; Read the current class
		${If} $R1 != "${TYPE}" ; If the current mimetype is the same as what we want skip changing it
			WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "Content Type" $R1
			WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HKCR\.${EXT}" "" $R2
			WriteRegStr HKCU "${REG_CLASS_SET}\.${EXT}" "Content Type" "${TYPE}"
		${EndIf} ; Proper mimetype has been set
;		${NSD_GetState} ${HWND} $R0 ; Read the status of the checkbox for this file type
		${If} $PROG != "0" ; If set to 0 this denotes a special case such as for HTML and we need to skip this section
;			${If} $R0 == ${BST_CHECKED} ; The user has selected to associate this file type with Bluefish
		${If} ${HWND} == 1
;				DetailPrint "$(FILETYPE_REGISTERING)${DESC}..." ; Let the user know we're registering this file type
				; The following should only be needed once and could be in the above IF block with some more checks
				${If} $PROG == "bfvbsfile"
					StrCpy $PROG "VBSFile"
					StrCpy $SHELLCMD "Edit"
				${ElseIf} $PROG == "bfjsfile"
					StrCpy $PROG "JSFile"
					StrCpy $SHELLCMD "Edit"
				${Else}
					StrCpy $SHELLCMD "Open"
					${If} $R2 != "${PROG}" ; If the current class is different that ours set it for Bluefish
						WriteRegStr HKCU "${REG_CLASS_SET}\.${EXT}" "" "$PROG"
					${EndIf} ; Else the class is already set for Bluefish so we needn't change it
					WriteRegStr HKCU "${REG_CLASS_SET}\$PROG" "" "${DESC}"
					WriteRegStr HKCU "${REG_CLASS_SET}\$PROG\DefaultIcon" "" "$INSTDIR\${PROGRAM_EXE},${ICON}"
					WriteRegStr HKCU "${REG_CLASS_SET}\$PROG\shell" "" "$SHELLCMD"
				${EndIf}
				WriteRegStr HKCU "${REG_CLASS_SET}\$PROG\shell\$SHELLCMD" "" "$SHELLCMD"
				WriteRegStr HKCU "${REG_CLASS_SET}\$PROG\shell\$SHELLCMD\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""

				; This is just so the un.UnRegisterFileTypes function removes all the bf*file entries
				WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HKCR\$PROG" "" ""
			${EndIf} ; Bluefish will not be associated with this file type
		${EndIf}
	${EndIf}
!macroend
!define RegisterFileType `!insertmacro RegisterFileType`

!macro RegisterHTMLType HWND
	${NSD_GetState} ${HWND} $R0
	${If} $R0 == ${BST_CHECKED}
		${If} $HKEY == "HKLM"
		${OrIf} $HKEY == "Classic"
			ReadRegStr $R1 HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description"
			${If} $R1 != "${PRODUCT}"
				WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "Description" $R1
				${If} $HKEY == "Classic"
					WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" "${PRODUCT}"
				${Else}
					WriteRegStr HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" "${PRODUCT}"
				${EndIf}

				${If} $HKEY == "Classic"
					ReadRegStr $R2 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" ""
					WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
				${Else}
					ReadRegStr $R2 HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" ""
					WriteRegStr HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
				${EndIf}
				WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "command" $R2

				${If} $HKEY == "Classic"
					ReadRegStr $R3 HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" ""
					WriteRegStr HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
				${Else}
					ReadRegStr $R3 HKLM "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" ""
					WriteRegStr HKLM "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
				${EndIf}
				WriteRegStr HKLM "${REG_UNINSTALL}\Backup\HTML" "command2" $R3
			${EndIf}
		${Else}
			ReadRegStr $R1 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description"
			${If} $R1 != "${PRODUCT}"
				WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HTML" "Description" $R1
				WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" "${PRODUCT}"

				ReadRegStr $R2 HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" ""
				WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HTML" "command" $R2
				WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""

				ReadRegStr $R3 HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" ""
				WriteRegStr HKCU "${REG_UNINSTALL}\Backup\HTML" "command2" $R3
				WriteRegStr HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" "$\"$INSTDIR\${PROGRAM_EXE}$\" $\"%1$\""
			${EndIf}
		${EndIf}
	${EndIf}
!macroend
!define RegisterHTMLType `!insertmacro RegisterHTMLType`

!macro Localize DEF LANG
	!if "${LANG}" == "ENGLISH"
		!define DEFAULT_${DEF} "${${DEF}}"
	!endif
	!ifdef "${DEF}"
		LangString "${DEF}" "${LANG_{$LANG}}" "${${DEF}}"
		!undef "${DEF}"
	!else
		LangString "${DEF}" "${LANG_{$LANG}}" "${DEFAULT_${DEF}}"
	!endif
!macroend

!macro LoadLocalization LANG INC
	!include "${INC}"

	!insertmacro Localize "LICENSEPAGE_BUTTON" "${LANG}"
	!insertmacro Localize "LICENSEPAGE_FOOTER" "${LANG}"

	!insertmacro Localize "FINISHPAGE_LINK" "${LANG}"

	!insertmacro Localize "DOWN_LOCAL" "${LANG}"
	!insertmacro Localize "DOWN_CHKSUM" "${LANG}"
	!insertmacro Localize "DOWN_CHKSUM_ERROR" "${LANG}"
	
	!insertmacro Localize "DICT_INSTALLED" "${LANG}"
	!insertmacro Localize "DICT_DOWNLOAD" "${LANG}"
	!insertmacro Localize "DICT_FAILED" "${LANG}"
	!insertmacro Localize "DICT_EXTRACT" "${LANG}"

	!insertmacro Localize "SECT_BLUEFISH" "${LANG}"
	!insertmacro Localize "UNINSTALL_SHORTCUT" "${LANG}"
	!insertmacro Localize "FILETYPE_REGISTER" "${LANG}"
	!insertmacro Localize "UNSTABLE_UPGRADE" "${LANG}"

	!insertmacro Localize "SECT_DEPENDS" "${LANG}"

	!insertmacro Localize "GTK_DOWNLOAD" "${LANG}"
	!insertmacro Localize "GTK_FAILED" "${LANG}"
	!insertmacro Localize "GTK_INSTALL" "${LANG}"
	!insertmacro Localize "GTK_UNINSTALL" "${LANG}"
	!insertmacro Localize "GTK_REQUIRED" "${LANG}"

	!insertmacro Localize "PYTHON_DOWNLOAD" "${LANG}"
	!insertmacro Localize "PYTHON_FAILED" "${LANG}"
	!insertmacro Localize "PYTHON_INSTALL" "${LANG}"
	!insertmacro Localize "PYTHON_REQUIRED" "${LANG}"

	!insertmacro Localize "SECT_PLUGINS" "${LANG}"
	!insertmacro Localize "PLUG_CHARMAP" "${LANG}"
	!insertmacro Localize "PLUG_ENTITIES" "${LANG}"
	!insertmacro Localize "PLUG_HTMLBAR" "${LANG}"
	!insertmacro Localize "PLUG_INFBROWSER" "${LANG}"
	!insertmacro Localize "PLUG_SNIPPETS" "${LANG}"
	!insertmacro Localize "PLUG_VCS" "${LANG}"
	!insertmacro Localize "PLUG_ZENCODING" "${LANG}"

	!insertmacro Localize "SECT_SHORTCUT" "${LANG}"

	!insertmacro Localize "SECT_DICT" "${LANG}"

	!insertmacro Localize "FA_TITLE" "${LANG}"
	!insertmacro Localize "FA_HEADER" "${LANG}"
	!insertmacro Localize "FA_SELECT" "${LANG}"
	!insertmacro Localize "FA_UNSELECT" "${LANG}"

	!insertmacro Localize "INETC_DOWN" "${LANG}"
	!insertmacro Localize "INETC_CONN" "${LANG}"
	!insertmacro Localize "INETC_TSEC" "${LANG}"
	!insertmacro Localize "INETC_TMIN" "${LANG}"
	!insertmacro Localize "INETC_THOUR" "${LANG}"
	!insertmacro Localize "INETC_TPLUR" "${LANG}"
	!insertmacro Localize "INETC_PROGRESS" "${LANG}"
	!insertmacro Localize "INETC_REMAIN" "${LANG}"

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
	!insertmacro Localize "CT_DIFF" "${LANG}"
	!insertmacro Localize "CT_PO" "${LANG}"
	!insertmacro Localize "CT_JAVA" "${LANG}"
	!insertmacro Localize "CT_JS" "${LANG}"
	!insertmacro Localize "CT_JSP" "${LANG}"
	!insertmacro Localize "CT_MW" "${LANG}"
	!insertmacro Localize "CT_NSI" "${LANG}"
	!insertmacro Localize "CT_NSH" "${LANG}"
	!insertmacro Localize "CT_PL" "${LANG}"
	!insertmacro Localize "CT_INC" "${LANG}"
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
