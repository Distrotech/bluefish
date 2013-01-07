;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Danish Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   flemming christensen <fc@stromata.dk>, 2012
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Redigeringsprogram"
!define SECT_PLUGINS "Udvidelsesmoduler"
!define SECT_SHORTCUT "Skrivebordsgenvej"
!define SECT_DICT "Sprog til stavekontrol (Internetforbindelse kræves for at downloade)"

; License Page
!define LICENSEPAGE_BUTTON "Fortsæt"
!define LICENSEPAGE_FOOTER "${PRODUCT} udgives under GNU General Public Licensen. Licensen gengives her udelukkende som information. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Lokal kopi af %s findes allerede..."
!define DOWN_CHKSUM "Checksum verificeret..."
!define DOWN_CHKSUM_ERROR "Checksum stemmer ikke..."

; Aspell Strings
!define DICT_INSTALLED "Nyeste udgave af denne ordbog er installeret, henter ikke:"
!define DICT_DOWNLOAD "Henter ordbog til stavekontrol..."
!define DICT_FAILED "Det lykkedes ikke at hente:"
!define DICT_EXTRACT "Udpakker ordbog..."

; GTK+ Strings
!define GTK_DOWNLOAD "Henter GTK+..."
!define GTK_FAILED "Kunne ikke hente GTK+:"
!define GTK_INSTALL "Installerer GTK+..."
!define GTK_UNINSTALL "Afinstallerer GTK+..."
!define GTK_REQUIRED "Installer GTK+ ${GTK_MIN_VERSION} eller nyere og kontroller at den findes i din PATH før du starter Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Tegnoversigt"
!define PLUG_ENTITIES "Entiteter"
!define PLUG_HTMLBAR "HTML-værktøjslinje"
!define PLUG_INFBROWSER "Info-browser"
!define PLUG_SNIPPETS "Tekststumper"
!define PLUG_ZENCODING "Zen-kodning"

; File Associations Page
!define FA_TITLE "Fil-tilknytninger"
!define FA_HEADER "Vælg de filtyper, hvor ${PRODUCT} skal være standard-redigeringsprogram."
!define FA_SELECT "Vælg alle"
!define FA_UNSELECT "Fravælg alle"

; Misc
!define FINISHPAGE_LINK "Besøg hjemmesiden for Bluefish"
!define UNINSTALL_SHORTCUT "Afinstallér ${PRODUCT}"
!define FILETYPE_REGISTER "Registrerer filtype:"
!define UNSTABLE_UPGRADE "Der er installeret en ustabil version af ${PRODUCT}.$\nSkal tidligere versioner fjernes, før du fortsætter (anbefales)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Henter %s"
!define INETC_CONN "Opretter forbindelse ..."
!define INETC_TSEC "sekund"
!define INETC_TMIN "minut"
!define INETC_THOUR "time"
!define INETC_TPLUR "r"
!define INETC_PROGRESS "%dkB (%d%%) af %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s tilbage)"



 Content Types
!define CT_ADA	"Ada kildefil"
!define CT_ASP "ActiveServerPage-skript"
!define CT_SH	"Bash shell-skript"
!define CT_BFPROJECT	"Bluefish-projekt"
!define CT_BFLANG2	"Bluefish sprogdefinitionsfil version 2"
!define CT_C	"C kildefil"
!define CT_H	"C headerfil"
!define CT_CPP	"C++ kildefil"
!define CT_HPP	"C++ kildefil"
!define CT_CSS "CSS-stilark"
!define CT_D	"D kildefil"
!define CT_DIFF "Diff/Patch-fil"
!define CT_PO	"Gettext-oversættelse"
!define CT_JAVA	"Java kildefil"	
!define CT_JS	"JavaScript-skript"
!define CT_JSP	"JavaServerPages-skript"
!define CT_MW	"MediaWiki-fil"
!define CT_NSI	"NSIS-skript"
!define CT_NSH	"NSIS header-fil"
!define CT_PL	"Perl-skript"
!define CT_PHP	"PHP-skript"
!define CT_INC	"PHP-include-skript"
!define CT_TXT	"Simpel tekst"
!define CT_PY	"Python-skript"
!define CT_RB	"Ruby-skript"
!define CT_SMARTY	"Smarty-skript"
!define CT_VBS	"VisualBasic-skript"
!define CT_XHTML	"XHTML-fil"
!define CT_XML	"XML-fil"
!define CT_XSL	"XML-stilark"
