;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Basque Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Inaki Larranaga Murgoitio <dooteo@euskalgnu.org>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish editorea"
!define SECT_PLUGINS "Pluginak"
!define SECT_SHORTCUT "Mahaigaineko lasterbidea"
!define SECT_DICT "Zuzentzaile ortografikoak (Interneteko konexioa behar da deskargatzeko)"

; License Page
!define LICENSEPAGE_BUTTON "Hurrengoa"
!define LICENSEPAGE_FOOTER "${PRODUCT} GNU Lizentzia Publikoaren pean argitaratu da. Informazio gehiagorako eskaintzen da lizentzia. $_CLICK"

; Aspell Strings
!define DICT_INSTALLED "Hiztegi honen azken bertsioa instalatuta dago, deskarga saltatzen:"
!define DICT_DOWNLOAD "Zuzentzaile ortografikoa deskargatzen..."
!define DICT_FAILED "Huts egin du hitzegia deskargatzean:"
!define DICT_EXTRACT "Hiztegia erauzten..."

; GTK+ Strings
!define GTK_DOWNLOAD "GTK+ deskargatzen..."
!define GTK_FAILED "Huts egin du GTK+ deskargatzean:"
!define GTK_INSTALL "GTK+ instalatzen..."
!define GTK_PATH "GTK+ instalatzen sistemako bide-izenean."

; Plugin Names
!define PLUG_CHARMAP "Karaktere-mapa"
!define PLUG_ENTITIES "Entitateak"
!define PLUG_HTMLBAR "HTML barra"
!define PLUG_INFBROWSER "Arakatzailearen informazioa"
!define PLUG_SNIPPETS "Mozkinak"

; File Associations Page
!define FA_TITLE "Fitxategien esleipenak"
!define FA_HEADER "Hautatu fitxategi motak horien editore lehenetsia ${PRODUCT} izateko."
!define FA_SELECT "Hautatu denak"
!define FA_UNSELECT "Desautatu denak"

; Misc
!define FINISHPAGE_LINK "Bisitatu Bluefish-en webgunea"
!define UNINSTALL_SHORTCUT "Desinstalatu ${PRODUCT}"

; Content Types
!define CT_ADA	"Ada iturburuko fitxategia"
!define CT_ASP "ActiveServer orrialdeen scripta"
!define CT_SH	"Bash scripta"
!define CT_BFPROJECT	"Bluefish proiektua"
!define CT_BFLANG2	"Bluefish hizkuntzaren definizio-fitxategia 2. bertsioa"
!define CT_C	"C iturburuko fitxategia"
!define CT_H	"C  goiburuko fitxategia"
!define CT_CPP	"C++ iturburuko fitxategia"
!define CT_HPP	"C++ goiburuko fitxategia"
!define CT_CSS "Kaskadako estilo-orriak"
!define CT_D	"D iturburuko fitxategia"
!define CT_PO	"Gettext itzulpena"
!define CT_JAVA	"Java iturburuko fitxategia"	
!define CT_JS	"JavaScript scripta"
!define CT_JSP	"JavaServer orrialdeen scripta"
!define CT_NSI	"NSIS scripta"
!define CT_NSH	"NSIS goiburuko fitxategia"
!define CT_PL	"Perl scripta"
!define CT_PHP	"PHP scripta"
!define CT_TXT	"Testu soila"
!define CT_PY	"Python scripta"
!define CT_RB	"Ruby scripta"
!define CT_SMARTY	"Smarty scripta"
!define CT_VBS	"VisualBasic scripta"
!define CT_XHTML	"XHTML fitxategia"
!define CT_XML	"XML fitxategia"
!define CT_XSL	"XML estilo-orria"
