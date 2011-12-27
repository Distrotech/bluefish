;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Czech Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Marek Černocký <marek@manet.cz>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Zásuvné moduly"
!define SECT_SHORTCUT "Zástupce na plochu"
!define SECT_DICT "Kontrola pravopisu (ke stažení je vyžadováno Internetové připojení)"

; License Page
!define LICENSEPAGE_BUTTON "Další"
!define LICENSEPAGE_FOOTER "${PRODUCT} je vydán pod licencí GNU General Public License. Licence je zde uvedena pouze pro informační účely. $_CLICK"

; General Download Messages
; !define DOWN_LOCAL "Local copy of %s found..."
; !define DOWN_CHKSUM "Checksum verified..."
; !define DOWN_CHKSUM_ERROR "Checksum mismatch..."

; Aspell Strings
!define DICT_INSTALLED "Je nainstalovaná nejnovější verze tohoto slovníku, vynechá se stažení:"
!define DICT_DOWNLOAD "Stahuje se slovník pro kontrolu pravopisu..."
!define DICT_FAILED "Selhalo stažení slovníku:"
!define DICT_EXTRACT "Rozbaluje se slovník..."

; GTK+ Strings
!define GTK_DOWNLOAD "Stahuje se GTK+..."
!define GTK_FAILED "Selhalo stažení GTK+:"
!define GTK_INSTALL "Instaluje se GTK+..."
; !define GTK_UNINSTALL "Uninstalling GTK+..."
!define GTK_PATH "Instaluje se GTK+ do systémové cesty."
; !define GTK_REQUIRED "Please install GTK+ ${GTK_MIN_VERSION} or higher and make sure it is in your PATH before running Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa znaků"
!define PLUG_ENTITIES "Entity"
!define PLUG_HTMLBAR "Panel HTML"
!define PLUG_INFBROWSER "Prohlížeč informací"
!define PLUG_SNIPPETS "Úryvky"
; !define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Přidružení souborů"
!define FA_HEADER "Vyberte typy souborů, u kterých chcete, aby se jako výchozí otevíraly s editorem ${PRODUCT}."
!define FA_SELECT "Vybrat vše"
!define FA_UNSELECT "Zrušit výběr"

; Misc
!define FINISHPAGE_LINK "Zobrazit domovskou stránku Bluefish"
!define UNINSTALL_SHORTCUT "Odinstalovat ${PRODUCT}"
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
!define CT_ADA	"Zdrojový soubor jazyka Ada"
!define CT_ASP "Skript ActiveServer Page"
!define CT_SH	"Skript shellu Bash"
!define CT_BFPROJECT	"Projekt Bluefish"
!define CT_BFLANG2	"Soubor Bluefish s definicí jazyka verze 2"
!define CT_C	"Zdrojový soubor jazyka C"
!define CT_H	"Hlavičkový soubor jazyka C"
!define CT_CPP	"Zdrojový soubor jazyka C++"
!define CT_HPP	"Hlavičkový soubor jazyka C++"
!define CT_CSS "Kaskádový stylopis"
!define CT_D	"Zdrojový soubor jazyka D"
; !define CT_DIFF "Diff/Patch File"
!define CT_PO	"Překlad Gettext"
!define CT_JAVA	"Zdrojový soubor jazyka Java"	
!define CT_JS	"Skript JavaScript"
!define CT_JSP	"Skript JavaServer Pages"
; !define CT_MW	"MediaWiki File"
!define CT_NSI	"Skript NSIS"
!define CT_NSH	"Hlavičkový soubor NSIS"
!define CT_PL	"Skript Perl"
!define CT_PHP	"Skript PHP"
; !define CT_INC	"PHP Include Script"
!define CT_TXT	"Prostý text"
!define CT_PY	"Skript Python"
!define CT_RB	"Skript Ruby"
!define CT_SMARTY	"Skript Smarty"
!define CT_VBS	"Skript VisualBasic"
!define CT_XHTML	"Soubor XHTML"
!define CT_XML	"Soubor XML"
!define CT_XSL	"Stylopis XML"
