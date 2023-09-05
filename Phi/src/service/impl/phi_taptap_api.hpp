/*
 * @File	  : phi_taptap_api.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/10 16:22
 * @Introduce : phi对应yuhao的python代码重写
*/


#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <regex>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <cmath>
#include "httplib.h"
#include <miniz.h>
#include "configuration/config.hpp"
#include "common/utils/other_util.hpp"
#include <cryptopp/cryptlib.h>

#ifndef PHI_TAPTAP_API_HPP
#define PHI_TAPTAP_API_HPP  

using ubyte = unsigned char;

#if 0
void HexDebug(const auto& content) {
	for (const auto& data : content)
	{
		std::cout << std::format("{:02X}", data);
	};
	std::cout << std::endl;
}
#endif // DEBUG


namespace self {

	class BinaryReader {
	private:
		std::vector<uint8_t> data_;
		size_t pos_ = 0;
		bool big_endian_{ };

	public:
		BinaryReader(std::vector<uint8_t>& stream, bool big_endian = true) : big_endian_{ big_endian } {
			if (stream.empty()) {
				return;
			}

			if (stream.data() != nullptr) {
				data_ = stream;
				pos_ = 0;
			}
		}

		uint8_t ReadByte() {
			uint8_t value = data_[pos_];
			pos_ += 1;
			return value;
		}

		int32_t ReadInt32() {
			int32_t value;
			std::memcpy(&value, &data_[pos_], sizeof(value));
			if (big_endian_) {
				OtherUtil::littleBigEndianConversion<int32_t>(value);
			}
			pos_ += sizeof(value);
			return value;
		}

		float ReadSingle() {
			float value;
			std::memcpy(&value, &data_[pos_], sizeof(value));
			if (big_endian_) {
				OtherUtil::littleBigEndianConversion<float>(value);
			}
			pos_ += sizeof(value);
			return value;
		}

		std::string ReadStr() {
			uint8_t strLength = ReadByte();
			std::string str(reinterpret_cast<const char*>(&data_[pos_]), strLength);
			pos_ += strLength;
			return str;
		}

		const auto& getPosition() const {
			return this->pos_;
		}

		bool getBit(uint8_t data, uint8_t index) {
			return (data & (1 << index)) != 0;
		}

		void setPosition(size_t pos) {
			this->pos_ = pos;
		}
	};

	struct GameProgress {
		uint8_t isFirstRun{ 0 };
		uint8_t legacyChapterFinished{ 0 };
		uint8_t alreadyShowCollectionTip{ 0 };
		uint8_t alreadyShowAutoUnlockINTip{ 0 };
		uint8_t completed{ 0 };
		uint8_t songUpdateInfo{ 0 };
		short challengeModeRank{ 0 };
		std::array<short, 5> data{ 0,0,0,0,0 };
		uint8_t unlockFlagOfSpasmodic{};
		uint8_t unlockFlagOfIgallta{ 0 };
		uint8_t unlockFlagOfRrharil{ 0 };
		uint8_t flagOfSongRecordKey{ 0 };
		uint8_t randomVersionUnlocked{ 0 };
		uint8_t chapter8UnlockBegin{ 0 };
		uint8_t chapter8UnlockSecondPhase{ 0 };
		uint8_t chapter8Passed{ 0 };
		uint8_t chapter8SongUnlocked{ 0 };

#if 0
		void Print() {
			std::cout // << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
				<< static_cast<int>(isFirstRun) << ", "
				<< static_cast<int>(legacyChapterFinished) << ", "
				<< static_cast<int>(alreadyShowCollectionTip) << ", "
				<< static_cast<int>(alreadyShowAutoUnlockINTip) << ", "
				<< static_cast<int>(completed) << ", "
				<< static_cast<int>(songUpdateInfo) << ", "
				<< "challengeModeRank: " << challengeModeRank << ", \n"
				<< "data: " << data[0] << ", "
				<< data[1] << ", "
				<< data[2] << ", "
				<< data[3] << ", "
				<< data[4] << ", \n"
				<< static_cast<int>(unlockFlagOfSpasmodic) << ", "
				<< static_cast<int>(unlockFlagOfIgallta) << ", "
				<< static_cast<int>(unlockFlagOfRrharil) << ", "
				<< static_cast<int>(flagOfSongRecordKey) << ", "
				<< static_cast<int>(randomVersionUnlocked) << ", "
				<< static_cast<int>(chapter8UnlockBegin) << ", "
				<< static_cast<int>(chapter8UnlockSecondPhase) << ", "
				<< static_cast<int>(chapter8Passed) << ", "
				<< static_cast<int>(chapter8SongUnlocked) << std::endl;
		};
#endif
	};

