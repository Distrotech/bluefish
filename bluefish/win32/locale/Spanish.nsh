;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Spanish Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Daniel Mustieles <daniel.mustieles@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Complementos"
!define SECT_SHORTCUT "Acceso directo del escritorio"
!define SECT_DICT "Comprobar la sintaxis (se requiere conexión a Internet para descargar)"

; License Page
!define LICENSEPAGE_BUTTON "Siguiente"
!define LICENSEPAGE_FOOTER "${PRODUCT} está publicado bajo la Licencia Pública General de GNU. Esta licencia se proporciona sólo para propósitos informativos. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Se ha encontrado una copia local de..."
!define DOWN_CHKSUM "Suma de comprobación verificada..."
!define DOWN_CHKSUM_ERROR "La suma de comprobación no coincide..."

; Aspell Strings
!define DICT_INSTALLED "Latest version of this dictionary is installed, skipping download of:"
!define DICT_DOWNLOAD "Descargando diccionardo de comprobación ortográfica..."
!define DICT_FAILED "Falló al extraer el diccionario:"
!define DICT_EXTRACT "Extrayendo diccionario..."

; GTK+ Strings
!define GTK_DOWNLOAD "Descargando GTK+..."
!define GTK_FAILED "Falló al descargar GTK+:"
!define GTK_INSTALL "Instalando GTK+..."
!define GTK_UNINSTALL "Desinstalando GTK+..."
!define GTK_REQUIRED "Instale GTK+ ${GTK_MIN_VERSION} o superior y asegúrese de que está en su ruta antes de ejecutar Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa de caracteres"
!define PLUG_ENTITIES "Entidades"
!define PLUG_HTMLBAR "Barra HTML"
!define PLUG_INFBROWSER "Navegador de información"
!define PLUG_SNIPPETS "Fragmentos"
!define PLUG_ZENCODING "Codificación Zen"

; File Associations Page
!define FA_TITLE "Asociaciones de archivos"
!define FA_HEADER "Seleccione los tipos de archivos para los que quiere que ${PRODUCT} sea el editor predeterminado."
!define FA_SELECT "Seleccionar todo"
!define FA_UNSELECT "Deseleccionar todo"

; Misc
!define FINISHPAGE_LINK "Visite la página web de the Bluefish"
!define UNINSTALL_SHORTCUT "Deinstalar ${PRODUCT}"
!define FILETYPE_REGISTER "Registrando tipo de archivo:"
!define UNSTABLE_UPGRADE "Hay instalada una versión inestable de ${PRODUCT}.$\n¿Eliminar las versiones anteriores antes de continuar (recomendado)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Descargando %s"
!define INETC_CONN "Conectando ..."
!define INETC_TSEC "segundo"
!define INETC_TMIN "minuto"
!define INETC_THOUR "hora"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) de %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s restante)"

; Content Types
!define CT_ADA	"Archivo de código en Ada"
!define CT_ASP 	"Script en ActiveServer"
!define CT_SH		"Script en Bash"
!define CT_BFPROJECT	"Proyecto Bluefish"
!define CT_BFLANG2	"Bluefish Language Definition File Version 2"
!define CT_C		"Archivo de código en C"
!define CT_H		"Archivo de cabecera de C"
!define CT_CPP	"Archivo de código en C++"
!define CT_HPP	"Archivo de cabecera de C++"
!define CT_CSS	 "Hoja de estilos en cascada"
!define CT_D		"Archivo de código en D"
!define CT_DIFF 	"Archivo Diff/Patch"
!define CT_PO		"Traducción Gettext"
!define CT_JAVA	"Archivo de código en Java"	
!define CT_JS		"Script en JavaScript"
!define CT_JSP	"Script en JavaServer Pages"
!define CT_MW		"Archivo de MediaWiki"
!define CT_NSI	"Script en NSIS"
!define CT_NSH	"Archivo de cabecera de NSIS"
!define CT_PL		"Script en Perl"
!define CT_PHP	"Script en PHP"
!define CT_INC	"Script en PHP Include"
!define CT_TXT	"Texto plano"
!define CT_PY		"Script en Python"
!define CT_RB		"Script en Ruby"
!define CT_SMARTY	"Script en Smarty"
!define CT_VBS	"Script en VisualBasic"
!define CT_XHTML	"Archivo XHTML"
!define CT_XML	"Archivo XML"
!define CT_XSL	"Hoja de estilos de XML"
