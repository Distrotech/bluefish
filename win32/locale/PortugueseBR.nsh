;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Brazilian Portuguese Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Matheus Cunha Motta <esfingemetal@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Editor Bluefish"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Atalho para Desktop"
!define SECT_DICT "Linguagens do corretor ortográfico (Conexão de Internet necessária para Download)"

; License Page
!define LICENSEPAGE_BUTTON "Próximo"
!define LICENSEPAGE_FOOTER "${PRODUCT} é distribuido sob a licença GNU General Public License. A licença é fornecida aqui apenas para fins informativos. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Copia local de %s encontrada..."
!define DOWN_CHKSUM "Checksum verificado..."
!define DOWN_CHKSUM_ERROR "Checksum incorreto..."

; Aspell Strings
!define DICT_INSTALLED "A versão mais recente desse dicionário está instalado, ignorando download de:"
!define DICT_DOWNLOAD "Fazendo Download do dicionário do corretor ortográfico..."
!define DICT_FAILED "Download do dicionário falhou:"
!define DICT_EXTRACT "Extraindo dicionário..."

; GTK+ Strings
!define GTK_DOWNLOAD "Fazendo Download do GTK+..."
!define GTK_FAILED "Download do GTK+ falhou:"
!define GTK_INSTALL "Instalando GTK+..."
!define GTK_UNINSTALL "Desinstalando GTK+..."
!define GTK_REQUIRED "Por favor instale GTK+ ${GTK_MIN_VERSION} ou superior e certifique-se de tê-lo em seu PATH antes de executar o Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa de caracteres"
!define PLUG_ENTITIES "Entidades"
!define PLUG_HTMLBAR "Barra HTML"
!define PLUG_INFBROWSER "Navegador de Info"
!define PLUG_SNIPPETS "Snippets"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Associações de Arquivo"
!define FA_HEADER "Marque os tipos de arquivo você deseja tornar ${PRODUCT} o editor padrão."
!define FA_SELECT "Marcar Tudo"
!define FA_UNSELECT "Desmarcar Tudo"

; Misc
!define FINISHPAGE_LINK "Visite a Homepage do Bluefish"
!define UNINSTALL_SHORTCUT "Desinstalar ${PRODUCT}"
!define FILETYPE_REGISTER "Registrando Tipo de Arquivo:"
!define UNSTABLE_UPGRADE "Uma versão instável de ${PRODUCT} está instalado.$\nAs versões antigas devem ser removidas antes de continuar (Recomendado)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Fazendo Download %s"
!define INETC_CONN "Conectando ..."
!define INETC_TSEC "segundo"
!define INETC_TMIN "minuto"
!define INETC_THOUR "hora"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) de %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s restante)"

; Content Types
!define CT_ADA	"Código Fonte Ada"
!define CT_ASP "ActiveServer Page Script"
!define CT_SH	"Bash Shell Script"
!define CT_BFPROJECT	"Projeto Bluefish"
!define CT_BFLANG2	"Arquivo de Definição de Linguagem do Bluefish Versão 2"
!define CT_C	"Código Fonte C"
!define CT_H	"Arquivo de Cabeçalho C"
!define CT_CPP	"Código Fonte C++"
!define CT_HPP	"Arquivo de Cabeçalho C++"
!define CT_CSS "Cascading Stylesheet"
!define CT_D	"Código Fonte D"
!define CT_DIFF "Arquivo Diff/Patch"
!define CT_PO	"Tradução Gettext"
!define CT_JAVA	"Código Fonte Java"
!define CT_JS	"Script JavaScript"
!define CT_JSP	"JavaServer Pages Script"
!define CT_MW	"Arquivo MediaWiki"
!define CT_NSI	"Script NSIS"
!define CT_NSH	"Arquivo de Cabeçalho NSIS"
!define CT_PL	"Script Perl"
!define CT_PHP	"Script PHP"
!define CT_INC	"Include Script PHP"
!define CT_TXT	"Texto Puro"
!define CT_PY	"Script Python"
!define CT_RB	"Script Ruby"
!define CT_SMARTY	"Script Smarty"
!define CT_VBS	"Script VisualBasic"
!define CT_XHTML	"Arquivo XHTML"
!define CT_XML	"Arquivo XML"
!define CT_XSL	"Folha de estilos XML"
