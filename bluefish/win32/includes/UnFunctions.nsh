;----------------------------------------------
; Bluefish Windows NSIS Macros Header
; [UnFunctions.nsh]
; 
;  Copyright (C) 2009-2010 The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------

Function un.UnRegisterFileTypes
	Push $0
	Push $1
	Push $R0
	Push $R1
	Push $R2
	Push $R3
	Push $R4

	StrCpy $1 0
	StrCpy $0 "init"
	${While} $0 != ""
		${If} $HKEY == "HKLM"
		${OrIf} $HKEY == "Classic"
			EnumRegKey $0 HKLM "${REG_UNINSTALL}\Backup\HKCR" $1
			IntOp $1 $1 + 1
			${If} $0 != ""
				ReadRegStr $R0 HKLM "${REG_UNINSTALL}\Backup\HKCR\$0" "" ; Read stored class
				ReadRegStr $R2 HKCR $0 "" ; Read current class
				${UnStrLoc} $R3 ${BF_FILE_CLASSES} $R2 "<" ; Check if current class is a Bluefish class
				StrLen $R4 ${BF_FILE_CLASSES}
				${If} $R0 == $R2 ; If the current class is the same as we stored
				${OrIf} $R3 != $R4 ; Or if the current class is a Bluefish class continue restoring the stored class
					${If} $R0 == "" ; If the stored class is unset simply remove it
						DeleteRegKey HKCR $0
					${ElseIf} $R0 != $R2 ; Else the stored class needs to be restored
						WriteRegStr HKCR $0 "" $R0
					${EndIf} ; Stored class has been restored
					ClearErrors ; make sure we don't have any old errors lingering about
					ReadRegStr $R2 HKCR $0 "Content Type" ; Try to read the current mimetype
					IfErrors +8 ; If we deleted the key we'll get an error and should skip restoring the mimetype, else restore it
						ClearErrors
						ReadRegStr $R1 HKLM "${REG_UNINSTALL}\Backup\HKCR\$0" "Content Type" ; Read stored mimetype
							${If} $R1 != "" ; Restore stored mimetype
							WriteRegStr HKCR $0 "Content Type" $R1
						${Else} ; We didn't store a mimetype so we should unset this value
							DeleteRegValue HKCR $0 "Content Type"
						${EndIf} ; Stored mimetype has been restored
				${EndIf} ; The current class wasn't a Bluefish class so don't touch it
			${EndIf}
		${Else}
			EnumRegKey $0 HKCU "${REG_UNINSTALL}\Backup\HKCR" $1
			IntOp $1 $1 + 1
			${If} $0 != ""
				ReadRegStr $R0 HKCU "${REG_UNINSTALL}\Backup\HKCR\$0" "" ; Read stored class
				ReadRegStr $R2 HKCU "${REG_CLASS_SET}\$0" "" ; Read current class
				${UnStrLoc} $R3 ${BF_FILE_CLASSES} $R2 "<" ; Check if current class is a Bluefish class
				StrLen $R4 ${BF_FILE_CLASSES}
				${If} $R0 == $R2 ; If the current class is the same as we stored
				${OrIf} $R3 != $R4 ; Or if the current class is a Bluefish class continue restoring the stored class
					${If} $R0 == "" ; If the stored class is unset simply remove it
						DeleteRegKey HKCU "${REG_CLASS_SET}\$0"
					${ElseIf} $R0 != $R2 ; Else the stored class needs to be restored
						WriteRegStr HKCU "${REG_CLASS_SET}\$0" "" $R0
					${EndIf} ; Stored class has been restored
					ClearErrors ; make sure we don't have any old errors lingering about
					ReadRegStr $R2 HKCU "${REG_CLASS_SET}\$0" "Content Type" ; Try to read the current mimetype
					IfErrors +8 ; If we deleted the key we'll get an error and should skip restoring the mimetype, else restore it
						ClearErrors
						ReadRegStr $R1 HKCU "${REG_UNINSTALL}\Backup\HKCR\$0" "Content Type" ; Read stored mimetype
							${If} $R1 != "" ; Restore stored mimetype
							WriteRegStr HKCU "${REG_CLASS_SET}\$0" "Content Type" $R1
						${Else} ; We didn't store a mimetype so we should unset this value
							DeleteRegValue HKCU "${REG_CLASS_SET}\$0" "Content Type"
						${EndIf} ; Stored mimetype has been restored
				${EndIf} ; The current class wasn't a Bluefish class so don't touch it
			${EndIf}
		${EndIf}
	${EndWhile}

	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
	Pop $1
	Pop $0
FunctionEnd

Function un.UnRegisterHTML
	Push $R1
	Push $R2
	Push $R3
	${If} $HKEY == "HKLM"
		ReadRegStr $R1 HKLM "${REG_UNINSTALL}\Backup\HTML" "Description"
		${If} $R1 != ""
			WriteRegStr HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" $R1
		${EndIf}
		ReadRegStr $R2 HKLM "${REG_UNINSTALL}\Backup\HTML" "command"
		${If} $R2 != ""
			WriteRegStr HKLM "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" $R2
		${EndIf}
		ReadRegStr $R3 HKLM "${REG_UNINSTALL}\Backup\HTML" "command2"
		${If} $R3 != ""
			WriteRegStr HKLM "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" $R3
		${EndIf}
	${ElseIf} $HKEY == "Classic"
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
	${Else}
		ReadRegStr $R1 HKCU "${REG_UNINSTALL}\Backup\HTML" "Description"
		${If} $R1 != ""
			WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor" "Description" $R1
		${EndIf}
		ReadRegStr $R2 HKCU "${REG_UNINSTALL}\Backup\HTML" "command"
		${If} $R2 != ""
			WriteRegStr HKCU "Software\Microsoft\Internet Explorer\Default HTML Editor\shell\edit\command" "" $R2
		${EndIf}
		ReadRegStr $R3 HKCU "${REG_UNINSTALL}\Backup\HTML" "command2"
		${If} $R3 != ""
			WriteRegStr HKCU "Software\Microsoft\Shared\HTML\Default Editor\shell\edit\command" "" $R3
		${EndIf}
	${EndIf}
	Pop $R3
	Pop $R2
	Pop $R1
FunctionEnd