	struct SongScore {
		float acc{};
		unsigned int score{};
		bool is_fc{};
		std::string difficulty{};
	};

	struct UserData {
		std::string profile{}, avatar{}, background{};
	};

	class PhiTaptapAPI {
	public:

		class CloudSaveSummary {
		public:
			std::string updatedAt;
			std::time_t timestamp;
			float RankingScore;
			uint16_t ChallengeModeRank;
			std::vector<uint8_t> EZ;
			std::vector<uint8_t> HD;
			std::vector<uint8_t> IN;
			std::vector<uint8_t> AT;
			std::string nickname;

			CloudSaveSummary(std::string Summary, std::string UpdateTime, std::string nickname) {
				const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

				auto b64_to_int = [&](char c) -> int {
					size_t pos = base64_chars.find(c);
					
					/*
					if (pos == std::string::npos) {
						throw std::runtime_error("Invalid base64 character");
					}
					*/
					
					return pos;
				};

				auto decode_base64 = [&](const std::string& encoded_string) -> std::vector<uint8_t> {
					size_t in_len = encoded_string.size();
					size_t i = 0;
					size_t j = 0;
					int in_ = 0;
					uint8_t char_array_4[4]{}, char_array_3[3]{};
					std::vector<uint8_t> ret;

					while (in_len-- && (encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || encoded_string[in_] == '+'
						|| encoded_string[in_] == '/')) {
						char_array_4[i++] = encoded_string[in_]; in_++;
						if (i == 4) {
							for (i = 0; i < 4; i++) {
								char_array_4[i] = b64_to_int(char_array_4[i]);
							}
							char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
							char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
							char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

							for (i = 0; (i < 3); i++) {
								ret.push_back(char_array_3[i]);
							}
							i = 0;
						}
					}

					if (i) {
						for (j = i; j < 4; j++) {
							char_array_4[j] = 0;
						}
						for (j = 0; j < 4; j++) {
							char_array_4[j] = b64_to_int(char_array_4[j]);
						}
						char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
						char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
						char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

						for (j = 0; (j < i - 1); j++) {
							ret.push_back(char_array_3[j]);
						}
					}

					return ret;
				};

				std::vector<uint8_t> summary = decode_base64(Summary);
				std::memcpy(&RankingScore, summary.data() + 3, 4);
				std::memcpy(&ChallengeModeRank, summary.data() + 1, 2);

				//std::cout << "size: " << summary.size() << std::endl;
				size_t size{ summary.size() }, byte_position{ size - 26 };

				//HexDebug(summary);
				EZ = { summary[byte_position += 2], summary[byte_position += 2], summary[byte_position += 2] };
				HD = { summary[byte_position += 2], summary[byte_position += 2], summary[byte_position += 2] };
				IN = { summary[byte_position += 2], summary[byte_position += 2], summary[byte_position += 2] };
				AT = { summary[byte_position += 2], summary[byte_position += 2], summary[byte_position += 2] };

				std::tm tm = {};
				std::istringstream ss(UpdateTime);
				ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
				std::time_t t = std::mktime(&tm);
				t += 8L * 3600;

				std::tm* tm1{ std::localtime(&t) };
				char buffer[80];
				std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", tm1);
				

				this->updatedAt = buffer;
				this->timestamp = t;
				this->nickname = nickname;
			}
		};

	private:
		httplib::Headers m_headers{
			{"X-LC-Id", "rAK3FfdieFob2Nn8Am"},
			{"X-LC-Key", "Qr9AEqtuoSVS3zeD6iVbM4ZC0AtkJcQ89tywVyi0"},
			{"Content-Type", "application/json"}
		};

		// progress信息
		GameProgress m_gameProgress{};

		// 曲目id/难度(0/1/2/3/4)/信息
		std::unordered_map<std::string, std::unordered_map<ubyte, SongScore>> m_player_record{};

		std::string
			m_nickname{};

		UserData m_user_data{};

		Json m_player_info, m_game_save_info;

		inline static const std::string
			URL{ Config::getConfig()["server"]["data-url"].as<std::string>() },
			GAME_SAVE_URI{ "/1.1/classes/_GameSave" },
			ME_URI{ "/1.1/users/me" },
			KEY_BASE64{ "6Jaa0qVAJZuXkZCLiOa/Ax5tIZVu+taKUN1V1nqwkks=" },
			IV_BASE64{ "Kk/wisgNYwcAV8WVGMgyUw==" };

