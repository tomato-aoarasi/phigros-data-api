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

#if DEBUG
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
			httplib::Error err{ httplib::Error::Success };

			m_headers.insert({ "X-LC-Session", sessionToken.data() });
			httplib::Client cli(URL);

			bool is_exception_1{ false }, is_exception_2{ false };
			HTTPException e1, e2;

			// 获取玩家自己信息的JSON
			std::thread get_player_info_thread([&] {
				auto res = cli.Get(ME_URI, m_headers);
				
				if (res && res->status == 200) {
					m_nickname = Json::parse(res->body)["nickname"].get<std::string>();
				}
				else if(res->status < 500 && res->status >= 400){
					is_exception_1 = true;
					uint16_t status_code = 1;
					switch (res->status)
					{
						case 400:
							status_code = 4;
							break;
						default:
							break;
					}
					e1 = HTTPException("", res->status, status_code);
				}else if (res.error() != httplib::Error::Success) {
					is_exception_1 = true;
					e1 = HTTPException(httplib::to_string(err), 500, 1);
				}else {
					is_exception_1 = true;
					e1 = HTTPException(httplib::to_string(res.error()), 500, 1);
				}
			});

			// 获取玩家存档的JSON
			auto res { cli.Get(GAME_SAVE_URI, m_headers) };
			if (res && res->status == 200) {
				this->m_game_save_info = Json::parse(res->body);
				this->m_game_save_info.swap(this->m_game_save_info["results"][0]);
			}else if (res.error() != httplib::Error::Success) {
				is_exception_2 = true;
				e2 = HTTPException(httplib::to_string(err), 500, 1);
			}else {
				throw HTTPException(httplib::to_string(res.error()), 500, 1);
			}

			get_player_info_thread.join();
			if (is_exception_1)
			{
				throw e1;
			}
			else if (is_exception_2) {
				throw e2;
			}

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
			auto result_zip{ game_save_zip.Get(save_uri) };
			err = result_zip.error();
			if (result_zip->status != 200) {
				throw HTTPException(httplib::to_string(err), 500, 1);
			}
			else if (err != httplib::Error::Success) {
				throw HTTPException(httplib::to_string(err), 500, 1);
			}

			mz_zip_archive zip_archive = {};
			mz_bool status = mz_zip_reader_init_mem(&zip_archive, result_zip->body.data(), result_zip->body.size(), 0);
			if (!status) {
				mz_zip_reader_end(&zip_archive);
				throw HTTPException("Decompression Failed", 500, 5);
			}

			try{
				for (int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); ++i) {
					mz_zip_archive_file_stat file_stat;
					mz_zip_reader_file_stat(&zip_archive, i, &file_stat);

					auto filename{ std::string(file_stat.m_filename) };

					//std::cout << "filename: " << filename << "\nsize: " << file_stat.m_uncomp_size << std::endl;

					
					if (filename == "user")
					{

						std::vector<ubyte> file_data(file_stat.m_uncomp_size);
						mz_zip_reader_extract_to_mem(&zip_archive, file_stat.m_file_index, file_data.data(), file_data.size(), 0);
						file_data.erase(file_data.begin());

						// 编码解码
						std::vector<ubyte> key{ OtherUtil::base64Decode(this->KEY_BASE64) };
						std::vector<ubyte> iv{ OtherUtil::base64Decode(this->IV_BASE64) };

						auto decrypt_data{ OtherUtil::decrypt_AES_CBC(file_data, key, iv) };

						auto data_size{ decrypt_data.size() };

						int string_size{ 128 * (decrypt_data[2] - 1) + decrypt_data[1] },
							init_pos{ 3 };
						if (string_size >= data_size) {
							init_pos = 2;
							string_size = decrypt_data[1];
						}
						try{
							if (static_cast<unsigned long>(init_pos) + string_size > data_size) {
								throw std::runtime_error("Profile Array Bound Error");
							}
							std::string profile(decrypt_data.begin() + init_pos, decrypt_data.begin() + init_pos + string_size);
							this->m_user_data.profile = std::move(profile);
							auto pos{ std::move(init_pos) + std::move(string_size) };
							auto avatar_size{decrypt_data[pos]};

							if (static_cast<unsigned long>(pos) + avatar_size + 1 > data_size) {
								throw std::out_of_range("Avatar Array Bound Error");
							}
							std::string avatar(decrypt_data.begin() + pos + 1, decrypt_data.begin() + pos + avatar_size + 1);
							this->m_user_data.avatar = std::move(avatar);
							pos += std::move(avatar_size) + 1;
							auto background_size{ decrypt_data[pos] };
							if (static_cast<unsigned long>(pos) + std::move(background_size) + 1 > data_size) {
								throw std::out_of_range("Background Array Bound Error");
							}
							std::string background(decrypt_data.begin() + pos + 1, decrypt_data.begin() + pos + std::move(background_size) + 1);
							this->m_user_data.background = std::move(background);
						}
						catch (const std::out_of_range& e) {
							LogSystem::logError(e.what());
						}
						catch (const std::runtime_error& e) {
							LogSystem::logError(e.what());
						}
					}

					if (filename == "gameRecord")
					{
						std::vector<ubyte> file_data(file_stat.m_uncomp_size);
						mz_zip_reader_extract_to_mem(&zip_archive, file_stat.m_file_index, file_data.data(), file_data.size(), 0);
						file_data.erase(file_data.begin());

						// 编码解码
						std::vector<ubyte> key{ OtherUtil::base64Decode(this->KEY_BASE64) };
						std::vector<ubyte> iv{ OtherUtil::base64Decode(this->IV_BASE64) };

						auto decrypt_data{ OtherUtil::decrypt_AES_CBC(file_data, key, iv) };
						//HexDebug(decrypt_data);
						getGameRecord(decrypt_data);
						break;
					}
				}
			}
			catch (...) {
				// 关闭ZIP文件
				mz_zip_reader_end(&zip_archive);
				throw HTTPException("Unknown Error", 500, 1);
			}
			
			// 关闭ZIP文件
			mz_zip_reader_end(&zip_archive);
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
	};
}

#endif