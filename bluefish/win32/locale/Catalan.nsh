;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Catalan Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Mònica Ramírez Arceda <monica@probeta.net>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Connectors"
!define SECT_SHORTCUT "Drecera de l'escriptori"
!define SECT_DICT "Verificació de l'ortografia (es necessita una connexió a Internet per a la descàrrega)"

; License Page
!define LICENSEPAGE_BUTTON "Següent"
!define LICENSEPAGE_FOOTER "El ${PRODUCT} es distribueix sota la llicència GNU General Public License. Es proporciona la llicència només a tall d'informació. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "S'ha trobat una còpia local de %s..."
!define DOWN_CHKSUM "S'ha verificat la suma de verificació..."
!define DOWN_CHKSUM_ERROR "La suma de verificació no concorda..."

; Aspell Strings
!define DICT_INSTALLED "L'última versió d'aquest diccionari està instal·lada, s'omet la baixada de:"
!define DICT_DOWNLOAD "S'està baixanr el diccionari de correcció ortogràfica..."
!define DICT_FAILED "La baixada del diccionari ha fallat:"
!define DICT_EXTRACT "S'està extraient el diccionari..."

; GTK+ Strings
!define GTK_DOWNLOAD "S'està baixant el GTK+..."
!define GTK_FAILED "La baixada del GTK+ ha fallat:"
!define GTK_INSTALL "S'està instal·lant el GTK+..."
!define GTK_UNINSTALL "S'està desintal·lant el GTK+..."
!define GTK_PATH "S'està instal·lant el camí del sistema."
!define GTK_REQUIRED "Instal·leu el GTK+ ${GTK_MIN_VERSION} o superior i assegureu-vos que està en el PATH abans d'executar el Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa de caràcters"
!define PLUG_ENTITIES "Entitats"
!define PLUG_HTMLBAR "Barra de l'HTML"
!define PLUG_INFBROWSER "Navegador de documentació"
!define PLUG_SNIPPETS "Fragments de codi"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Associacions de fitxers"
!define FA_HEADER "Seleccioneu els tipus de fitxers pels quals voleu que el ${PRODUCT} sigui l'editor per defecte."
!define FA_SELECT "Selecciona-ho tot"
!define FA_UNSELECT "Desselecciona-ho tot"

; Misc
!define FINISHPAGE_LINK "Visiteu la pàgina web del Bluefish"
!define UNINSTALL_SHORTCUT "Desinstal·leu el ${PRODUCT}"
!define FILETYPE_REGISTER "S'està registrant el tipus de fitxer:"
!define UNSTABLE_UPGRADE "Hi ha una versió inestable del ${PRODUCT} instal·lada.$\nVoleu suprimir les versions anteriors abans de continuar (recomanat)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "S'està baixant %s"
!define INETC_CONN "S'està connectant..."
!define INETC_TSEC "segon"
!define INETC_TMIN "minut"
!define INETC_THOUR "hora"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) de %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s restant)"

; Content Types
!define CT_ADA	"Codi font Ada"
!define CT_ASP "Script ActiveServer Page"
!define CT_SH	"Script Bash Shell"
!define CT_BFPROJECT	"Projecte Bluefish"
!define CT_BFLANG2	"Fitxer de definició de llenguatge Bluefish Versió 2"
!define CT_C	"Fitxer de codi C"
!define CT_H	"Fitxer de capçalera C"
!define CT_CPP	"Fixer de codi C++"
!define CT_HPP	"Fitxer de capçalera C++"
!define CT_CSS "Full d'estil"
!define CT_D	"Fitxer de codi D"
!define CT_DIFF "Fitxer Diff/Patch"
!define CT_PO	"Traducció Gettext"
!define CT_JAVA	"Fitxer de codi Java"	
!define CT_JS	"Script JavaScript"
!define CT_JSP	"Script JavaServer Pages"
!define CT_MW	"Fitxer de MediaWiki"
!define CT_NSI	"Script NSIS"
!define CT_NSH	"Fitxer de capçalera NSIS"
!define CT_PL	"Script Perl"
!define CT_PHP	"Script PHP"
!define CT_INC	"Script PHP Include"
!define CT_TXT	"Fitxer de text"
!define CT_PY	"Script Python"
!define CT_RB	"Script Ruby"
!define CT_SMARTY	"Script Smarty"
!define CT_VBS	"Script VisualBasic"
!define CT_XHTML	"Fitxer XHTML"
!define CT_XML	"Fitxer XML"
!define CT_XSL	"Full d'estil XML"
