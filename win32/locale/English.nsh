;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  English Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Shawn Novak <Kernel86@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Desktop Shortcut"
!define SECT_DICT "Spellcheck Languages (Internet Connection Required for Download)"

; License Page
!define LICENSEPAGE_BUTTON "Next"
!define LICENSEPAGE_FOOTER "${PRODUCT} is released under the GNU General Public License. The license is provided here for informational purposes only. $_CLICK"

; Aspell Strings
!define DICT_INSTALLED "Latest version of this dictionary is installed, skipping download of:"
!define DICT_DOWNLOAD "Downloading spellchecker dictionary..."
!define DICT_FAILED "Dictionary download failed:"
!define DICT_EXTRACT "Extracting dictionary..."

; GTK+ Strings
!define GTK_DOWNLOAD "Downloading GTK+..."
!define GTK_FAILED "GTK+ Download failed:"
!define GTK_INSTALL "Installing GTK+..."
!define GTK_PATH "Installing GTK+ in the system path."

; Plugin Names
!define PLUG_CHARMAP "Character Map"
!define PLUG_ENTITIES "Entities"
!define PLUG_HTMLBAR "HTML Bar"
!define PLUG_INFBROWSER "Info Browser"
!define PLUG_SNIPPETS "Snippets"

; File Associations Page
!define FA_TITLE "File Associations"
!define FA_HEADER "Select the file types you would like ${PRODUCT} to be the default editor for."
!define FA_SELECT "Select All"
!define FA_UNSELECT "UnSelect All"

; Misc
!define FINISHPAGE_LINK "Visit the Bluefish Homepage"
!define UNINSTALL_SHORTCUT "Uninstall ${PRODUCT}"

; Content Types
!define CT_ADA	"Ada Source File"
!define CT_ASP "ActiveServer Page Script"
!define CT_SH	"Bash Shell Script"
!define CT_BFPROJECT	"Bluefish Project"
!define CT_BFLANG2	"Bluefish Language Definition File Version 2"
!define CT_C	"C Source File"
!define CT_H	"C Header File"
!define CT_CPP	"C++ Source File"
!define CT_HPP	"C++ Header File"
!define CT_CSS "Cascading Stylesheet"
!define CT_D	"D Source File"
!define CT_PO	"Gettext Translation"
!define CT_JAVA	"Java Source File"	
!define CT_JS	"JavaScript Script"
!define CT_JSP	"JavaServer Pages Script"
!define CT_NSI	"NSIS Script"
!define CT_NSH	"NSIS Header File"
!define CT_PL	"Perl Script"
!define CT_PHP	"PHP Script"
!define CT_TXT	"Plain Text"
!define CT_PY	"Python Script"
!define CT_RB	"Ruby Script"
!define CT_SMARTY	"Smarty Script"
!define CT_VBS	"VisualBasic Script"
!define CT_XHTML	"XHTML File"
!define CT_XML	"XML File"
!define CT_XSL	"XML Stylesheet"
