;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Hungarian Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Balázs Úr <urbalazs@gmail.com>, 2012
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish szerkesztő"
!define SECT_PLUGINS "Beépülők"
!define SECT_SHORTCUT "Asztali parancsikon"
!define SECT_DICT "Helyesírás-ellenőrzés nyelvei (Internetkapcsolat szükséges a letöltéshez)"

; License Page
!define LICENSEPAGE_BUTTON "Tovább"
!define LICENSEPAGE_FOOTER "A ${PRODUCT} a GPL General Public Licence alatt került kiadásra. A licenc csak tájékoztató jelleggel van itt. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "A %s helyi másolata megvan…"
!define DOWN_CHKSUM "Ellenőrzőösszeg megfelelő…"
!define DOWN_CHKSUM_ERROR "Ellenőrzőösszeg eltérés…"

; Aspell Strings
!define DICT_INSTALLED "A szótár legújabb verziója telepítve, a letöltése kihagyva ennek:"
!define DICT_DOWNLOAD "Helyesírási szótár letöltése…"
!define DICT_FAILED "A szótár letöltése sikertelen:"
!define DICT_EXTRACT "Szótár kicsomagolása…"

; GTK+ Strings
!define GTK_DOWNLOAD "GTK+ letöltése…"
!define GTK_FAILED "GTK+ letöltése sikertelen:"
!define GTK_INSTALL "GTK+ telepítése…"
!define GTK_UNINSTALL "GTK+ eltávolítása…"
!define GTK_REQUIRED "Kérjük telepítse a GTK+ 2.14.7 vagy újabb verzióját és győződjön meg arról, hogy az a PATH változóhoz hozzá van adva, mielőtt a Bluefish-t futtatná."

; Plugin Names
!define PLUG_CHARMAP "Karakter térkép"
!define PLUG_ENTITIES "Entitások"
!define PLUG_HTMLBAR "HTML eszköztár"
!define PLUG_INFBROWSER "Infó böngésző"
!define PLUG_SNIPPETS "Kódrészletek"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Fájltársítások"
!define FA_HEADER "Válassza ki azokat a fájltípusokat, amelyeket a ${PRODUCT} alapértelmezetten megnyisson!"
!define FA_SELECT "Összes kijelölése"
!define FA_UNSELECT "Összes törlése"

; Misc
!define FINISHPAGE_LINK "Látogassa meg a Bluefish weboldalát!"
!define UNINSTALL_SHORTCUT "${PRODUCT} eltávolítása"
!define FILETYPE_REGISTER "Fájltípus regisztrálása:"
!define UNSTABLE_UPGRADE "A ${PRODUCT} nem stabil kiadása telepítve van.$\nEltávolítja a korábbi verziót a telepítés előtt (ajánlott)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "%s letöltése"
!define INETC_CONN "Csatlakozás…"
!define INETC_TSEC "másodperc"
!define INETC_TMIN "perc"
!define INETC_THOUR "óra"
!define INETC_TPLUR "mp"
!define INETC_PROGRESS "%dkB (%d%%) letöltve ennyiből: %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s van hátra)"

; Content Types
!define CT_ADA	"Ada forrásfájl"
!define CT_ASP "ActiveServer Page parancsfájl"
!define CT_SH	"Bash Shell parancsfájl"
!define CT_BFPROJECT	"Bluefish projekt"
!define CT_BFLANG2	"Bluefish nyelvi definíciós fájl 2. verzió"
!define CT_C	"C forrásfájl"
!define CT_H	"C fejlécfájl"
!define CT_CPP	"C++ forrásfájl"
!define CT_HPP	"C++ fejlécfájl"
!define CT_CSS "Kaszkád stíluslap"
!define CT_D	"D forrásfájl"
!define CT_DIFF "Diff/Patch fájl"
!define CT_PO	"Gettext fordítás"
!define CT_JAVA	"Java forrásfájl"	
!define CT_JS	"JavaScript parancsfájl"
!define CT_JSP	"JavaServer Pages parancsfájl"
!define CT_MW	"MediaWiki fájl"
!define CT_NSI	"NSIS parancsfájl"
!define CT_NSH	"NSIS fejlécfájl"
!define CT_PL	"Perl parancsfájl"
!define CT_PHP	"PHP parancsfájl"
!define CT_INC	"PHP Include parancsfájl"
!define CT_TXT	"Egyszerű szöveg"
!define CT_PY	"Python parancsfájl"
!define CT_RB	"Ruby parancsfájl"
!define CT_SMARTY	"Smarty parancsfájl"
!define CT_VBS	"VisualBasic parancsfájl"
!define CT_XHTML	"XHTML fájl"
!define CT_XML	"XML fájl"
!define CT_XSL	"XML stíluslap"
