;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Norwegian Nynorsk Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Yngve Spjeld Landro <l10n@landro.net>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish skriveprogram"
!define SECT_PLUGINS "Programtillegg"
!define SECT_SHORTCUT "Snarveg på skrivebordet"
!define SECT_DICT "Stavekontroll (krev internettilkopling for å lasta ned ordlister)"

; License Page
!define LICENSEPAGE_BUTTON "Neste"
!define LICENSEPAGE_FOOTER "${PRODUCT} er lisensiert etter GNU General Public-lisensen. Lisensen er tilgjengelig her for gjennomsyn. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Fann lokal kopi av %s …"
!define DOWN_CHKSUM "Sjekksummen er kontrollert …"
!define DOWN_CHKSUM_ERROR "Sjekksumfeil …"

; Aspell Strings
!define DICT_INSTALLED "Siste utgåva av denne ordlista er allereie installert. Lastar ikkje ned:"
!define DICT_DOWNLOAD "Lastar ned stavekontrollordlista …"
!define DICT_FAILED "Klarte ikkje å lasta ned ordlista:"
!define DICT_EXTRACT "Pakkar ut ordlista …"

; GTK+ Strings
!define GTK_DOWNLOAD "Lastar ned GTK+ …"
!define GTK_FAILED "GTK+-nedlastinga feila:"
!define GTK_INSTALL "Installerer GTK+ …"
!define GTK_UNINSTALL "Avinstallerer GTK+ …"
!define GTK_REQUIRED "Installer GTK+ 2.14.7 eller nyare og sjå til at det er i systemfilbana (PATH) før du køyrer Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Teiknkart"
!define PLUG_ENTITIES "Entitetar"
!define PLUG_HTMLBAR "HTML-verktøylinje"
!define PLUG_INFBROWSER "Info-browser"
!define PLUG_SNIPPETS "Utklipp"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Filtilknytingar"
!define FA_HEADER "Vel filtypane du vil at ${PRODUCT} skal handtera."
!define FA_SELECT "Vel alle"
!define FA_UNSELECT "Fjern alle"

; Misc
!define FINISHPAGE_LINK "Besøk Bluefish si heimeside"
!define UNINSTALL_SHORTCUT "Avinstaller ${PRODUCT}"
!define FILETYPE_REGISTER "Registrerer filtypen:"
!define UNSTABLE_UPGRADE "Ei ustabil utgåve av ${PRODUCT} er allereie installert.$\nVil du fjerna tidlegare utgåver før vi held fram (tilrådd)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Lastar ned %s"
!define INETC_CONN "Koplar til …"
!define INETC_TSEC "sekund"
!define INETC_TMIN "minutt"
!define INETC_THOUR "time"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) av %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s står att)"

; Content Types
!define CT_ADA	"Ada-kjeldefil"
!define CT_ASP "ActiveServer Page-skript"
!define CT_SH	"Bash-skalskript"
!define CT_BFPROJECT	"Bluefish-prosjekt"
!define CT_BFLANG2	"Bluefish språkdefinisjonsfil utgåve 2"
!define CT_C	"C-kjeldefil"
!define CT_H	"C-hovud-fil"
!define CT_CPP	"C++ kjeldefil"
!define CT_HPP	"C++-hovud-fil"
!define CT_CSS "Stilark (CSS)"
!define CT_D	"D-kjeldefil"
!define CT_DIFF "Diff-/Patch-fil"
!define CT_PO	"Gettext-omsetjing"
!define CT_JAVA	"Java-kjeldefil"	
!define CT_JS	"JavaScript-skript"
!define CT_JSP	"JavaServer Pages-skript"
!define CT_MW	"MediaWiki-fil"
!define CT_NSI	"NSIS-skript"
!define CT_NSH	"NSIS-hovud-fil"
!define CT_PL	"Perl-skript"
!define CT_PHP	"PHP-skript"
!define CT_INC	"PHP Include-skript"
!define CT_TXT	"Rein tekst"
!define CT_PY	"Python-skript"
!define CT_RB	"Ruby-skript"
!define CT_SMARTY	"Smarty-skript"
!define CT_VBS	"VisualBasic-skript"
!define CT_XHTML	"XHTML-fil"
!define CT_XML	"XML-fil"
!define CT_XSL	"XML-stilsett"