		void getGameRecord(std::vector<ubyte>& data) {
			size_t data_size{ data.size() };
			std::unordered_map<std::string, std::unordered_map<ubyte, SongScore>> records;

			try{
				BinaryReader reader(data);
				auto songcount{ reader.ReadByte() };
				auto fc{ reader.ReadByte() };

				for (size_t i = 0; i < songcount; ++i) {
					const std::string levels[]{ "EZ", "HD", "IN", "AT", "Legacy" };
					auto songid{ reader.ReadStr() };
					auto length{ reader.ReadByte() };
					auto diffs{ reader.ReadByte() };
					fc = reader.ReadByte();

					//std::cout << songid << "\n";

					std::unordered_map<ubyte, SongScore> record;
					for (size_t j = 0; j < 5; ++j) {
						if (reader.getBit(diffs, j)) {
							SongScore song_score;
							song_score.score = reader.ReadInt32();
							song_score.acc = reader.ReadSingle();
							song_score.is_fc = reader.getBit(fc, j);
							song_score.difficulty = levels[j];
							record[j] = std::move(song_score);
						}
					}

					if (reader.getPosition() > data_size){
						records.clear();

						//std::cout << "Binary data read out of bounds\n";

						throw std::out_of_range("Binary data read out of bounds");
					}

					records[songid] = std::move(record);
				}
			}catch (const std::out_of_range& e) {
				BinaryReader reader(data);
				auto songcount{ reader.ReadByte() };

				for (size_t i = 0; i < songcount; ++i) {
					std::string levels[]{ "EZ", "HD", "IN", "AT", "Legacy" };
					auto songid{ reader.ReadStr() };
					auto length{ reader.ReadByte() };
					auto diffs{ reader.ReadByte() };
					auto fc = reader.ReadByte();


					std::unordered_map<ubyte, SongScore> record;
					for (size_t j = 0; j < 5; ++j) {
						if (reader.getBit(diffs, j)) {
							SongScore song_score;
							song_score.score = reader.ReadInt32();
							song_score.acc = reader.ReadSingle();
							song_score.is_fc = reader.getBit(fc, j);
							song_score.difficulty = levels[j];
							record[j] = std::move(song_score);
						}
					}

					if (reader.getPosition() > data_size) {
						records.clear();
						throw std::out_of_range("Binary data read out of bounds");
					}

					records[songid] = std::move(record);
				}
			}

			//std::cout << "data size: " << data_size << ",position: " << reader.getPosition() << std::endl;
			this->m_player_record = std::move(records);
		}
	public:
		PhiTaptapAPI(std::string_view sessionToken) {
			std::string
				filename{ "save" },
				dir_path{ Global::PlayerSavePath + "/" + sessionToken.data() + "/" },
				file_path{ dir_path + filename };


			httplib::Error err{ httplib::Error::Success };

			m_headers.insert({ "X-LC-Session", sessionToken.data() });
			httplib::Client cli(URL);

			try{
				// 异步获取玩家自己信息的JSON
				std::future<void> get_player_info_thread = std::async(std::launch::async, [&] {
					auto res = cli.Get(ME_URI, m_headers);

					//std::cout << httplib::to_string(res.error()) << std::endl;
					if (res.error() != httplib::Error::Success) {
						throw HTTPException(httplib::to_string(err), 500, 1);
					}
					else if (res && res->status == 200) {
						this->m_nickname = Json::parse(res->body)["nickname"].get<std::string>();
					}
					else if (res->status < 500 && res->status >= 400) {
						uint16_t status_code = 1;
						switch (res->status)
						{
						case 400:
							status_code = 4;
							break;
						default:
							break;
						}
						throw HTTPException("", res->status, status_code);
					}
					else {
						throw HTTPException(httplib::to_string(res.error()), 500, 1);
					}
					});

				// 获取玩家存档的JSON
				auto res{ cli.Get(GAME_SAVE_URI, m_headers) };
				if (res.error() != httplib::Error::Success) {
					throw HTTPException(httplib::to_string(err), 500, 1);
				} else if (res && res->status == 200) {
					this->m_game_save_info = Json::parse(res->body);
					this->m_game_save_info.swap(this->m_game_save_info["results"][0]);
				} else {
					throw HTTPException(httplib::to_string(res.error()), 500, 1);
				}

				get_player_info_thread.get();
			}
			catch (const nlohmann::json::type_error&) {
				// plan B启动
				if (Global::IsPlanB)
				{
					LogSystem::logInfo("悲观主义的一套planB");

					mz_zip_archive zip_archive{};
					memset(&zip_archive, 0, sizeof(zip_archive));
					mz_bool status{ mz_zip_reader_init_file(&zip_archive, file_path.c_str(), 0) };
					if (!status) {
						mz_zip_reader_end(&zip_archive);
						throw HTTPException("Decompression Failed", 500, 5);
					}
					#include "zip_archive"
					// 突然终止
					return;
				}
				throw;
			} 
			catch (const HTTPException&) {
				throw;
			}
			catch (const std::exception&) {
				throw;
			};

			std::string
				save_url{ this->m_game_save_info["gameFile"]["url"].get<std::string>() },
				save_domain{},
				save_uri{};

			// 正则表达式解析域名和url
			std::regex url_regex("(https?://[^/]+)(/.*)");

			if (std::smatch url_match; std::regex_match(save_url, url_match, url_regex)) {
				save_domain = std::move(url_match[1]);
				save_uri = std::move(url_match[2]);
			}
			else throw std::runtime_error("Invalid URL");

			// 发送HTTP GET请求
			httplib::Client game_save_zip(save_domain);
			httplib::Result result_zip{ game_save_zip.Get(save_uri) };
			err = result_zip.error();
			if (err != httplib::Error::Success) {
				throw HTTPException(httplib::to_string(err), 500, 1);
			}
			else if (result_zip->status != 200) {
				throw HTTPException(httplib::to_string(err), 500, 1);
			}

			// ==========================================
			// 存储文件(临时用)
			bool is_exists{ false }, do_save{false};
			SQL_Util::LocalDB << "select COUNT(sessionToken) from PlayerKey where sessionToken = ?;" << sessionToken.data() >> is_exists;
			std::string key{ this->m_game_save_info["gameFile"]["key"].get<std::string>() };
			// 如果不存在字段
			if (!is_exists){
				// 第一个用户新建字段
				SQL_Util::LocalDB  << "insert into PlayerKey(sessionToken,key,nickname) values (?,?,?);"
					<< sessionToken.data()
					<< key
					<< this->m_nickname;
				do_save = true;
				LogSystem::logInfo(std::format("已新增{}字段", sessionToken.data()));
			}
			else {
				std::string key_old{};
				SQL_Util::LocalDB << "select key from PlayerKey where sessionToken = ?;" << sessionToken.data() >> key_old;
				if (key_old != key) {
					// key不相同更新字段
					SQL_Util::LocalDB << "UPDATE PlayerKey SET key = ? WHERE sessionToken = ?"
						<< key
						<< sessionToken.data();
					do_save = true;
					LogSystem::logInfo(std::format("已更新{}的字段", sessionToken.data()));
				}
			}
			// 是否需要存储呢
			if (do_save){
				namespace fs = std::filesystem;
				
				// 如果目录不存在，则创建它
				if (!fs::exists(dir_path)) {
					fs::create_directories(dir_path);
				}

				std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
				if (file.is_open()) {
					// 将响应体写入文件
					file.write(result_zip->body.c_str(), result_zip->body.length());
					file.close();
					LogSystem::logInfo(std::format("已将{}存储到SQLite中", sessionToken.data()));
				}
				else {
					file.close();
					throw HTTPException("Failed to save file to "s + file_path, 500, 1);
				}
			}
			// ==========================================

			mz_zip_archive zip_archive {};
			memset(&zip_archive, 0, sizeof(zip_archive));

			mz_bool status{ mz_zip_reader_init_mem(&zip_archive, result_zip->body.data(), result_zip->body.size(), 0)};
			if (!status) {
				mz_zip_reader_end(&zip_archive);
				throw HTTPException("Decompression Failed", 500, 5);
			}
			#include "zip_archive"
		};

		~PhiTaptapAPI() noexcept {
			m_player_info.clear();
			m_game_save_info.clear();
		};

		std::string getNickname() const {
			return this->m_nickname;
		};

		auto GetSummary() const {
			return CloudSaveSummary(
				this->m_game_save_info["summary"].get<std::string>(),
				this->m_game_save_info["gameFile"]["createdAt"].get<std::string>(),
				this->m_nickname);
		};

		auto& getPlayerRecord() const {
			return this->m_player_record;
		}

		auto& getUserData() const {
			return this->m_user_data;
		}

		auto& getGameProgress() const {
			return this->m_gameProgress;
		}
	};
}

#endif