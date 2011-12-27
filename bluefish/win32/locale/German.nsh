;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  German Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Daniel Leidert <daniel.leidert@wgdd.de>, 2009
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Desktop-Verknüpfung"
!define SECT_DICT "Unterstützung für Rechtschreibkontrolle (für den Download wird eine Internetverbindung benötigt)"

; License Page
!define LICENSEPAGE_BUTTON "Weiter"
!define LICENSEPAGE_FOOTER "${PRODUCT} wird unter der GNU General Public License (GPL) veröffentlicht. Die Lizenz dient der Information. $_CLICK"

; General Download Messages
; !define DOWN_LOCAL "Local copy of %s found..."
; !define DOWN_CHKSUM "Checksum verified..."
; !define DOWN_CHKSUM_ERROR "Checksum mismatch..."

; Aspell Strings
!define DICT_INSTALLED "Die aktuellste Version des Wörterbuches ist bereits installiert. Der Download wird übersprungen:"
!define DICT_DOWNLOAD "Herunterladen des Wörterbuches..."
!define DICT_FAILED "Das Herunterladen des Wörterbuches ist fehlgeschlagen:"
!define DICT_EXTRACT "Entpacken des Wörterbuches..."

; GTK+ Strings
!define GTK_DOWNLOAD "Herunterladen von GTK+..."
!define GTK_FAILED "Das Herunterladen von GTK+ ist fehlgeschlagen:"
!define GTK_INSTALL "Installieren von GTK+..."
; !define GTK_UNINSTALL "Uninstalling GTK+..."
!define GTK_PATH "GTK+ wird in das Systemverzeichnis installiert."
; !define GTK_REQUIRED "Please install GTK+ ${GTK_MIN_VERSION} or higher and make sure it is in your PATH before running Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Zeichentabelle"
!define PLUG_ENTITIES "Entitäten"
!define PLUG_HTMLBAR "HTML-Werkzeugleiste"
!define PLUG_INFBROWSER "Info-Browser"
!define PLUG_SNIPPETS "Schnipsel"
; !define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Dateitypen zuordnen"
!define FA_HEADER "Wählen Sie die Dateitypen aus, die bevorzugt mit ${PRODUCT} geöffnet werden sollen."
!define FA_SELECT "Alles auswählen"
!define FA_UNSELECT "Nichts auswählen"

; Misc
!define FINISHPAGE_LINK "Besuchen Sie die Bluefish Webseite"
!define UNINSTALL_SHORTCUT "${PRODUCT} deinstallieren"
!define FILETYPE_REGISTER "Registriere Dateityp:"
!define UNSTABLE_UPGRADE "Eine Entwicklerversion von ${PRODUCT} ist installiert.$\nSollen alle älteren Versionen entferent werden, bevor mit der Installation fortgefahren wird? (Empfohlen)"

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
!define CT_ADA	"Ada Quelldatei"
!define CT_ASP	"ActiveServer Page Skript"
!define CT_SH	"Bash Shellskript"
!define CT_BFPROJECT	"Bluefish Projektdatei"
!define CT_BFLANG2	"Bluefish Sprach- und Syntaxunterstützung (Version 2)"
!define CT_C	"C Quelldatei"
!define CT_H	"C Header"
!define CT_CPP	"C++ Quelldatei"
!define CT_HPP	"C++ Header"
!define CT_CSS	"Cascading Style Sheet"
!define CT_D	"D Quelldatei"
!define CT_DIFF	"Diff/Patch Datei"
!define CT_PO	"Gettext Übersetzung"
!define CT_JAVA	"Java Quelldatei"	
!define CT_JS	"JavaScript Skript"
!define CT_JSP	"JavaServer Pages Skript"
!define CT_MW	"MediaWiki Datei"
!define CT_NSI	"NSIS Skript"
!define CT_NSH	"NSIS Header"
!define CT_PL	"Perl Skript"
!define CT_PHP	"PHP Skript"
; !define CT_INC	"PHP Include Script"
!define CT_TXT	"Textdatei"
!define CT_PY	"Python Skript"
!define CT_RB	"Ruby Skript"
!define CT_SMARTY	"Smarty Skript"
!define CT_VBS	"VisualBasic Skript"
!define CT_XHTML	"XHTML"
!define CT_XML	"XML Datei"
!define CT_XSL	"XSL Stylesheet"
