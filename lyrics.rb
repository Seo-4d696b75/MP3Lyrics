require 'net/http'
require 'openssl'
Encoding.default_external = 'UTF-8'

EXE_PATH = './lyrics.exe'
CA_PATH = 'hogehoge'

# カレントディレクトリの同名の.mp3 .txtファイルを探して歌詞を埋め込む.
# 歌詞のファイル.txtはUTF-8(without BOM)を仮定
# mp3ヘッダID3v2形式の v2.3~v2.4をサポート
# mp3ファイルにはUTF-16(with BOM)で書き込む
def imbed_lyrics()
	music = Dir.glob("*.mp3")
	lyric = Dir.glob("*.txt")
	music.each do |m|
		name = /(.+)\.mp3/.match(m)[1]
		if lyric.include?(name+".txt")
			system("#{EXE_PATH} \"#{name}\"")
		end
	end
end

# 歌詞検索サービス https://www.uta-net.com/ から歌詞データをダウンロード
# @param audio 歌詞に対応する楽曲ファイル.mp3
#              出力ファイルは{楽曲ファイル名}.txtとなる
# @param path ページのURL
def get_lyrics(audio,path)
	puts "audio #{audio} \npath  #{path}"
	if (m=/(.+)\.mp3/.match(audio)) == nil
		puts "invalide audio file : #{audio}"
		return
	end
	des = m[1]+".txt"
	return if ( m = /^https:\/\/.+?(\/.+)$/.match(path) ) == nil
	t = m[1]
	uri = URI.parse(path)
	https = Net::HTTP.new(uri.host, uri.port)
  https.use_ssl = true
	https.verify_mode = OpenSSL::SSL::VERIFY_PEER
  https.verify_depth = 5
  https.ca_file = CA_PATH
  res = https.start { |w| w.get(t) }
  if res.code != '200'
    puts "Error > code : " + res.code
    return
	end
	if ( m = /<div id="kashi_area".+?>(.+?)<\/div>/.match(res.body)) == nil
		puts "fail to detecte lyrics area"
		return
	end

	f = File.open(des, "w")
	f.puts(m[1].gsub(/<br\s?\/?>/, "\n"))
	f.close

	puts "des   #{des}"

end

if ARGV.length == 0
	puts "imbed lyrics into audio file..."
	imbed_lyrics()
elsif ARGV.length == 2
	get_lyrics(ARGV[0], ARGV[1])
else
	puts "invalide params."
end

