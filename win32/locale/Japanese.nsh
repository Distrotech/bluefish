;----------------------------------------------
; Bluefish Windows NSIS Install Script
;  English Language Header
; 
;  The Bluefish Developers
;
;  Translators:
;   Takeshi Hamasaki <hmatrjp@users.sourceforge.jp>
;----------------------------------------------

; Section Names
!define SECT_BLUEFISH "Bluefish エディタ"
!define SECT_PLUGINS "プラグイン"
!define SECT_SHORTCUT "デスクトップ ショートカット"
!define SECT_DICT "スペルチェック 言語 (ダウンロードするため、インターネット接続が必要です)"

; License Page
!define LICENSEPAGE_BUTTON "次へ"
!define LICENSEPAGE_FOOTER "${PRODUCT} は GNU General Public License でリリースされています。参考のため、このライセンスをここに表示します。$_CLICK"

; General Download Messages
!define DOWN_LOCAL "ローカルPCで %s が見つかりました..."
!define DOWN_CHKSUM "チェックサムを確認しました..."
!define DOWN_CHKSUM_ERROR "チェックサムが正しくありません..."

; Aspell Strings
!define DICT_INSTALLED "最新版の辞書がインストールされています。ダウンロードをスキップします:"
!define DICT_DOWNLOAD "スペルチェック用の辞書をダウンロードしています..."
!define DICT_FAILED "辞書のダウンロードに失敗しました:"
!define DICT_EXTRACT "辞書を展開しています..."

; GTK+ Strings
!define GTK_DOWNLOAD "GTK+ をダウンロードしています..."
!define GTK_FAILED "GTK+ のダウンロードに失敗しました:"
!define GTK_INSTALL "GTK+ をインストールしています..."
!define GTK_UNINSTALL "GTK+ をアンインストールしています..."
!define GTK_REQUIRED "バージョン ${GTK_MIN_VERSION} 以上の GTK+ をインストールし、Bluefish を起動する前にそこに PATH が通っていることを確認してください。"

; Plugin Names
!define PLUG_CHARMAP "文字マップ"
!define PLUG_ENTITIES "実体参照"
!define PLUG_HTMLBAR "HTML バー"
!define PLUG_INFBROWSER "情報ブラウザ"
!define PLUG_SNIPPETS "スニペット"
!define PLUG_ZENCODING "Zencoding"

; File Associations Page
!define FA_TITLE "拡張子の関連付け"
!define FA_HEADER "${PRODUCT} を既定のエディタとするファイルの拡張子を選んでください。"
!define FA_SELECT "すべて選択"
!define FA_UNSELECT "すべて非選択"

; Misc
!define FINISHPAGE_LINK "Bluefish ホームページを開く"
!define UNINSTALL_SHORTCUT "${PRODUCT} をアンインストール"
!define FILETYPE_REGISTER "ファイルの関連付けを登録しています:"
!define UNSTABLE_UPGRADE "不安定版の ${PRODUCT} がインストールされました。$\n続ける前に以前のバージョンを削除しますか？ (推奨)"

; InetC Plugin Translations
;  /TRANSLATE downloading connecting second minute hour plural progress remaining
!define INETC_DOWN "%s をダウンロードしています"
!define INETC_CONN "接続中 ..."
!define INETC_TSEC "秒"
!define INETC_TMIN "分"
!define INETC_THOUR "時間"
; hour and hours, minute and minutes, second and seconds 
; are not distinguished in Japanese language
!define INETC_TPLUR ""
!define INETC_PROGRESS "%dkB (%d%%) （全部で %dkB）, %d.%01dkB/s"
!define INETC_REMAIN " (%d %s%s 残っています)"

; Content Types
!define CT_ADA	"Ada ソース ファイル"
!define CT_ASP "ActiveServer Page スクリプト"
!define CT_SH	"Bash シェル スクリプト"
!define CT_BFPROJECT	"Bluefish プロジェクト"
!define CT_BFLANG2	"Bluefish 言語定義ファイル バージョン 2"
!define CT_C	"C ソース ファイル"
!define CT_H	"C ヘッダ ファイル"
!define CT_CPP	"C++ ソース ファイル"
!define CT_HPP	"C++ ヘッダ ファイル"
!define CT_CSS "CSS カスケーディング スタイルシート"
!define CT_D	"D ソース ファイル"
!define CT_DIFF "Diff/Patch 差分/パッチ ファイル"
!define CT_PO	"Gettext メッセージカタログ翻訳"
!define CT_JAVA	"Java ソース ファイル"	
!define CT_JS	"Javaスクリプト スクリプト"
!define CT_JSP	"JavaServer Pages スクリプト"
!define CT_MW	"MediaWiki ファイル"
!define CT_NSI	"NSIS スクリプト"
!define CT_NSH	"NSIS ヘッダ ファイル"
!define CT_PL	"Perl スクリプト"
!define CT_PHP	"PHP スクリプト"
!define CT_INC	"PHP インクルード スクリプト"
!define CT_TXT	"テキストファイル"
!define CT_PY	"Python スクリプト"
!define CT_RB	"Ruby スクリプト"
!define CT_SMARTY	"Smarty スクリプト"
!define CT_VBS	"VisualBasic スクリプト"
!define CT_XHTML	"XHTML ファイル"
!define CT_XML	"XML ファイル"
!define CT_XSL	"XML スタイルシート"
