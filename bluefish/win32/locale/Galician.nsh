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
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Conectores"
!define SECT_SHORTCUT "Atallo de escritorio"
!define SECT_DICT "Comprobación da escritura (Necesaria conexión a internet para descarga)"

; License Page
!define LICENSEPAGE_BUTTON "Seguinte"
!define LICENSEPAGE_FOOTER "${PRODUCT} é ofrecido baixo Licenza Pública Xeral GNU. A licenza amósase aquí só a xeito informativo. $_CLICK"

; Aspell Strings
!define DICT_INSTALLED "A última versión deste diccionario xa está instalada, evitando a descarga de:"
!define DICT_DOWNLOAD "Descargando o diccionario de comprobación..."
!define DICT_FAILED "Erro na descarga do diccionario:"
!define DICT_EXTRACT "Extraendo o diccionario..."

; GTK+ Strings
!define GTK_DOWNLOAD "Baixando GTK+..."
!define GTK_FAILED "Erro na descarga de GTK+ :"
!define GTK_INSTALL "Instalando GTK+..."
!define GTK_PATH "Instalando GTK+ no sistema de ficheiros."

; Plugin Names
!define PLUG_CHARMAP "Mapa de Caracteres"
!define PLUG_ENTITIES "Entidades"
!define PLUG_HTMLBAR "Barra HTML"
!define PLUG_INFBROWSER "Info do navegador"
!define PLUG_SNIPPETS "Anacos de código"

; File Associations Page
!define FA_TITLE "Asociación de ficheiros"
!define FA_HEADER "Escolle o tipo de aquivos dos que che gustaría que ${PRODUCT} fose o editor por defecto."
!define FA_SELECT "Escolleos todos"
!define FA_UNSELECT "Deseleccionaos todos"

; Misc
!define FINISHPAGE_LINK "Visita a web de Bluefish"
!define UNINSTALL_SHORTCUT "Desinstala ${PRODUCT}"

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
!define CT_PO	"Tradución Gettext"
!define CT_JAVA	"Ficheiro fonte Java"	
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
