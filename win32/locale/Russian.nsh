;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Russian Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Fr. Br. George <george@altlinux.org>
;----------------------------------------------

; Section Names
define SECT_BLUEFISH "Редактор Bluefish"
define SECT_PLUGINS "Модули"
define SECT_SHORTCUT "Ярлык на рабочем столе"
define SECT_DICT "Язык проверки орфографии (для загрузки необходимо подключение к сети Интернет)"

; License Page
define LICENSEPAGE_BUTTON "Далее"
define LICENSEPAGE_FOOTER "${PRODUCT} распространяется на условиях Общественной Лицензии GNU (GPL). Здесь она показана исключительно в информационных целях. $_CLICK"

; General Download Messages
define DOWN_LOCAL "Найдена локальная копия %s..."
define DOWN_CHKSUM "Контрольная сумма проверена..."
define DOWN_CHKSUM_ERROR "Несовпадение контрольной суммы..."

; Aspell Strings
define DICT_INSTALLED "Последняя версия данного словаря уже установлена, загрузка отменяется:"
define DICT_DOWNLOAD "Загрузка словаря для проверки орфографии..."
define DICT_FAILED "Ошибка загрузки словаря:"
define DICT_EXTRACT "Распаковка словаря..."

; GTK+ Strings
define GTK_DOWNLOAD "Загрузка GTK+..."
define GTK_FAILED "Ошибка загрузки GTK+:"
define GTK_INSTALL "Установка GTK+..."
define GTK_UNINSTALL "Удаление GTK+..."
define GTK_REQUIRED "Пожалуйста, установите GTK+ версии ${GTK_MIN_VERSION} или выше. Перед запуском Bluefish убедитесь также, что путь до GTK+ присутствует в переменной окружения PATH."

; Plugin Names
define PLUG_CHARMAP "Таблица символов"
define PLUG_ENTITIES "Элементы"
define PLUG_HTMLBAR "Панель HTML"
define PLUG_INFBROWSER "Просмотр справки"
define PLUG_SNIPPETS "Сниппеты"
define PLUG_ZENCODING "Zencoding"

; File Associations Page
define FA_TITLE "Привязка файлов"
define FA_HEADER "Выберите типы файлов, для которых ${PRODUCT} будет редактором по умолчанию."
define FA_SELECT "Выбрать всё"
define FA_UNSELECT "Отменить выбор"

; Misc
define FINISHPAGE_LINK "Перейти на домашнюю страницу Bluefish"
define UNINSTALL_SHORTCUT "Удаление ${PRODUCT}"
define FILETYPE_REGISTER "Регистрация типа файла:"
define UNSTABLE_UPGRADE "Установлена нестабильная версия ${PRODUCT}.$\nУдалить предыдущие версии, а затем продолжить работу (рекомендуется)?"

; InetC Plugin Translations
/TRANSLATE downloading connecting second minute hour plural progress remaining
define INETC_DOWN "Загружено %s"
define INETC_CONN "Соединение ..."
define INETC_TSEC "секунд"
define INETC_TMIN "минут"
define INETC_THOUR "час"
define INETC_TPLUR "(а)"
define INETC_PROGRESS "%dкб (%d%%) из %dкб @ %d.%01dкб/сек"
define INETC_REMAIN " (осталось %d %s%s)"

; Content Types
define CT_ADA	"Исходник Ada"
define CT_ASP "Сценарий ActiveServer Page"
define CT_SH	"Сценарий оболочки Bash Shell"
define CT_BFPROJECT	"Проект Bluefish"
define CT_BFLANG2	"Файл определения языков Bluefish, версия 2"
define CT_C	"Исходник Си"
define CT_H	"Заголовочный файл Си"
define CT_CPP	"Исходник Си++"
define CT_HPP	"Заголовочный файл Си++"
define CT_CSS "Таблица стилей CSS"
define CT_D	"Исходник D"
define CT_DIFF "Файл Diff/Patch"
define CT_PO	"Перевод Gettext"
define CT_JAVA	"Исходник Java"	
define CT_JS	"Сценарий JavaScript"
define CT_JSP	"Сценарий JavaServer Pages"
define CT_MW	"Текст MediaWiki"
define CT_NSI	"Сценарий NSIS"
define CT_NSH	"Заголовочный файл NSIS"
define CT_PL	"Сценарий Perl"
define CT_PHP	"Сценарий PHP"
define CT_INC	"Внешний сценарий PHP"
define CT_TXT	"Неразмеченный текст"
define CT_PY	"Сценарий Python"
define CT_RB	"Сценарий Ruby"
define CT_SMARTY	"Сценарий Smarty"
define CT_VBS	"Сценарий VBScript"
define CT_XHTML	"Файл XHTML"
define CT_XML	"Файл XML"
define CT_XSL	"Таблица стилей XML"
