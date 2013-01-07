;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Norwegian Bokmål Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Christian Tellefsen <chris@tellefsen.net>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Programmtillegg"
!define SECT_SHORTCUT "Snarvei på skrivebordet"
!define SECT_DICT "Stavekontroll (Krever internettilkobling for å laste ned ordlister)"

; License Page
!define LICENSEPAGE_BUTTON "Neste"
!define LICENSEPAGE_FOOTER "${PRODUCT} er lisensiert etter GNU General Public License. Lisensen er tilgjengelig for gjennomsyn. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Fant lokal kopi av %s ..."
!define DOWN_CHKSUM "Sjekksummen er kontrollert ..."
!define DOWN_CHKSUM_ERROR "Sjekksumfeil ..."

; Aspell Strings
!define DICT_INSTALLED "Siste versjon av denne ordboken er allerede installert. Laster ikke ned:"
!define DICT_DOWNLOAD "Laster ned stavekontrollordlista ..."
!define DICT_FAILED "Klarte ikke å laste ned ordlista:"
!define DICT_EXTRACT "Pakker ut ordliste..."

; GTK+ Strings
!define GTK_DOWNLOAD "Laster ned GTK+..."
!define GTK_FAILED "Nedlasting av GTK+ feilet:"
!define GTK_INSTALL "Installerer GTK+..."
!define GTK_UNINSTALL "Avinstallerer GTK+..."
!define GTK_REQUIRED "Installer GTK+ 2.14.7 eller nyere og sikre at biblioteket finnes i systemfilbanen (PATH) før du starter Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Tegnkart"
!define PLUG_ENTITIES "Entiteter"
!define PLUG_HTMLBAR "HTML-verktøylinje"
!define PLUG_INFBROWSER "Info Browser"
!define PLUG_SNIPPETS "Utklipp"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Filtypeassosiasjoner"
!define FA_HEADER "Velg filtypene du ønsker at ${PRODUCT} skal håndtere."
!define FA_SELECT "Velg alle"
!define FA_UNSELECT "Fjern alle"

; Misc
!define FINISHPAGE_LINK "Besøk nettsiden for Bluefish."
!define UNINSTALL_SHORTCUT "Avinstaller ${PRODUCT}"
!define FILETYPE_REGISTER "Registrerer filtypen:"
!define UNSTABLE_UPGRADE "En eksperimentell utgave av ${PRODUCT} er allerede installert.$\nVil du fjerne denne utgaven? (Anbefales)"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Laster ned %s"
!define INETC_CONN "Kobler til ..."
!define INETC_TSEC "sekund"
!define INETC_TMIN "minutt"
!define INETC_THOUR "time"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) av %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s gjenstår)"

; Content Types
!define CT_ADA	"Ada Source File"
!define CT_ASP "ActiveServer Page Script"
!define CT_SH	"Bash Shell Script"
!define CT_BFPROJECT	"Bluefish-prosjekt"
!define CT_BFLANG2	"Bluefish språkdefinisjonsfil versjon 2"
!define CT_C	"C Source File"
!define CT_H	"C Header File"
!define CT_CPP	"C++ Source File"
!define CT_HPP	"C++ Header File"
!define CT_CSS "Cascading Stylesheet"
!define CT_D	"D Source File"
!define CT_DIFF "Diff/Patch Fil"
!define CT_PO	"Gettext Translation"
!define CT_JAVA	"Java Source File"	
!define CT_JS	"JavaScript Script"
!define CT_JSP	"JavaServer Pages Script"
!define CT_MW	"MediaWiki File"
!define CT_NSI	"NSIS Script"
!define CT_NSH	"NSIS Header File"
!define CT_PL	"Perl Script"
!define CT_PHP	"PHP Script"
!define CT_INC	"PHP Include Script"
!define CT_TXT	"Plain Text"
!define CT_PY	"Python Script"
!define CT_RB	"Ruby Script"
!define CT_SMARTY	"Smarty Script"
!define CT_VBS	"VisualBasic Script"
!define CT_XHTML	"XHTML File"
!define CT_XML	"XML File"
!define CT_XSL	"XML Stylesheet"
