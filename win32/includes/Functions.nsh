;----------------------------------------------
; Bluefish Windows NSIS Macros Header
; [Functions.nsh]
; 
;  Copyright (C) 2009-2014 The Bluefish Developers
;   Shawn Novak <Kernel86@gmail.com>
;   Daniel Leidert <daniel.leidert@wgdd.de>
;----------------------------------------------

!define BF_FILE_CLASSES "bf_must_be_first,bfdifffile,bfmwfile,bfadafile,bfaspfile,bfshfile,bfprojectfile,bflang2file,bfcfile,bfhfile,bfcppfile,bfhppfile,bfcssfile,bfdfile,bfpofile,bfjavafile,bfjsfile,bfjspfile,bfnsifile,bfnshfile,bfplfile,bfphpfile,bftxtfile,bfpyfile,bfrbfile,bfsmartyfile,bfvbsfile,bfxhtmlfile,bfxmlfile,bfxslfile"
Function FileAssociations
	!insertmacro MUI_HEADER_TEXT "$(FA_TITLE)" "$(FA_HEADER)"
	nsDialogs::Create 1018
	Pop $FA_Dialog
	${If} $FA_Dialog == "error"
		Abort
	${EndIf}

	${NSD_CreateCheckBox} 5% 0u 40% 8u "Ada (.ada)"
	Pop $FA_Ada
	${SelectIfRegistered} $FA_Ada "ada"
	${NSD_CreateCheckBox} 5% 10u 40% 8u "ActiveServer Pages (.asp)"
	Pop $FA_Asp
	${SelectIfRegistered} $FA_Asp "asp"
	${NSD_CreateCheckBox} 5% 20u 40% 8u "Bash (.sh)"
	Pop $FA_Sh
	${SelectIfRegistered} $FA_Sh "sh"
	${NSD_CreateCheckBox} 5% 30u 40% 8u "Bluefish Project (.bfproject)"
	Pop $FA_BFProject
	${NSD_Check} $FA_BFProject
	${NSD_CreateCheckBox} 5% 40u 40% 8u "Bluefish Language (.bflang2)"
	Pop $FA_BFLang2
	${NSD_Check} $FA_BFLang2
	${NSD_CreateCheckBox} 5% 50u 40% 8u "C (.c; .h)"
	Pop $FA_C
	${SelectIfRegistered} $FA_C "c"
	${SelectIfRegistered} $FA_C "h"
	${NSD_CreateCheckBox} 5% 60u 40% 8u "C++ (.cpp; .cxx; .cc; .hpp)"
	Pop $FA_Cpp
	${SelectIfRegistered} $FA_Cpp "cpp"
	${SelectIfRegistered} $FA_Cpp "cxx"
	${SelectIfRegistered} $FA_Cpp "cc"
	${SelectIfRegistered} $FA_Cpp "hpp"
	${NSD_CreateCheckBox} 5% 70u 40% 8u "CSS (.css)"
	Pop $FA_Css
	${SelectIfRegistered} $FA_Css "css"
	${NSD_CreateCheckBox} 5% 80u 40% 8u "D (.d)"
	Pop $FA_D
	${SelectIfRegistered} $FA_D "d"
	${NSD_CreateCheckBox} 5% 90u 40% 8u "Diff (.diff; .patch)"
	Pop $FA_Diff
	${SelectIfRegistered} $FA_Diff "diff"
	${SelectIfRegistered} $FA_Diff "patch"
	${NSD_CreateCheckBox} 5% 100u 40% 8u "Gettext PO (.po)"
	Pop $FA_Po
	${SelectIfRegistered} $FA_Po "po"
	${NSD_CreateCheckBox} 5% 110u 40% 8u "HTML (.htm; .html)"
	Pop $FA_Html
	${SelectIfRegisteredHTML}
	${NSD_CreateCheckBox} 5% 120u 40% 8u "Java (.java)"
	Pop $FA_Java
	${SelectIfRegistered} $FA_Java "java"

	${NSD_CreateCheckBox} 55% 0u 40% 8u "JavaScript (.js)"
	Pop $FA_Js
	${SelectIfRegistered} $FA_Js "js"
	${NSD_CreateCheckBox} 55% 10u 40% 8u "JavaServer Pages (.jsp)"
	Pop $FA_Jsp
	${SelectIfRegistered} $FA_Jsp "jsp"
	${NSD_CreateCheckBox} 55% 20u 40% 8u "MediaWiki File (.mw)"
	Pop $FA_Mw
	${SelectIfRegistered} $FA_Mw "mw"
	${NSD_CreateCheckBox} 55% 30u 40% 8u "NSIS (.nsi; .nsh)"
	Pop $FA_Nsi
	${SelectIfRegistered} $FA_Nsi "nsi"
	${SelectIfRegistered} $FA_Nsi "nsh"
	${NSD_CreateCheckBox} 55% 40u 40% 8u "Perl (.pl)"
	Pop $FA_Pl
	${SelectIfRegistered} $FA_Pl "pl"
	${NSD_CreateCheckBox} 55% 50u 40% 8u "PHP (.php; .php3)"
	Pop $FA_Php
	${SelectIfRegistered} $FA_Php "php"
	${SelectIfRegistered} $FA_Php "php3"
	${NSD_CreateCheckBox} 55% 60u 40% 8u "Plain Text (.txt)"
	Pop $FA_Txt
	${SelectIfRegistered} $FA_Txt "txt"
	${NSD_CreateCheckBox} 55% 70u 40% 8u "Python (.py)"
	Pop $FA_Py
	${SelectIfRegistered} $FA_Py "py"
	${NSD_CreateCheckBox} 55% 80u 40% 8u "Ruby (.rb)"
	Pop $FA_Rb
	${SelectIfRegistered} $FA_Rb "rb"
	${NSD_CreateCheckBox} 55% 90u 40% 8u "Smarty (.tpl)"
	Pop $FA_Smarty
	${SelectIfRegistered} $FA_Smarty "tpl"
	${NSD_CreateCheckBox} 55% 100u 40% 8u "VisualBasic Script (.vbs; .vb)"
	Pop $FA_Vbs
	${SelectIfRegistered} $FA_Vbs "vbs"
	${SelectIfRegistered} $FA_Vbs "vb"
	${NSD_CreateCheckBox} 55% 110u 40% 8u "XHTML (.xhtml)"
	Pop $FA_Xhtml
	${SelectIfRegistered} $FA_Xhtml "xhtml"
	${NSD_CreateCheckBox} 55% 120u 40% 8u "XML (.xml; .xsl)"
	Pop $FA_Xml
	${SelectIfRegistered} $FA_Xml "xml"
	${SelectIfRegistered} $FA_Xml "xsl"

	${NSD_CreateCheckBox} 40% 130u 30% 8u "$(FA_SELECT)"
	Pop $FA_SelectAll
	${NSD_OnClick} $FA_SelectAll FileAssociations_SelectAll

	nsDialogs::Show
