;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Dutch Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Olivier Sessink <olivier@bluefish.openoffice.nl>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Bureaublad snelkoppeling"
!define SECT_DICT "Spelling woordenboeken (Internet verbinding nodig voor de download)"

; License Page
!define LICENSEPAGE_BUTTON "Volgende"
!define LICENSEPAGE_FOOTER "${PRODUCT} wordt uitgebracht onder de GNU General Public License. De licentie is beschikbaar ter info. $_CLICK"

; General Download Messages
; !define DOWN_LOCAL "Local copy of %s found..."
; !define DOWN_CHKSUM "Checksum verified..."
; !define DOWN_CHKSUM_ERROR "Checksum mismatch..."

; Aspell Strings
!define DICT_INSTALLED "Laatste versie van dit woordenboek is geinstalleerd, nergeer download voor:"
!define DICT_DOWNLOAD "Download spelling checker woordenboek..."
!define DICT_FAILED "Woordenboek download mislukt:"
!define DICT_EXTRACT "Woordenboek uitpakken..."

; GTK+ Strings
!define GTK_DOWNLOAD "Download GTK+ bibliotheek..."
!define GTK_FAILED "GTK+ bibliotheek download mislukt:"
!define GTK_INSTALL "Installeer GTK+ bibliotheek..."
; !define GTK_UNINSTALL "Uninstalling GTK+..."
!define GTK_PATH "Installeer GTK+ bibliotheek in het systeempad."
; !define GTK_REQUIRED "Please install GTK+ ${GTK_MIN_VERSION} or higher and make sure it is in your PATH before running Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Karacter map"
!define PLUG_ENTITIES "Entities"
!define PLUG_HTMLBAR "HTML Balk"
!define PLUG_INFBROWSER "Info Browser"
!define PLUG_SNIPPETS "Snippers"
; !define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Bestands koppeling"
!define FA_HEADER "Selecteer de bestandstypen die met ${PRODUCT} geopend moeten worden."
!define FA_SELECT "Alles selecteren"
!define FA_UNSELECT "Alles deselecteren"

; Misc
!define FINISHPAGE_LINK "Bezoek de Bluefish website"
!define UNINSTALL_SHORTCUT "Verwijder ${PRODUCT}"
; !define FILETYPE_REGISTER "Registering File Type:"
; !define UNSTABLE_UPGRADE "An unstable release of ${PRODUCT} is installed.$\nShould previous versions be removed before we continue (Recommended)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
; !define INETC_DOWN "Downloading %s"
; !define INETC_CONN "Connecting ..."
; !define INETC_TSEC "second"
; !define INETC_TMIN "minute"
; !define INETC_THOUR "hour"
; !define INETC_TPLUR "s"
; !define INETC_PROGRESS "%dkB (%d%%) of %dkB @ %d.%01dkB/s"
; !define INETC_REMAIN " (%d %s%s remaining)"

; Content Types
!define CT_ADA	"Ada broncode"
!define CT_ASP "ActiveServer Page Script"
!define CT_SH	"Bash Shell Script"
!define CT_BFPROJECT	"Bluefish Project"
!define CT_BFLANG2	"Bluefish Language Definition File Version 2"
!define CT_C	"C Broncode"
!define CT_H	"C Header"
!define CT_CPP	"C++ Broncode"
!define CT_HPP	"C++ Header"
!define CT_CSS "Cascading Stylesheet"
!define CT_D	"D Broncode"
; !define CT_DIFF "Diff/Patch File"
!define CT_PO	"Gettext Translation"
!define CT_JAVA	"Java Broncode"	
!define CT_JS	"JavaScript Script"
!define CT_JSP	"JavaServer Pages Script"
; !define CT_MW	"MediaWiki File"
!define CT_NSI	"NSIS Script"
!define CT_NSH	"NSIS Header"
!define CT_PL	"Perl Script"
!define CT_PHP	"PHP Script"
; !define CT_INC	"PHP Include Script"
!define CT_TXT	"Plain Text"
!define CT_PY	"Python Script"
!define CT_RB	"Ruby Script"
!define CT_SMARTY	"Smarty Script"
!define CT_VBS	"VisualBasic Script"
!define CT_XHTML	"XHTML Bestand"
!define CT_XML	"XML Bestand"
!define CT_XSL	"XML Stylesheet"
