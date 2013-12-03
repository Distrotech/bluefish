;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Ukrainian Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Yuri Chornoivan <yurchor@ukr.net>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Редактор Bluefish"
!define SECT_DEPENDS "Залежності"
!define SECT_PLUGINS "Додатки"
!define SECT_SHORTCUT "Піктограма на стільниці"
!define SECT_DICT "Файли перевірки правопису (для звантаження потрібне з’єднання з мережею інтернет)"

; License Page
!define LICENSEPAGE_BUTTON "Далі"
!define LICENSEPAGE_FOOTER "${PRODUCT} випущено за умов дотримання GNU General Public License. Текст ліцензії наведено тут лише з інформаційною метою. $_CLICK"

; General Download Messages
!define DOWN_LOCAL "Виявлено локальну копію %s..."
!define DOWN_CHKSUM "Перевірено контрольну суму..."
!define DOWN_CHKSUM_ERROR "Контрольні суми не збігаються..."

; Aspell Strings
!define DICT_INSTALLED "Вже встановлено найсвіжішу версію цього словника, пропускаємо звантаження:"
!define DICT_DOWNLOAD "Звантаження словника перевірки правопису..."
!define DICT_FAILED "Помилка під час звантаження словника:"
!define DICT_EXTRACT "Видобування словника..."

; GTK+ Strings
!define GTK_DOWNLOAD "Звантаження GTK+..."
!define GTK_FAILED "Помилка під час звантаження GTK+:"
!define GTK_INSTALL "Встановлення GTK+..."
!define GTK_UNINSTALL "Вилучення GTK+..."
!define GTK_REQUIRED "Будь ласка, встановіть GTK+ ${GTK_MIN_VERSION} або новішу версію і переконайтеся, що відповідні каталоги вказано у змінній PATH, перш ніж запускати Bluefish."

; Python Strings
!define PYTHON_DOWNLOAD "Отримуємо пакунок Python..."
!define PYTHON_FAILED "Спроба отримання Python зазнала невдачі:"
!define PYTHON_INSTALL "Встановлюємо Python..."
!define PYTHON_REQUIRED "Будь ласка, встановіть Python ${PYTHON_MIN_VERSION} або новішу версію, перш ніж запускати Bluefish.$\nPython потрібен для роботи додатка Zencoding та інших можливостей."

; Plugin Names
!define PLUG_CHARMAP "Таблиця символів"
!define PLUG_ENTITIES "Об’єкти"
!define PLUG_HTMLBAR "Панель HTML"
!define PLUG_INFBROWSER "Панель перегляду даних"
!define PLUG_SNIPPETS "Фрагменти"
!define PLUG_VCS "Керування версіями"
!define PLUG_ZENCODING "Дзен-програмування"

; File Associations Page
!define FA_TITLE "Прив’язка файлів"
!define FA_HEADER "Вкажіть типи файлів, для редагування яких типово слід використовувати ${PRODUCT}."
!define FA_SELECT "Позначити всі"
!define FA_UNSELECT "Зняти позначення з усіх"

; Misc
!define FINISHPAGE_LINK "Відвідати домашню сторінку Bluefish"
!define UNINSTALL_SHORTCUT "Вилучити ${PRODUCT}"
!define FILETYPE_REGISTER "Реєстрування типу файлів:"
!define UNSTABLE_UPGRADE "Встановлено нестабільну версію ${PRODUCT}.$\nЧи слід вилучити попередні версії, перш ніж продовжувати (рекомендуємо це зробити)?"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "Отримання %s"
!define INETC_CONN "Встановлення з’єднання..."
!define INETC_TSEC "сек."
!define INETC_TMIN "хв."
!define INETC_THOUR "год."
!define INETC_TPLUR " "
!define INETC_PROGRESS "%dкБ (%d%%) з %dкБ @ %d.%01dкБ/с"
!define INETC_REMAIN " (лишилося %d %s%s)"

; Content Types
!define CT_ADA	"Файл кодів Ada"
!define CT_ASP "Скрипт сторінки ActiveServer"
!define CT_SH	"Скрипт оболонки Bash"
!define CT_BFPROJECT	"Проект Bluefish"
!define CT_BFLANG2	"Файл визначення мов Bluefish версії 2"
!define CT_C	"Файл кодів мовою C"
!define CT_H	"Файл заголовків мовою C"
!define CT_CPP	"Файл кодів мовою C++"
!define CT_HPP	"Файл заголовків мовою C++"
!define CT_CSS "Таблиця стилів"
!define CT_D	"Файл кодів мовою D"
!define CT_DIFF "Файл diff/patch"
!define CT_PO	"Файл перекладу Gettext"
!define CT_JAVA	"Файл кодів мовою Java"	
!define CT_JS	"Скрипт JavaScript"
!define CT_JSP	"Скрипт сторінок JavaServer"
!define CT_MW	"Файл MediaWiki"
!define CT_NSI	"Скрипт NSIS"
!define CT_NSH	"Файл заголовків NSIS"
!define CT_PL	"Скрипт Perl"
!define CT_PHP	"Скрипт PHP"
!define CT_INC	"Скрипт включення PHP"
!define CT_TXT	"Звичайний текст"
!define CT_PY	"Скрипт Python"
!define CT_RB	"Скрипт Ruby"
!define CT_SMARTY	"Скрипт Smarty"
!define CT_VBS	"Скрипт VisualBasic"
!define CT_XHTML	"Скрипт XHTML"
!define CT_XML	"Файл XML"
!define CT_XSL	"Таблиця стилів XML"