FunctionEnd

; Note: Make sure to add or remove handlers from the BF_FILE_CLASSES define when changing
Function SetFileAssociations
	;                     HWND			Extension	Mime Type		Handler	Content Type	ICON Id
	${RegisterFileType} $FA_Ada  	"ada" 	"text/x-ada" 			"bfadafile" "$(CT_ADA)" 1
	${RegisterFileType} $FA_Asp  	"asp" 	"text/x-asp" 			"bfaspfile" "$(CT_ASP)" 2
	${RegisterFileType} $FA_Sh  	"sh" 		"text/x-shellscript" 		"bfshfile" "$(CT_SH)" 22
	${RegisterFileType} $FA_BFProject 	"bfproject" 	"application/x-bluefish-project" 	"bfprojectfile" "$(CT_BFPROJECT)" 4
	${RegisterFileType} $FA_BFLang2 	"bflang2" 	"application/x-bluefish-language2" 		"bflang2file" "$(CT_BFLANG2)" 3
	${RegisterFileType} $FA_C  		"c" 		"text/x-csrc" 			"bfcfile" "$(CT_C)" 5
	${RegisterFileType} $FA_C  		"h" 		"text/x-chdr" 			"bfhfile" "$(CT_H)" 10
	${RegisterFileType} $FA_Cpp 	"cpp" 	"text/x-c++src" 		"bfcppfile" "$(CT_CPP)" 6
	${RegisterFileType} $FA_Cpp 	"cxx" 	"text/x-c++src" 		"bfcppfile" "$(CT_CPP)" 6
	${RegisterFileType} $FA_Cpp 	"cc" 		"text/x-c++src" 		"bfcppfile" "$(CT_CPP)" 6
	${RegisterFileType} $FA_Cpp 	"hpp" 	"text/x-c++hdr" 		"bfhppfile" "$(CT_HPP)" 11
	${RegisterFileType} $FA_Css 	"css" 	"text/css" 			"bfcssfile" "$(CT_CSS)" 7
	${RegisterFileType} $FA_D 		"d" 		"text/x-dsrc" 			"bfdfile" "$(CT_D)" 8
	${RegisterFileType} $FA_Diff 	"diff" 	"text/x-diff" 			"bfdifffile" "$(CT_DIFF)" 100
	${RegisterFileType} $FA_Diff 	"patch" 	"text/x-patch" 			"bfdifffile" "$(CT_DIFF)" 100
	${RegisterFileType} $FA_Po  	"po" 		"text/x-gettext-translation" 	"bfpofile" "$(CT_PO)" 9
	${RegisterFileType} $FA_Java 	"java" 	"text/x-java" 			"bfjavafile" "$(CT_JAVA)" 13
	${RegisterFileType} $FA_Jsp 	"jsp" 	"application/x-jsp" 		"bfjspfile" "$(CT_JSP)" 15
	${RegisterFileType} $FA_Mw  	"mw" 		"text/x-mediawiki" 		"bfmwfile" "$(CT_MW)" 100
	${RegisterFileType} $FA_Nsi 	"nsi" 	"text/x-nsi" 			"bfnsifile" "$(CT_NSI)" 17
	${RegisterFileType} $FA_Nsi 	"nsh" 	"text/x-nsh" 			"bfnshfile" "$(CT_NSH)" 16
	${RegisterFileType} $FA_Pl  	"pl" 		"application/x-perl" 		"bfplfile" "$(CT_PL)" 18
	${RegisterFileType} $FA_Php 	"php" 	"application/x-php" 		"bfphpfile" "$(CT_PHP)" 19
	${RegisterFileType} $FA_Php 	"php3" 	"application/x-php" 		"bfphpfile" "$(CT_PHP)" 19
	${RegisterFileType} $FA_Txt 	"txt" 	"text/plain" 			"bftxtfile" "$(CT_TXT)" 24
	${RegisterFileType} $FA_Py  	"py" 		"text/x-python" 		"bfpyfile" "$(CT_PY)" 20
	${RegisterFileType} $FA_Rb  	"rb" 		"text/x-ruby" 			"bfrbfile" "$(CT_RB)" 21
	${RegisterFileType} $FA_Smarty 	"tpl" 	"application/x-smarty" 		"bfsmartyfile" "$(CT_SMARTY)" 23
	${RegisterFileType} $FA_Vbs 	"vb" 	"application/x-vbscript" 		"bfvbsfile" "$(CT_VBS)" 25
	${RegisterFileType} $FA_Xhtml 	"xhtml" 	"application/xhtml+xml" 		"bfxhtmlfile" "$(CT_XHTML)" 26
	${RegisterFileType} $FA_Xml 	"xml" 	"text/xml" 			"bfxmlfile" "$(CT_XML)" 27
	${RegisterFileType} $FA_Xml 	"xsl" 	"application/xslt+xml" 		"bfxslfile" "$(CT_XSL)" 28
	${RegisterFileType} $FA_Xml 	"xslt" 	"application/xslt+xml" 		"bfxslfile" "$(CT_XSL)" 28

    ; HTML is a special case
	${RegisterHTMLType} $FA_Html
	${RegisterFileType} $FA_Html	"htm" 	"text/html" 		"0"	"0"	"0"
	${RegisterFileType} $FA_Html	"html" 	"text/html" 		"0"	"0"	"0"

	; Extra special cases
	${RegisterFileType} $FA_Js  	"js" 		"application/javascript" 		"bfjsfile" "$(CT_JS)" 14
	${RegisterFileType} $FA_Vbs 	"vbs" 		"application/x-vbscript" 		"bfvbsfile" "$(CT_VBS)" 25
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
		${NSD_Check} $FA_Diff
		${NSD_Check} $FA_Po
		${NSD_Check} $FA_Html
		${NSD_Check} $FA_Java
		${NSD_Check} $FA_Js
		${NSD_Check} $FA_Jsp
		${NSD_Check} $FA_Mw
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
		${NSD_Uncheck} $FA_Diff
		${NSD_Uncheck} $FA_Po
		${NSD_Uncheck} $FA_Html
		${NSD_Uncheck} $FA_Java
		${NSD_Uncheck} $FA_Js
		${NSD_Uncheck} $FA_Jsp
		${NSD_Uncheck} $FA_Mw
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
	; Get the current user's installed version of GTK+ from the registry
	${If} $HKEY == "Classic"
		ReadRegStr $R0 HKCU "${REG_USER_SET}" "GTK"
	${Else}
		ReadRegStr $R0 HKLM "${REG_USER_SET}" "GTK"
	${EndIf}

	; If we were unable to retrieve the current GTK+ version from the registry
	;  we can assume that it is not currently installed
	StrLen $R1 $R0
	${If} $R1 == 0
		StrCpy $GTK_STATUS ""
	${Else}
		; Check the major version
		${StrTok} $R1 $R0 "." "0" "1"
		${StrTok} $R2 ${GTK_MIN_VERSION} "." "0" "1"
		${If} $R1 == $R2
			; Check the minor version
			${StrTok} $R1 $R0 "." "1" "1"
			${StrTok} $R2 ${GTK_MIN_VERSION} "." "1" "1"
			${If} $R1 == $R2
				; Check the patch level
				${StrTok} $R1 $R0 "." "2" "1"
				${StrTok} $R2 ${GTK_MIN_VERSION} "." "2" "1"
				${If} $R1 >= $R2
					StrCpy $GTK_STATUS "installed"
				${Else}
					StrCpy $GTK_STATUS ""
				${EndIf}
			${ElseIf} $R1 > $R2
				StrCpy $GTK_STATUS "installed"
			${Else}
				StrCpy $GTK_STATUS ""
			${EndIf}
		${ElseIf} $R1 > $R2
			StrCpy $GTK_STATUS "installed"	
		${Else}
			StrCpy $GTK_STATUS ""
		${EndIf}
	${EndIf}
	Pop $R2
	Pop $R1
	Pop $R0
FunctionEnd

Function PythonVersionCheck
; Check if Python is installed as the Current User
	StrCpy $0 0
PyClassLoop:
	EnumRegKey $1 HKCU "Software\Python\PythonCore" $0
	StrCmp $1 "" PyAdmin 0 ; If not installed as CU check Admin
	IntOp $0 $0 + 1
	StrCmp $1 "2.7" PyFinish PyClassLoop

; Check if Python is installed as Local Administrator
PyAdmin:
	StrCpy $0 0
PyAdminLoop:
	EnumRegKey $1 HKLM "Software\Python\PythonCore" $0
	StrCmp $1 "" PyFail 0 ; If not, Python is not installed
	IntOp $0 $0 + 1
	StrCmp $1 "2.7" PyFinish PyAdminLoop

PyFail:
	StrCpy $PYTHON_STATUS ""
	Return
PyFinish:
	StrCpy $PYTHON_STATUS "installed"
	Return
FunctionEnd
