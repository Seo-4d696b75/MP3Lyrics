/**
 *
 * http://eleken.y-lab.org/report/other/mp3tags.shtml
 * https://akabeko.me/blog/memo/mp3/id3v2-frame-list/
 * https://akabeko.me/blog/memo/mp3/id3v2-frame-detail/
 *
 * 窓+MinGW での使用を前提として設計
 *
 * @author Seo-4d696b75
 * @version 2019-01-16
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int version; // supposed to be either 3 or 4
int frames_size;

int read_header(FILE* src);
int parse_syncSafeInt(char* bytes);
int parse_int(char* bytes);
int read_frame(FILE* src);
int read_text(char* des, size_t max_des_size, char* src, size_t src_size, int language, int desc);

// http://yanchde.gozaru.jp/utf16_to_utf8/utf16_to_utf8_1.html
int Utf16ToUtf8(char* dest, size_t dest_size, wchar_t* src, size_t src_size);

int main(int argc, char** argv){
	if ( argc < 2 ){
		fprintf(stderr, "music file not specified.\n");
		return 1;
	}
	FILE *src = fopen(argv[1], "rb");
	if ( src == NULL ){
		fprintf(stderr, "fail to open file : %s.\n", argv[1]);
		return 1;
	}
	if ( !read_header(src) ){
		fprintf(stderr, "fail to read header.\n");
		fclose(src);
	}
	int remain = frames_size;
	while ( remain > 0 ){
		int result = read_frame(src);
		if ( result < 0 ){
			break;
		}
		remain -= result;
	}
	if ( remain < 0 ){
		fprintf(stderr, "frame size mismatched. %d\n", remain);
	}
	fclose(src);
	return 0;
}

int read_header(FILE* src){
	char buffer[4];
	if ( fread(buffer, 1, 3, src) != 3 ) return 0;
	buffer[3] = '\0';
	fprintf(stdout, "HeaderID : %s\n", buffer);
	if ( strcmp(buffer, "ID3") != 0 ){
		fprintf(stderr, "invalid header identifer.");
		return 0;
	}
	if ( fread(buffer, 1, 3, src) != 3 ) return 0;
	fprintf(stdout, "Version : 2.%d.%d\n", buffer[0], buffer[1]);
	if ( buffer[0] != 3 && buffer[0] != 4 ){
		fprintf(stderr, "unsupported version detected.\n");
		return 0;
	}
	version = buffer[0];
	int has_extra_header = (buffer[2] & 0b00100000) > 0;
	fprintf(stdout, "Extra Header : %s\n", has_extra_header ? "yes" : "no");

	if ( fread(buffer, 1, 4, src) != 4 ) return 0;
	int length = parse_syncSafeInt(buffer);
	fprintf(stdout, "Body length : %d\n", length);
	frames_size = length;

	if ( has_extra_header ){
		if ( fread(buffer, 1, 4, src) != 4 ) return 0;
		int size = version == 3 ? parse_int(buffer) : parse_syncSafeInt(buffer);
		fseek(src, size, SEEK_CUR);
	}
	return 1;
}

int parse_syncSafeInt(char* bytes){
	int value = 0x0;
	value += (int)bytes[0];
	value <<= 7;
	value += (int)bytes[1];
	value <<= 7;
	value += (int)bytes[2];
	value <<= 7;
	value += (int)bytes[3];
	return value;
}

int parse_int(char* bytes){
	char temp = bytes[0];
	bytes[0] = bytes[3];
	bytes[3] = temp;
	temp = bytes[1];
	bytes[1] = bytes[2];
	bytes[2] = temp;
	int value;
	memcpy(&value, bytes, 4);
	return value;
}

int read_frame(FILE* src){
	char id[5];
	id[4] = '\0';
	if ( fread(id, 1, 4, src) != 4 ) return -1;
	if ( strlen(id) != 4 ) return -1;
	char size_buffer[4];
	if ( fread(size_buffer, 1, 4, src) != 4 ) return -1;
	int size = version == 3 ? parse_int(size_buffer) : parse_syncSafeInt(size_buffer);
	fseek(src, 2, SEEK_CUR);

	char* buffer = (char*)malloc(sizeof(char) * size);
	if ( buffer == NULL ) return -1;
	if ( fread(buffer, 1, size, src) != size ) return -1;

	char* str = (char*)malloc(sizeof(char) * (size * 2));

	if ( strcmp(id, "TIT2") == 0 ){
		if ( read_text(str, size*2, buffer, size, 0, 0)) fprintf(stdout, "TITLE : %s\n", str);
	}else if ( strcmp(id, "TPE1") == 0 ){
		if ( read_text(str, size*2, buffer, size, 0, 0)) fprintf(stdout, "ARTIST : %s\n", str);
	}else if ( strcmp(id, "TALB") == 0 ){
		if ( read_text(str, size*2, buffer, size, 0, 0)) fprintf(stdout, "ALBUM : %s\n", str);
	}else if ( strcmp(id, "USLT") == 0 ){
		if ( read_text(str, size*2, buffer, size, 1, 1)) fprintf(stdout, "LYRICS : %s\n", str);
	}else{
		fprintf(stdout, "ID : %s, size : %d\n", id, size);
	}

	free(buffer);
	free(str);
	return 10 + size;
}

int litle_edian(char* des, size_t max, char* src, size_t size){
    if (size % sizeof(wchar_t) != 0) {
        fprintf(stderr, "size mismatch.");
        return 0;
    }
    if (Utf16ToUtf8(des, max, (wchar_t*)src,
                    size / sizeof(wchar_t)) == FALSE) {
        fprintf(stderr, "fail convert.");
        return 0;
	}
	return 1;
}

int big_edian(char* des, size_t max, char* src, size_t size) {
    char temp;
    for (char* pt = src; pt < src + size; pt += 2) {
        temp = *pt;
        *pt = *(pt + 1);
        *(pt + 1) = temp;
	}
	return litle_edian(des, max, src, size);
}

int read_text(char* des, size_t max_des_size, char* src, size_t src_size, int language, int desc){
	char encoding = *src;
	int step;
	switch ( encoding ){
		case 1:
		case 2:
			step = 2;
			break;
		case 3:
			step = 1;
			break;
		default:
			fprintf(stderr, "unsupported text encoding : %d\n", src[0]);
			return 0;

	}
	int index = 1;
	if ( language ){
		char buf[4];
		memcpy(buf, src+index, 3);
		buf[3] = '\0';
		fprintf(stdout, "language : %s\n", buf);
		index += 3;
	}
	if ( desc ){
		if ( step == 1 ){
			while ( src[index] != '\0') index++;
		}else{
			fprintf(stdout, "des:\"");
			while ( src[index] != '\0' || src[index+1] != '\0' ){
				unsigned char b1 = src[index];
				unsigned char b2 = src[index+1];
				fprintf(stdout, "%x%x", b1, b2);
				char str[8];
				int result = Utf16ToUtf8(str, 8, (wchar_t*)(src + index), 1);
				if ( result != FALSE ){
					str[result] = '\0';
					fprintf(stdout, "%s", str);
				}
				index += 2;
			} 
			fprintf(stdout, "\"\n");
		}
		index += step;
	}
	switch ( encoding ){
		case 1:;
			int bom1 = (unsigned char)src[index];
			int bom2 = (unsigned char)src[index+1];
			index += 2;
			if ( bom1 == 0xFE && bom2 == 0xFF ){
				//fprintf(stdout, "UTF-16BE");
				return big_edian(des, max_des_size, src+index, src_size-index);
			}else if ( bom1 == 0xFF && bom2 == 0xFE ){
				//fprintf(stdout, "UTF-16LE");
				return litle_edian(des, max_des_size, src+index, src_size-index);
			}else{
				fprintf(stderr, "invalide endian : %x %x\n", bom1, bom2);
				return 0;
			}
		case 2:
			return big_edian(des, max_des_size, src+index, src_size-index);
		case 3:
			if ( src_size <= max_des_size ){
				memcpy(des, src, src_size);
			}else{
				memcpy(des, src, max_des_size-1);
				des[max_des_size-1] = '\0';
			}
			break;
        }
	return 1;
}

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
int Utf16ToUtf8(char* dest, size_t dest_size, wchar_t* src, size_t src_size) {
    UINT uCodePage;        /* コードページ */
    DWORD dwFlags;         /* 処理性能とマッピングのフラグ */
    LPCWSTR lpWideCharStr; /* ワイド文字の文字列のアドレス */
    int cchWideChar;       /* 文字列の文字数 */
    LPSTR lpMultiByteStr; /* 新しい文字列のバッファのアドレス */
    int cchMultiByte;     /* バッファのサイズ */
    LPCSTR
        lpDefaultChar; /* 文字をマップ不可能な場合のデフォルト文字のアドレス */
    LPBOOL
        lpfUsedDefaultChar; /* デフォルト文字が使われたときにセットされるフラグのアドレス
                             */
    int iResult;

    uCodePage = CP_UTF8;
    dwFlags = 0;
    lpWideCharStr = src;
    cchWideChar = src_size;
    lpMultiByteStr = dest;
    cchMultiByte = dest_size;
    lpDefaultChar = NULL;
    lpfUsedDefaultChar = NULL;
    iResult = WideCharToMultiByte(uCodePage, dwFlags, lpWideCharStr,
                                  cchWideChar, lpMultiByteStr, cchMultiByte,
                                  lpDefaultChar, lpfUsedDefaultChar);
    return iResult;
}