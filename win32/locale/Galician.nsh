;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  English Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Xose M (Pika) <pikamoku@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Conectores"
!define SECT_SHORTCUT "Atallo de escritorio"
!define SECT_DICT "Comprobación da escritura (Necesaria conexión a internet para descarga)"

; License Page
!define LICENSEPAGE_BUTTON "Seguinte"
!define LICENSEPAGE_FOOTER "${PRODUCT} é ofrecido baixo Licenza Pública Xeral GNU. A licenza amósase aquí só a xeito informativo. $_CLICK"

; General Download Messages
; !define DOWN_LOCAL "Local copy of %s found..."
; !define DOWN_CHKSUM "Checksum verified..."
; !define DOWN_CHKSUM_ERROR "Checksum mismatch..."

; Aspell Strings
!define DICT_INSTALLED "A última versión deste diccionario xa está instalada, evitando a descarga de:"
!define DICT_DOWNLOAD "Descargando o diccionario de comprobación..."
!define DICT_FAILED "Erro na descarga do diccionario:"
!define DICT_EXTRACT "Extraendo o diccionario..."

; GTK+ Strings
!define GTK_DOWNLOAD "Baixando GTK+..."
!define GTK_FAILED "Erro na descarga de GTK+ :"
!define GTK_INSTALL "Instalando GTK+..."
; !define GTK_UNINSTALL "Uninstalling GTK+..."
; !define GTK_REQUIRED "Please install GTK+ ${GTK_MIN_VERSION} or higher and make sure it is in your PATH before running Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa de Caracteres"
!define PLUG_ENTITIES "Entidades"
!define PLUG_HTMLBAR "Barra HTML"
!define PLUG_INFBROWSER "Info do navegador"
!define PLUG_SNIPPETS "Anacos de código"
; !define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Asociación de ficheiros"
!define FA_HEADER "Escolle o tipo de aquivos dos que che gustaría que ${PRODUCT} fose o editor por defecto."
!define FA_SELECT "Escolleos todos"
!define FA_UNSELECT "Deseleccionaos todos"

; Misc
!define FINISHPAGE_LINK "Visita a web de Bluefish"
!define UNINSTALL_SHORTCUT "Desinstala ${PRODUCT}"
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
!define CT_ADA	"Ficheiro fonte Ada"
!define CT_ASP "ActiveServer Page Script"
!define CT_SH	"Bash Shell Script"
!define CT_BFPROJECT	"Proxecto Bluefish"
!define CT_BFLANG2	"Ficheiro de definición de lingua de Bluefish Version 2"
!define CT_C	"Ficheiro fonte C"
!define CT_H	"Ficheiro cabeceira C"
!define CT_CPP	"Ficheiro fonte C++"
!define CT_HPP	"Ficheiro cabeceira C++"
!define CT_CSS "Folla de estilo CSS"
!define CT_D	"Ficheiro fonte D"
; !define CT_DIFF "Diff/Patch File"
!define CT_PO	"Tradución Gettext"
!define CT_JAVA	"Ficheiro fonte Java"	
!define CT_JS	"JavaScript Script"
!define CT_JSP	"JavaServer Pages Script"
; !define CT_MW	"MediaWiki File"
!define CT_NSI	"NSIS Script"
!define CT_NSH	"NSIS Header File"
!define CT_PL	"Perl Script"
!define CT_PHP	"PHP Script"
; !define CT_INC	"PHP Include Script"
!define CT_TXT	"Plain Text"
!define CT_PY	"Python Script"
!define CT_RB	"Ruby Script"
!define CT_SMARTY	"Smarty Script"
!define CT_VBS	"VisualBasic Script"
!define CT_XHTML	"XHTML File"
!define CT_XML	"XML File"
!define CT_XSL	"XML Stylesheet"
