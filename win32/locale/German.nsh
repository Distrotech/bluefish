;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  German Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Daniel Leidert <daniel.leidert@wgdd.de>, 2009, 2012
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Desktop-Verkn�pfung"
!define SECT_DICT "Unterst�tzung f�r Rechtschreibkontrolle (f�r den Download wird eine Internetverbindung ben�tigt)"

; License Page
!define LICENSEPAGE_BUTTON "Weiter"
!define LICENSEPAGE_FOOTER "${PRODUCT} wird unter der GNU General Public License (GPL) ver�ffentlicht. Die Lizenz dient der Information. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Lokale Kopie von %s gefunden..."
!define DOWN_CHKSUM "Pr�fsumme verifiziert..."
!define DOWN_CHKSUM_ERROR "Pr�fsummenfehler..."

; Aspell Strings
!define DICT_INSTALLED "Die aktuellste Version des W�rterbuches ist bereits installiert. Der Download wird �bersprungen:"
!define DICT_DOWNLOAD "Herunterladen des W�rterbuches..."
!define DICT_FAILED "Das Herunterladen des W�rterbuches ist fehlgeschlagen:"
!define DICT_EXTRACT "Entpacken des W�rterbuches..."

; GTK+ Strings
!define GTK_DOWNLOAD "Herunterladen von GTK+..."
!define GTK_FAILED "Das Herunterladen von GTK+ ist fehlgeschlagen:"
!define GTK_INSTALL "Installieren von GTK+..."
!define GTK_UNINSTALL "Deinstallieren von GTK+..."
!define GTK_REQUIRED "Bitte installieren Sie GTK+ ${GTK_MIN_VERSION} oder neuer und stellen Sie sicher, dass der Installationspfad in PATH enthalten ist, bevor Sie Bluefish starten."

; Plugin Names
!define PLUG_CHARMAP "Zeichentabelle"
!define PLUG_ENTITIES "Entit�ten"
!define PLUG_HTMLBAR "HTML-Werkzeugleiste"
!define PLUG_INFBROWSER "Info-Browser"
!define PLUG_SNIPPETS "Schnipsel"
!define PLUG_ZENCODING "Zen-Coding"

; File Associations Page
!define FA_TITLE "Dateitypen zuordnen"
!define FA_HEADER "W�hlen Sie die Dateitypen aus, die bevorzugt mit ${PRODUCT} ge�ffnet werden sollen."
!define FA_SELECT "Alles ausw�hlen"
!define FA_UNSELECT "Nichts ausw�hlen"

; Misc
!define FINISHPAGE_LINK "Besuchen Sie die Bluefish Webseite"
!define UNINSTALL_SHORTCUT "${PRODUCT} deinstallieren"
!define FILETYPE_REGISTER "Registriere Dateityp:"
!define UNSTABLE_UPGRADE "Eine Entwicklerversion von ${PRODUCT} ist installiert.$\nSollen alle �lteren Versionen entfernt werden, bevor mit der Installation fortgefahren wird? (Empfohlen)"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Herunterladen %s"
!define INETC_CONN "Verbinden ..."
!define INETC_TSEC "Sekunde"
!define INETC_TMIN "Minute"
!define INETC_THOUR "Stunde"
!define INETC_TPLUR "n"
!define INETC_PROGRESS "%dkB (%d%%) von %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s verbleibend)"

; Content Types
!define CT_ADA	"Ada Quelldatei"
!define CT_ASP	"ActiveServer Page Skript"
!define CT_SH	"Bash Shellskript"
!define CT_BFPROJECT	"Bluefish Projektdatei"
!define CT_BFLANG2	"Bluefish Sprach- und Syntaxunterst�tzung (Version 2)"
!define CT_C	"C Quelldatei"
!define CT_H	"C Header"
!define CT_CPP	"C++ Quelldatei"
!define CT_HPP	"C++ Header"
!define CT_CSS	"Cascading Style Sheet"
!define CT_D	"D Quelldatei"
!define CT_DIFF	"Diff/Patch Datei"
!define CT_PO	"Gettext �bersetzung"
!define CT_JAVA	"Java Quelldatei"	
!define CT_JS	"JavaScript Skript"
!define CT_JSP	"JavaServer Pages Skript"
!define CT_MW	"MediaWiki Datei"
!define CT_NSI	"NSIS Skript"
!define CT_NSH	"NSIS Header"
!define CT_PL	"Perl Skript"
!define CT_PHP	"PHP Skript"
!define CT_INC	"PHP Include Skript"
!define CT_TXT	"Textdatei"
!define CT_PY	"Python Skript"
!define CT_RB	"Ruby Skript"
!define CT_SMARTY	"Smarty Skript"
!define CT_VBS	"VisualBasic Skript"
!define CT_XHTML	"XHTML"
!define CT_XML	"XML Datei"
!define CT_XSL	"XSL Stylesheet"
