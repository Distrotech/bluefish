;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  English Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Roberto Bellingeri <bellingeri@netguru.it>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish Editor"
!define SECT_PLUGINS "Plugin"
!define SECT_SHORTCUT "Collegamento sul desktop"
!define SECT_DICT "Correttore ortografico (connessione ad Internet richiesta per il download)"

; License Page
!define LICENSEPAGE_BUTTON "Avanti"
!define LICENSEPAGE_FOOTER "${PRODUCT} è distribuito sotto la GNU General Public License. La licenza viene fornita a scopo puramente informativo. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Trovata copia locale di %s..."
!define DOWN_CHKSUM "Codice di controllo verificato..."
!define DOWN_CHKSUM_ERROR "Codice di controllo errato..."

; Aspell Strings
!define DICT_INSTALLED "L'ultima versione di questo dizionario è installata, salto il download di:"
!define DICT_DOWNLOAD "Download dizionario ortografico..."
!define DICT_FAILED "Impossibile scaricare il dizionario:"
!define DICT_EXTRACT "Estrazione dizionario..."

; GTK+ Strings
!define GTK_DOWNLOAD "Download di GTK+..."
!define GTK_FAILED "Impossibile scaricare GTK+:"
!define GTK_INSTALL "Instalazione di GTK+..."
!define GTK_UNINSTALL "Disinstallazione di GTK+..."
!define GTK_REQUIRED "Si prega di installare GTK+ ${GTK_MIN_VERSION} o superiore e assicurarsi che sia nel PATH prima di eseguire Bluefish."


; Plugin Names
!define PLUG_CHARMAP "Mappa Caratteri"
!define PLUG_ENTITIES "Entità"
!define PLUG_HTMLBAR "Barra HTML"
!define PLUG_INFBROWSER "Info Browser"
!define PLUG_SNIPPETS "Ritagli"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Associazione file"
!define FA_HEADER "Selezionare i tipi di file che si desidera associare a ${PRODUCT}."
!define FA_SELECT "Seleziona tutti"
!define FA_UNSELECT "Deseleziona tutti"

; Misc
!define FINISHPAGE_LINK "Visita il sito Bluefish"
!define UNINSTALL_SHORTCUT "Disinstalla ${PRODUCT}"
!define FILETYPE_REGISTER "Associa Tipi di File:"
!define UNSTABLE_UPGRADE "Una versione instabile di ${PRODUCT} risulta installata.$\nRimuovere le versioni precedenti prima di proseguire (Raccomandato)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Download %s"
!define INETC_CONN "Connessione..."
!define INETC_TSEC "secondi"
!define INETC_TMIN "minuti"
!define INETC_THOUR "ore"
!define INETC_TPLUR " "
!define INETC_PROGRESS "%dkB (%d%%) di %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s restanti)"

; Content Types
!define CT_ADA	"File sorgente Ada"
!define CT_ASP	"Script ActiveServer Page"
!define CT_SH	"Script per Shell Bash"
!define CT_BFPROJECT	"Progetto Bluefish"
!define CT_BFLANG2	"File di definizione linguaggio Bluefish versione 2"
!define CT_C	"File sorgente C"
!define CT_H	"File Header C"
!define CT_CPP	"File sorgente C++"
!define CT_HPP	"File Header C++"
!define CT_CSS	"Fogli di stile"
!define CT_D	"File sorgente D"
!define CT_DIFF "File Diff/Patch"
!define CT_PO	"Traduzione Gettext"
!define CT_JAVA	"File sorgente Java"
!define CT_JS	"Script JavaScript"
!define CT_JSP	"Script JavaServer Page"
!define CT_MW	"File MediaWiki"
!define CT_NSI	"Script NSIS"
!define CT_NSH	"File Header NSIS"
!define CT_PL	"Script Perl"
!define CT_PHP	"Script PHP"
!define CT_INC	"Script Inclusi"
!define CT_TXT	"File di testo"
!define CT_PY	"Script Python"
!define CT_RB	"Script Ruby"
!define CT_SMARTY	"Script Smarty"
!define CT_VBS	"Script VisualBasic"
!define CT_XHTML	"File XHTML"
!define CT_XML	"File XML"
!define CT_XSL	"Fogli di stile XML"
