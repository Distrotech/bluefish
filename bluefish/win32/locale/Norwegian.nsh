;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  English Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Christian Tellefsen <chris@tellefsen.net>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Snarvei på skrivebordet"
!define SECT_DICT "Stavekontroll (Krever internettilkobling for å laste ned ordlister)"

; License Page
!define LICENSEPAGE_BUTTON "Neste"
!define LICENSEPAGE_FOOTER "${PRODUCT} er lisensiert etter GNU General Public License. Lisensen er tilgjengelig for gjennomsyn. $_CLICK"

; Aspell Strings
!define DICT_INSTALLED "Siste versjon av denne ordboken er allerede installert. Trenger ikke laste ned:"
!define DICT_DOWNLOAD "Laster ned ordliste for stavekontroll..."
!define DICT_FAILED "Nedlasting av ordliste feilet:"
!define DICT_EXTRACT "Pakker ut ordliste..."

; GTK+ Strings
!define GTK_DOWNLOAD "Laster ned GTK+..."
!define GTK_FAILED "Nedlasting av GTK+ feilet:"
!define GTK_INSTALL "Installerer GTK+..."
!define GTK_PATH "Installerer GTK+ i system path."

; Plugin Names
!define PLUG_CHARMAP "Tegnkart"
!define PLUG_ENTITIES "Entiteter"
!define PLUG_HTMLBAR "HTML verktøylinje"
!define PLUG_INFBROWSER "Info Browser"
!define PLUG_SNIPPETS "Utklipp"

; File Associations Page
!define FA_TITLE "Filtypeassosiasjoner"
!define FA_HEADER "Velg filtypene du ønsker at ${PRODUCT} skal være standardprogram for."
!define FA_SELECT "Velg alle"
!define FA_UNSELECT "Fjern alle"

; Misc
!define FINISHPAGE_LINK "Besøk nettsiden vår."
!define UNINSTALL_SHORTCUT "Avinstaller ${PRODUCT}"

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
