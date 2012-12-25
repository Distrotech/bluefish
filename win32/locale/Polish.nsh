;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Polish Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Quick-Master SI, Paweł Worobiak <dawajpoczte@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Edytor Bluefish"
!define SECT_PLUGINS "Wtyczki"
!define SECT_SHORTCUT "Skrót na pulpicie"
!define SECT_DICT "Sprawdzanie pisowni języków (Wymagane aktywne połączenie z Internetem aby je pobrać)"

; License Page
!define LICENSEPAGE_BUTTON "Dalej"
!define LICENSEPAGE_FOOTER "${PRODUCT} jest wydany na licencji GNU General Public License. Licencja publikowana jest tutaj tylko w celach informacyjnych. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Znaleziono lokalną kopię %s ."
!define DOWN_CHKSUM "Suma kontrolna zweryfikowana ..."
!define DOWN_CHKSUM_ERROR "Suma kontrolna nie zgadza się..."

; Aspell Strings
!define DICT_INSTALLED "Ostatnia wersja tego słownika jest już zainstalowana, pobieranie anulowane:"
!define DICT_DOWNLOAD "Pobierania słownika do sprawdzania pisowni..."
!define DICT_FAILED "Pobieranie słownika nie powiodło się:"
!define DICT_EXTRACT "Rozpakowywanie słownika..."

; GTK+ Strings
!define GTK_DOWNLOAD "Pobieranie GTK+..."
!define GTK_FAILED "Pobieranie GTK+ nie powiodło się:"
!define GTK_INSTALL "Instalowanie GTK+..."
!define GTK_UNINSTALL "Usuwanie GTK+..."
!define GTK_PATH "Instalowanie GTK+ w lokalizacji systemowej."
!define GTK_REQUIRED "Zainstaluję proszę GTK+ w wersji ${GTK_MIN_VERSION} lub wyższej i upewnij się, że znajduje się w odpowiedniej lokalizacji zanim uruchomisz Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Mapa znaków"
!define PLUG_ENTITIES "Podmioty"
!define PLUG_HTMLBAR "Pasek HTML"
!define PLUG_INFBROWSER "Przeglądarka Informacji"
!define PLUG_SNIPPETS "Snippety"
!define PLUG_ZENCODING "Programowanie zencoding"

; File Associations Page
!define FA_TITLE "Powiązania plików"
!define FA_HEADER "Zaznacz te rodzaje plików, dla których ${PRODUCT} ma być domyślnym edytorem."
!define FA_SELECT "Zaznacz wszystkie"
!define FA_UNSELECT "Odznacz wszystkie"

; Misc
!define FINISHPAGE_LINK "Odwiedź stronę domową Bluefish"
!define UNINSTALL_SHORTCUT "Usuń ${PRODUCT}"
!define FILETYPE_REGISTER "Rejestrowanie rodzaju plików:"
!define UNSTABLE_UPGRADE "Wersja rozwojowa ${PRODUCT} jest zainstalowana.$\nCzy mam usunąć poprzednie wersje przed instalacją bieżącej (Zalecane) ?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Pobieranie %s"
!define INETC_CONN "Łączenie ..."
!define INETC_TSEC "sekunda"
!define INETC_TMIN "minuta"
!define INETC_THOUR "godzina"
!define INETC_TPLUR "y"
!define INETC_PROGRESS "%dkB (%d%%) z %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (pozostało %d %s%s)"

; Content Types
!define CT_ADA	"Plik źródłowy Ada"
!define CT_ASP "Strona skryptowe ActiveServer"
!define CT_SH	"Skrypt Bash Shell"
!define CT_BFPROJECT	"Projekt Bluefish"
!define CT_BFLANG2	"Plik definicji języka Bluefish wersja 2"
!define CT_C	"Plik źródłowy C"
!define CT_H	"Plik nagłówkowy C"
!define CT_CPP	"Plik źródłowy C++"
!define CT_HPP	"Plik nagłówkowy C++"
!define CT_CSS "Style kaskadowe CSS"
!define CT_D	"Plik źródłowy D"
!define CT_DIFF "Plik Diff/Łatki"
!define CT_PO	"Tłumaczenie Gettext"
!define CT_JAVA	"Plik źródłowy Java"	
!define CT_JS	"Skrypt JavaScript"
!define CT_JSP	"Strony skryptowe JavaServer"
!define CT_MW	"Plik MediaWiki"
!define CT_NSI	"Skrypt NSIS"
!define CT_NSH	"Plik nagłówkowy NSIS"
!define CT_PL	"Skrypt Perl"
!define CT_PHP	"Skrypt PHP"
!define CT_INC	"Skrypt zawierający PHP"
!define CT_TXT	"Zwykły tekst"
!define CT_PY	"Skrypt Python"
!define CT_RB	"Skrypt Ruby"
!define CT_SMARTY	"Skrypt Smarty"
!define CT_VBS	"Skrypt VisualBasic"
!define CT_XHTML	"Plik XHTML"
!define CT_XML	"Plik XML"
!define CT_XSL	"Arkusz stylów XML"
