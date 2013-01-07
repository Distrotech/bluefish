;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  Simple Chinese Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   ZhiFeng Ma <mzf9527@gmail.com>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish 编辑器"
!define SECT_PLUGINS "插件"
!define SECT_SHORTCUT "桌面快捷方式"
!define SECT_DICT "语法检查语言（需连接到网络进行下载）"

; License Page
!define LICENSEPAGE_BUTTON "下一个"
!define LICENSEPAGE_FOOTER "${PRODUCT} 基于 GNU General Public License 发布。这里提供的许可仅为查阅之用。$_CLICK"

; General Download Messages
!define DOWN_LOCAL "已发现 %s 的本地备份..."
!define DOWN_CHKSUM "已验证校验和..."
!define DOWN_CHKSUM_ERROR "校验和不匹配..."

; Aspell Strings
!define DICT_INSTALLED "字典的最新版本已安装，跳过下载："
!define DICT_DOWNLOAD "语法检查字典下载中..."
!define DICT_FAILED "字典下载失败："
!define DICT_EXTRACT "字典提取中..."

; GTK+ Strings
!define GTK_DOWNLOAD "GTK+ 下载中..."
!define GTK_FAILED "GTK+ 下载失败："
!define GTK_INSTALL "GTK+ 安装中..."
!define GTK_UNINSTALL "GTK+ 卸载中..."
!define GTK_REQUIRED "请安装 GTK+ ${GTK_MIN_VERSION} 或更高的版本，并保证运行 Bluefish 前将其置于你系统的 PATH 中。"

; Plugin Names
!define PLUG_CHARMAP "字符映射"
!define PLUG_ENTITIES "实体"
!define PLUG_HTMLBAR "HTML 栏"
!define PLUG_INFBROWSER "消息浏览器"
!define PLUG_SNIPPETS "代码片段"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "文件关联"
!define FA_HEADER "请选择将 ${PRODUCT} 作为其默认编辑器的文件类型。"
!define FA_SELECT "全选"
!define FA_UNSELECT "取消全选"

; Misc
!define FINISHPAGE_LINK "访问 Bluefish 主页"
!define UNINSTALL_SHORTCUT "卸载 ${PRODUCT}"
!define FILETYPE_REGISTER "注册文件类型："
!define UNSTABLE_UPGRADE "${PRODUCT} 的一个不稳定版本已被安装。$\n在继续前删除前一个版本吗（推荐）？"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "%s 下载中"
!define INETC_CONN "连接中 ..."
!define INETC_TSEC "秒"
!define INETC_TMIN "分"
!define INETC_THOUR "小时"
!define INETC_TPLUR " "
!define INETC_PROGRESS "当前 %dkB (%d%%) 全部 %dkB 速度 %d.%01dkB/秒"
!define INETC_REMAIN " （剩余 %d %s%s）"

; Content Types
!define CT_ADA	"Ada 源程序文件"
!define CT_ASP "ASP 脚本"
!define CT_SH	"Bash Shell 脚本"
!define CT_BFPROJECT	"Bluefish 项目"
!define CT_BFLANG2	"Bluefish 语言定义文件 V2"
!define CT_C	"C 源程序文件"
!define CT_H	"C 头文件"
!define CT_CPP	"C++ 源程序文件"
!define CT_HPP	"C++ 头文件"
!define CT_CSS "CSS"
!define CT_D	"D 源程序文件"
!define CT_DIFF "Diff/Patch 文件"
!define CT_PO	"Gettext 翻译文件"
!define CT_JAVA	"Java 源程序文件"	
!define CT_JS	"JavaScript 脚本"
!define CT_JSP	"JSP 脚本"
!define CT_MW	"MediaWiki 文件"
!define CT_NSI	"NSIS 脚本"
!define CT_NSH	"NSIS 头文件"
!define CT_PL	"Perl 脚本"
!define CT_PHP	"PHP 脚本"
!define CT_INC	"PHP 包含脚本"
!define CT_TXT	"文本文件"
!define CT_PY	"Python 脚本"
!define CT_RB	"Ruby 脚本"
!define CT_SMARTY	"Smarty 脚本"
!define CT_VBS	"VisualBasic 脚本"
!define CT_XHTML	"XHTML 文件"
!define CT_XML	"XML 文件"
!define CT_XSL	"XML 样式表"
