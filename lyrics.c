/**
 * ファイル名を指定してmp3楽曲にtxtファイルで与える歌詞を埋め込む.
 * usage: $./lyrics.exe {file_name}
 * 以下の２ファイルが同ディレクトリに存在すること。
 * 楽曲 {file_name}.mp3 : ID3v2.3~v2.4形式のヘッダ付き
 * 歌詞ファイル {file_name}.txt : UTF-8仮定 (BOMなし)
 * 
 * 歌詞データ(文字列)を以下の形式で書き込む
 * エンコーディング：UTF-16(LE, BOM付き)
 * 言語："jpn"
 * 説明テキスト：""
 * 内容テキスト："{歌詞の文字列}"
 * 
 * mp3ヘッダの詳しい形式の参考
 * http://eleken.y-lab.org/report/other/mp3tags.shtml
 * https://akabeko.me/blog/memo/mp3/id3v2-frame-list/
 * https://akabeko.me/blog/memo/mp3/id3v2-frame-detail/
 *
 * 窓+MinGW での使用を前提として設計
 * ヘッダ内のマルチバイトの数値表現はすべてBigEndianなので計算機内表現LittleEndianと変換する必要あり
 *
 * @author Seo-4d696b75
 * @version 2019-01-16
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>
#include <fcntl.h>
#include "charset.h"

char* file_name;
int version;  // supposed to be either 3 or 4
char header_flag;
long lyrics_offset = -1;
int lyrics_size = 0;

int parse_syncSafeInt(char* bytes);
int parse_int(char* bytes);
void encode_int(char* des, int src);
void encode_syncSafeInt(char* des, int src);

int read_header(FILE* src);
int read_frame(FILE* src);
int write_lyrics(FILE* src, FILE* des, FILE* text, int size);

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "invalid param size.\n");
        return 1;
	}
	file_name = argv[1];
    char str[1024];
    FILE* audio;
    sprintf(str, "%s.mp3", file_name);
    if ((audio = fopen(str, "rb")) == NULL) {
        fprintf(stderr, "fail to open audio.\n");
        return 1;
	}
	fprintf(stdout, "%s.mp3\n", file_name);
    int frames_size = read_header(audio);
    if (frames_size == 0) {
        fprintf(stderr, "fail to read header.\n");
        return 1;
    }
    int body_offset = 10;
    while (body_offset < frames_size + 10) {
        int result = read_frame(audio);
        if (result == 0) {
            break;
        } else if (result < 0) {
            fclose(audio);
            return 1;
        }
        body_offset += result;
    }
    if (body_offset > frames_size + 10) {
        fprintf(stderr, "frame size mismatched.\n");
        fclose(audio);
        return 1;
    }
    if (lyrics_offset < 0) lyrics_offset = body_offset;
    fseek(audio, 0, SEEK_SET);

    sprintf(str, "%s.txt", argv[1]);
    FILE* text = fopen(str, "rb");
    if (text == NULL) {
        fprintf(stderr, "fail to open lyrics text.\n");
        fclose(audio);
        return 1;
    }
    FILE* des = fopen("temp.mp3", "wb");
    if (des == NULL) {
        fclose(audio);
        fclose(text);
        return 1;
    }

    if (write_lyrics(audio, des, text, body_offset - 10)) {
        fclose(des);
        fclose(audio);
		fclose(text);
		sprintf(str, "%s.mp3", file_name);
		if ( remove(str) != 0 ){
			fprintf(stderr, "fail to remove original file.\n");
			return 1;
		}
		if ( rename("temp.mp3", str) != 0 ){
			fprintf(stderr, "fail to rename output file.\n");
			return 1;
		}
        fprintf(stdout, "  success.\n");
    } else {
        fclose(des);
        fclose(audio);
        fclose(text);
        return 1;
    }

    return 0;
}

int read_header(FILE* src) {
    char buffer[4];
    if (fread(buffer, 1, 3, src) != 3) return 0;
    buffer[3] = '\0';
    if (strcmp(buffer, "ID3") != 0) {
        fprintf(stderr, "invalid header identifer.");
        return 0;
    }
    if (fread(buffer, 1, 3, src) != 3) return 0;
    fprintf(stdout, "ID3v2.%d.%d ", buffer[0], buffer[1]);
    if (buffer[0] != 3 && buffer[0] != 4) {
        fprintf(stderr, "unsupported version detected.\n");
        return 0;
    }
    version = buffer[0];
    header_flag = buffer[2];
    int has_extra_header = (buffer[2] & 0b00100000) > 0;

    if (fread(buffer, 1, 4, src) != 4) return 0;
    int length = parse_syncSafeInt(buffer);

    if (has_extra_header) {
        if (fread(buffer, 1, 4, src) != 4) return 0;
        int size = version == 3 ? parse_int(buffer) : parse_syncSafeInt(buffer);
        fseek(src, size, SEEK_CUR);
    }
    return length;
}

int parse_syncSafeInt(char* bytes) {
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

int parse_int(char* bytes) {
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


void encode_int(char* des, int src){
	memcpy(des, &src, 4);
	char temp = des[0];
	des[0] = des[3];
	des[3] = temp;
	temp = des[1];
	des[1] = des[2];
	des[2] = temp;
}

void encode_syncSafeInt(char* des, int src){
	unsigned int val = src & 0x0FFFFFFF;
	des[3] = (char)( 0x7F & val);
	val >>= 7;
	des[2] = (char)( 0x7F & val);
	val >>= 7;
	des[1] = (char)( 0x7F & val);
	val >>= 7;
	des[0] = (char)( 0x7F & val);
}

int read_frame(FILE* src) {
    long pos = ftell(src);
    char buffer[5];
    buffer[4] = '\0';
    if (fread(buffer, 1, 4, src) != 4) return -1;
    if (strlen(buffer) != 4) return 0;
    for (int i = 0; i < 4; i++) {
        if (('0' <= buffer[i] && buffer[i] <= '9') ||
            ('A' <= buffer[i] && buffer[i] <= 'Z')) {
            continue;
        } else {
            return 0;
        }
    }
    char size_buffer[4];
    if (fread(size_buffer, 1, 4, src) != 4) return -1;
    int size =
        version == 3 ? parse_int(size_buffer) : parse_syncSafeInt(size_buffer);
    fseek(src, 2, SEEK_CUR);

    if (strcmp(buffer, "USLT") == 0) {
        if (fread(buffer, 1, 4, src) != 4) return -1;
        if (strncmp(buffer + 1, "jpn", 3) == 0) {
            lyrics_offset = pos;
            lyrics_size = size + 10;
            return 0;
        }
        fseek(src, size - 4, SEEK_CUR);
    } else {
        fseek(src, size, SEEK_CUR);
    }

    return 10 + size;
}

int write_lyrics(FILE* src, FILE* des, FILE* text, int size) {

    char* buffer = (char*)malloc(sizeof(char) * lyrics_offset);
    if (fread(buffer, 1, lyrics_offset, src) != lyrics_offset) return 0;
    if (fwrite(buffer, 1, lyrics_offset, des) != lyrics_offset) return 0;
    free(buffer);

	char name[256];
	sprintf(name, "%s.txt", file_name);
	struct stat stbuf;
	if ( stat(name, &stbuf) == -1 ) return 0;
	long text_size = stbuf.st_size;

	buffer = (char*)malloc(sizeof(char) * (text_size+1));
	if ( fread(buffer, 1, text_size, text) != text_size ) return 0;
	buffer[text_size] = '\0';

	wchar_t* str = (wchar_t*)malloc(sizeof(wchar_t) * (text_size + 1));
	int result = Utf8ToUtf16(str, text_size+1, buffer, text_size+1);
	free(buffer);
	if ( result == FALSE ) return 0;


	int new_size = sizeof(wchar_t) * result;
	int content_size = new_size + 10;
	int frame_size = content_size + 10;

	char buf[1024];
	memcpy(buf, "USLT", 4);
	fwrite(buf, 1, 4, des);
	if ( version == 3 ){
		encode_int(buf, content_size);
	}else{
		encode_syncSafeInt(buf, content_size);
	}
	fwrite(buf, 1, 4, des);
	buf[0] = (char)0;
	buf[1] = (char)0;
	buf[2] = (char)1;
	memcpy(buf+3, "jpn", 3);
	buf[6] = (char)0xff;
	buf[7] = (char)0xfe;
	buf[8] = (char)0;
	buf[9] = (char)0;
	buf[10] = (char)0xff;
	buf[11] = (char)0xfe;
	fwrite(buf, 1, 12, des);
	fwrite(str, 1, new_size, des);
	free(str);

	fseek(src, lyrics_size, SEEK_CUR);
	size = size + frame_size - lyrics_size;
	fseek(des, 6, SEEK_SET);
	encode_syncSafeInt(buf, size);
	fwrite(buf, 1, 4, des);
	fseek(des, lyrics_offset + frame_size, SEEK_SET);

	result = 1;
	while ( 1 ){
		result = fread(buf, 1, 1024, src);
		if ( result == 0 ) break;
		fwrite(buf, 1, result, des);
	}

	return 1;

}