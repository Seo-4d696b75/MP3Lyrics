#ifndef CHAR_UTF8_UTF16
#define CHAR_UTF8_UTF16

#include <windows.h>



/**
 * 文字コードをUTF-16よりUTF-8へと変換。
 *
 * @param[out] dest 出力文字列UTF-8
 * @param[in]  dest_size destのバイト数
 * @param[in]  src 入力文字列UTF-16
 * @param[in]  src_size 入力文字列の文字数
 *
 * @return 成功時には出力文字列のバイト数を戻します。
 *         dest_size に0を指定し、こちらの関数を呼び出すと、変換された
 *         文字列を格納するのに必要なdestのバイト数を戻します。
 *         関数が失敗した場合には、FALSEを戻します。
 */
int Utf16ToUtf8(char* dest, size_t dest_size, wchar_t* src, size_t src_size);



/**
 * 文字コードをUTF-8よりUTF16へと変換。
 *
 * @param[out] dest 出力文字列UTF-16
 * @param[in]  dest_size destのサイズをワード単位で指定
 * @param[in]  src 入力文字列UTF-8
 * @param[in]  src_size 入力文字列のバイト数
 *
 * @return 成功時には出力文字列の文字数を戻します。
 *         dest_size に0を指定し、こちらの関数を呼び出すと、変換された
 *         文字列を格納するのに必要なdestのサイズの文字数を戻します。
 *         関数が失敗した場合には、FALSEを戻します。
 */
int Utf8ToUtf16(wchar_t* dest, size_t dest_size, char* src, size_t src_size);

#endif