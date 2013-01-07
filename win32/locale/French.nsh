;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  French Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Frédéric Falsetti <falsetti@clansco.org>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Éditeur Bluefish"
!define SECT_PLUGINS "Plugins"
!define SECT_SHORTCUT "Raccourci de bureau"
!define SECT_DICT "Vérification de l'orthographe (connexion Internet requise pour le téléchargement)"

; License Page
!define LICENSEPAGE_BUTTON "Suivant"
!define LICENSEPAGE_FOOTER "${PRODUCT} est diffusé sous licence publique générale GNU. La licence est fournie ici à titre informatif seulement. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Copie locale de %s trouvée… "
!define DOWN_CHKSUM "Checksum vérifié…"
!define DOWN_CHKSUM_ERROR "Checksum erreur…"

; Aspell Strings
!define DICT_INSTALLED "La dernière version de ce dictionnaire est installée, ne télécharge pas :"
!define DICT_DOWNLOAD "Téléchargement du dictionnaire… "
!define DICT_FAILED "Échec du téléchargement du dictionnaire :"
!define DICT_EXTRACT "Extraction du dictionnaire… "

; GTK+ Strings
!define GTK_DOWNLOAD "Téléchargement de GTK+… "
!define GTK_FAILED "Échec du téléchargement de GTK+ :"
!define GTK_INSTALL "Installation de GTK+… "
!define GTK_UNINSTALL "Désinstallation de GTK +… "
!define GTK_REQUIRED "S'il vous plaît installer GTK + 2.14.7 ou supérieur et assurez-vous qu'il est dans votre PATH avant d'exécuter Bluefish."

; Plugin Names
!define PLUG_CHARMAP "Table des caractères"
!define PLUG_ENTITIES "Entités"
!define PLUG_HTMLBAR "Barre HTML"
!define PLUG_INFBROWSER "Navigateur de documentation"
!define PLUG_SNIPPETS "Snippets"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "Associations de fichiers"
!define FA_HEADER "Sélectionner les types de fichier que vous souhaitez associer à ${PRODUCT}."
!define FA_SELECT "Tout sélectionner"
!define FA_UNSELECT "Tout désélectionner"

; Misc
!define FINISHPAGE_LINK "Visiter la page d'accueil Bluefish"
!define UNINSTALL_SHORTCUT "Désinstaller ${PRODUCT}"
!define FILETYPE_REGISTER "Enregistrement du type de fichier:"
!define UNSTABLE_UPGRADE "Une version instable de ${PRODUCT} est installée.$\nPuis-je supprimer cette version avant de continuer (recommandé) ?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Télechargement %s"
!define INETC_CONN "Connection… "
!define INETC_TSEC "seconde"
!define INETC_TMIN "minute"
!define INETC_THOUR "heure"
!define INETC_TPLUR "s"
!define INETC_PROGRESS "%dkB (%d%%) de %dkB @ %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s restant)"

; Content Types
!define CT_ADA	"Fichier source Ada"
!define CT_ASP "Script ActiveServer Page"
!define CT_SH	"Script shell Bash"
!define CT_BFPROJECT	"Projet Bluefish"
!define CT_BFLANG2	"Fichier de définition de Langage Bluefish Version 2"
!define CT_C	"Fichier source C"
!define CT_H	"Fichier d'entête C"
!define CT_CPP	"Fichier source C++"
!define CT_HPP	"Fichier d'entête C++"
!define CT_CSS "Feuille de style en cascade"
!define CT_D	"Fichier source D"
!define CT_DIFF "Fichier Diff/Patch"
!define CT_PO	"Fichier de traduction Gettext"
!define CT_JAVA	"Fichier source Java"	
!define CT_JS	"Script JavaScript"
!define CT_JSP	"Script JavaServer Pages"
!define CT_MW	"Fichier MediaWiki"
!define CT_NSI	"Script NSIS"
!define CT_NSH	"Fichier d'entête NSIS"
!define CT_PL	"Script Perl"
!define CT_PHP	"Script PHP"
!define CT_INC	"PHP Include Script"
!define CT_TXT	"Fichier texte"
!define CT_PY	"Script Python"
!define CT_RB	"Script Ruby"
!define CT_SMARTY	"Script Smarty"
!define CT_VBS	"Script VisualBasic"
!define CT_XHTML	"Fichier XHTML"
!define CT_XML	"Fichier XML"
!define CT_XSL	"Feuille de style XML"
