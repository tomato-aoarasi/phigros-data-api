/*
 * @File	  : phigros_service_impl.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/17 21:40
 * @Introduce : Phigros
*/

#pragma once

#include <service/phigros_service.hpp>
#include <service/impl/phi_taptap_api.hpp>
#include "configuration/config.hpp"
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#ifndef PHIGROS_SERVICE_HPP_IMPL
#define PHIGROS_SERVICE_HPP_IMPL  
class PhigrosServiceImpl : public PhigrosService {
private:
	struct KeyComparator {
		bool operator()(const auto& lhs, const auto& rhs) const {
			return lhs > rhs;
		}
	};
	struct personalPhiSongInfo {
		std::string sid;
		int id;
		std::string title;
		std::string song_illustration_path;
		float level;
		std::string difficulty{};
		float rks;
		float acc;
		float rating;
		bool is_fc;
		unsigned int score;
	};
	
	inline void player_records(std::string_view sessionToken, self::PhiTaptapAPI::CloudSaveSummary& cloudSaveSummary) {
		bool is_exists{ false }, is_timestamp_same {false};
		std::string st{ sessionToken.data() };
		SQL_Util::PlayerRdDB <<
			"SELECT EXISTS(SELECT 1 FROM sqlite_master WHERE type='table' AND name=?);"
			<< st >> is_exists;

		// 表不存在(第一次记录)
		if (!is_exists){
			//std::cout << "create" << std::endl;
			SQL_Util::PlayerRdDB << 
				"CREATE TABLE if not exists \"" + st + "\" ( "
				"sid integer PRIMARY KEY AUTOINCREMENT NOT NULL,"
				"rks text, "
				"challengeModeRank integer, "
				"timestamp date, "
				"nickname text );";
			//std::cout << "create end" << std::endl;
		}
		// 表存在
		else 
		{
			//std::cout << "timestamp" << std::endl;
			std::time_t timestamp_temp {};
			SQL_Util::PlayerRdDB << "SELECT timestamp FROM \"" + st + "\" WHERE sid = (SELECT MAX(sid) FROM \"" + st + "\"); " >> timestamp_temp;
			is_timestamp_same = timestamp_temp == cloudSaveSummary.timestamp;
			//std::cout << "timestamp end" << std::endl;
		}

		if (!is_timestamp_same){
			//std::cout << "insert" << std::endl;
			SQL_Util::PlayerRdDB << "insert into \"" + st + "\" (rks,challengeModeRank,timestamp,nickname) values (?,?,?,?);"
				<< cloudSaveSummary.RankingScore
				<< cloudSaveSummary.ChallengeModeRank
				<< cloudSaveSummary.timestamp
				<< cloudSaveSummary.nickname;
			//std::cout << "insert end" << std::endl;
		}
	};

	inline void updateSearch(int32_t id) {
		Json song_info;
		SQL_Util::PhiDB << "select sid,id,title from phigros where id = ?" << id >> [&]
		(std::unique_ptr<std::string> sid_p, int32_t id, std::string title) {
			if (sid_p)song_info["sid"] = *sid_p;
			else song_info["sid"] = nullptr;

			song_info["id"] = id;
			song_info["title"] = title;
			SQL_Util::PhiDB << "select id,alias from alias where song_id = ?" << id >> [&]
			(int32_t id, std::string alias) {
				Json alias_data;
				song_info["aliases"].emplace_back(Json{
					{"aliasId", id},
					{"alias", alias},
					});
			};
		};

		web::http::client::http_client client(U(Global::Meilisearch::Url));
		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::POST);
		request_add_index.set_request_uri("/indexes/"s + Global::Meilisearch::Phi::SearchNamespace + "/documents?primaryKey=id"s);
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + Global::Meilisearch::Authorization);
		request_add_index.set_body(web::json::value::parse(song_info.dump()));

		auto response = client.request(request_add_index).get();

		if (response.status_code() >= 300 or response.status_code() < 200) {
			auto error = response.extract_string().get();
			throw self::HTTPException(error, 500, 12);
		}
	};
public:
	virtual ~PhigrosServiceImpl() = default;
	Json getAll(const UserData& authentication, std::string_view sessionToken) override {

		Json data;

		// umyckc74rluncpn7mtxkcanxn
		// yc443mp6cea7xozb3e0kxvvid
		// qdpliq0laha53lfzfptyimz1j
		// v6yitajqe20ceim211502r3h0
		// 496bcu67y7j65oo900f9oznja
		self::PhiTaptapAPI phiAPI(sessionToken);

		auto phi{ phiAPI.getPlayerRecord() };

		std::multimap<float, personalPhiSongInfo, KeyComparator> rksSort;
		std::multimap<float, personalPhiSongInfo, KeyComparator> singlePhi;

		//std::cout << "\n=====================\n";
		auto playerSummary{ phiAPI.GetSummary()};
		auto playerData{ phiAPI.getUserData() };

		// 玩家记录
		player_records(sessionToken, playerSummary);

		data["content"]["playerNickname"] = phiAPI.getNickname();
		data["content"]["challengeModeRank"] = playerSummary.ChallengeModeRank;
		data["content"]["rankingScore"] = playerSummary.RankingScore;
		data["content"]["updateTime"] = playerSummary.updatedAt;
		data["content"]["timestamp"] = playerSummary.timestamp;
		{
			data["content"]["other"]["records"]["AT"]["clear"] = playerSummary.AT[0];
			data["content"]["other"]["records"]["AT"]["fc"] = playerSummary.AT[1];
			data["content"]["other"]["records"]["AT"]["phi"] = playerSummary.AT[2];
			data["content"]["other"]["records"]["IN"]["clear"] = playerSummary.IN[0];
			data["content"]["other"]["records"]["IN"]["fc"] = playerSummary.IN[1];
			data["content"]["other"]["records"]["IN"]["phi"] = playerSummary.IN[2];
			data["content"]["other"]["records"]["HD"]["clear"] = playerSummary.HD[0];
			data["content"]["other"]["records"]["HD"]["fc"] = playerSummary.HD[1];
			data["content"]["other"]["records"]["HD"]["phi"] = playerSummary.HD[2];
			data["content"]["other"]["records"]["EZ"]["clear"] = playerSummary.EZ[0];
			data["content"]["other"]["records"]["EZ"]["fc"] = playerSummary.EZ[1];
			data["content"]["other"]["records"]["EZ"]["phi"] = playerSummary.EZ[2];

			std::string avatar_id{ playerData.avatar };
			if (Global::PhigrosPlayerAvatar.count(avatar_id)) {
				if (authentication.authority == 5) {
					data["content"]["other"]["avatarPath"] = Global::PhigrosPlayerAvatar.at(avatar_id).avatar_path;
					data["content"]["other"]["avatarDbId"] = Global::PhigrosPlayerAvatar.at(avatar_id).sid;
				}
				data["content"]["other"]["avatarHasEnable"] = true;
			}else data["content"]["other"]["avatarHasEnable"] = false;

			data["content"]["other"]["avatar"] = avatar_id;
			data["content"]["other"]["background"] = playerData.background;
			data["content"]["other"]["profile"] = playerData.profile;
		}

		for (const auto& [key, value] : Global::PhigrosSongInfo) {
			std::string levels[]{ "EZ", "HD", "IN", "AT", "Legacy" };
			//std::cout << "\n=====================\n( " << key << " )\n";
			auto content{ phi[key] };
			// <= 4为有LG但是在计算b19并不会包含lg
			for (size_t i = 0; i < 4; i++)
			{
				try
				{
					unsigned int score{ content.at(i).score };
					float
						acc{ content.at(i).acc },
						rks{ 0.0f },
						rate{ value.rating[i] };
					std::string difficulty{ content.at(i).difficulty };
					bool is_fc{ content.at(i).is_fc };
					if (acc >= 70) rks = (std::pow((acc - 55) / 45, 2)) * rate;

					personalPhiSongInfo info;
					info.acc = acc;
					info.sid = key;
					info.id = value.id;
					info.is_fc = is_fc;
					info.difficulty = difficulty;
					info.score = score;
					info.level = rate;
					info.rks = rks;
					info.title = value.title;
					info.song_illustration_path = value.song_illustration_path;

					if (score >= 1000000)
					{
						singlePhi.insert(std::make_pair(rks, info));
					}
					rksSort.insert(std::make_pair(rks, info));

					/*std::cout << "[" << difficulty << " / " << rate << "] Score: " << score
						<< ", Acc: " << acc << ", RankingSocre: " << rks
						<< ", Is FC: " << is_fc
						<< '\n';*/
				}
				catch (const std::out_of_range e)
				{
					//std::cout << "[" << levels[i] << "] No Record\n";
				}
				catch (const std::exception& e) {
					// std::cout << e.what() << std::endl;
				}
			}

		}

		Json res{};
		size_t count{ 0 };
		for (const auto& [key, value] : singlePhi) {
			++count;
			/*
			std::cout << "(phi) -- [" << value.title << " / " << value.difficulty << " / " << value.level << "] Score: " << value.score
				<< ", Acc: " << value.acc << ", RankingSocre: " << value.rks
				<< ", Is FC: " << value.is_fc
				<< '\n';
			*/
			Json j{
				{"ranking", 0}, // 0 = "phi"
				{"acc", value.acc},
				{"rankingSocre", value.rks},
				{"score", value.score},
				{"difficulty", value.difficulty},
				{"title", value.title},
				{"isfc", value.is_fc},
				{"level", value.level},
				{"songid", value.sid}
			};

			if (authentication.authority == 5)
			{
				j["id"] = value.id;
				j["illustrationPath"] = value.song_illustration_path;
			}

			//res.emplace_back(std::move());
			data["content"]["best_list"]["phi"] = j;
			//std::cout << "\n=====================\n";
			if (count >= 1)break;
		}


		count = 0;
		for (const auto& [key, value] : rksSort) {
			++count;
			/*
			std::cout << "(" << count << ") -- [" << value.title << " / " << value.difficulty << " / " << value.level << "] Score: " << value.score
				<< ", Acc: " << value.acc << ", RankingSocre: " << value.rks
				<< ", Is FC: " << value.is_fc
				<< '\n';
			std::cout << "\n=====================\n";
			*/
			Json j{
				{"ranking", count},
				{"acc", value.acc},
				{"rankingSocre", value.rks},
				{"score", value.score},
				{"difficulty", value.difficulty},
				{"title", value.title},
				{"isfc", value.is_fc},
				{"level", value.level},
				{"songid", value.sid}
			};

			if (authentication.authority == 5)
			{
				j["id"] = value.id;
				j["illustrationPath"] = value.song_illustration_path;
			}

			res.emplace_back(std::move(j));
			//if (count >= 19)break;
		}
		data["content"]["best_list"]["best"] = std::move(res);
		data["content"]["best_list"]["is_phi"] = singlePhi.size() > 0;
		data["status"] = 0;
		return data;
	};

	Json getBest(const UserData& authentication, std::string_view sessionToken, const std::string& song_id, unsigned char difficulty) override {
		Json data;

		self::PhiTaptapAPI phiAPI(sessionToken);

		auto records{ phiAPI.getPlayerRecord() };

		self::SongScore record{};
		if (difficulty == 4) {
			for (uint8_t i{ difficulty }; i > 0; --i)
			{
				--difficulty;
				if (records.at(song_id).count(difficulty)){
					break;
				}
			}
		}
		record = records.at(song_id).at(difficulty);

		float
			acc{ record.acc },
			rks{ 0.0f },
			rate{ Global::PhigrosSongInfo[song_id].rating[difficulty]};
		if (acc >= 70) rks = (std::pow((acc - 55) / 45, 2)) * rate;

		const std::string levels[]{ "EZ", "HD", "IN", "AT", "Legacy" };

		data["content"]["record"]["songid"] = song_id;
		data["content"]["record"]["difficulty"] = levels[difficulty] /* record.difficulty */ ;
		data["content"]["record"]["acc"] = acc;
		data["content"]["record"]["score"] = record.score;
		data["content"]["record"]["isfc"] = record.is_fc;
		data["content"]["record"]["title"] = Global::PhigrosSongInfo[song_id].title;
		data["content"]["record"]["artist"] = Global::PhigrosSongInfo[song_id].artist;
		data["content"]["record"]["rating"] = rate;
		data["content"]["record"]["rks"] = rks;


		if (authentication.authority == 5)
		{
			data["content"]["record"]["id"] = Global::PhigrosSongInfo[song_id].id;
			data["content"]["record"]["illustrationPath"] = Global::PhigrosSongInfo[song_id].song_illustration_path;
		}

		auto playerSummary{ phiAPI.GetSummary() };
		auto playerData{ phiAPI.getUserData() };

		// 玩家记录
		player_records(sessionToken, playerSummary);

		data["content"]["playerNickname"] = phiAPI.getNickname();
		data["content"]["challengeModeRank"] = playerSummary.ChallengeModeRank;
		data["content"]["rankingScore"] = playerSummary.RankingScore;
		data["content"]["updateTime"] = playerSummary.updatedAt;
		data["content"]["timestamp"] = playerSummary.timestamp;
		{
			data["content"]["other"]["records"]["AT"]["clear"] = playerSummary.AT[0];
			data["content"]["other"]["records"]["AT"]["fc"] = playerSummary.AT[1];
			data["content"]["other"]["records"]["AT"]["phi"] = playerSummary.AT[2];
			data["content"]["other"]["records"]["IN"]["clear"] = playerSummary.IN[0];
			data["content"]["other"]["records"]["IN"]["fc"] = playerSummary.IN[1];
			data["content"]["other"]["records"]["IN"]["phi"] = playerSummary.IN[2];
			data["content"]["other"]["records"]["HD"]["clear"] = playerSummary.HD[0];
			data["content"]["other"]["records"]["HD"]["fc"] = playerSummary.HD[1];
			data["content"]["other"]["records"]["HD"]["phi"] = playerSummary.HD[2];
			data["content"]["other"]["records"]["EZ"]["clear"] = playerSummary.EZ[0];
			data["content"]["other"]["records"]["EZ"]["fc"] = playerSummary.EZ[1];
			data["content"]["other"]["records"]["EZ"]["phi"] = playerSummary.EZ[2];

			std::string avatar_id{ playerData.avatar };
			if (Global::PhigrosPlayerAvatar.count(avatar_id)) {
				if (authentication.authority == 5) {
					data["content"]["other"]["avatarPath"] = Global::PhigrosPlayerAvatar.at(avatar_id).avatar_path;
					data["content"]["other"]["avatarDbId"] = Global::PhigrosPlayerAvatar.at(avatar_id).sid;
				}
				data["content"]["other"]["avatarHasEnable"] = true;
			}
			else data["content"]["other"]["avatarHasEnable"] = false;

			data["content"]["other"]["avatar"] = avatar_id;
			data["content"]["other"]["background"] = playerData.background;
			data["content"]["other"]["profile"] = playerData.profile;
		}

		data["status"] = 0;
		return data;
	}

	Json getRecords(const UserData& authentication, std::string_view sessionToken) override {
		Json data;

		if (!self::CheckParameterStr(sessionToken)) {
			throw self::HTTPException("SQL injection may exist", 403, 8);
		}

		std::string st{ sessionToken.data() };
		bool is_exists{ false };
		SQL_Util::PlayerRdDB <<
			"SELECT EXISTS(SELECT 1 FROM sqlite_master WHERE type='table' AND name=?);"
			<< st >> is_exists;

		if (!is_exists) {
			throw self::HTTPException("Record doesn't exist", 404, 9);
		}

		SQL_Util::PlayerRdDB << "select sid,rks,challengeModeRank,timestamp,nickname from \"" + st +  "\";"
			>> [&](uint32_t sid, double rks, uint16_t challengeModeRank, std::time_t timestamp, std::string nickname) {

			data["content"].emplace_back(
				Json{
				  {"sid", sid},
				  {"rks", rks},
				  {"challengeModeRank", challengeModeRank},
				  {"timestamp", timestamp},
				  {"nickname", nickname},
				});
		};

		data["status"] = 0;
		return data;
	};

	Json getAlias(const UserData& authentication, int32_t id) override {
		Json result;
		SQL_Util::PhiDB << "select id, alias from alias where song_id = ?"
			<< id
			>> [&](int32_t id, std::string alias) {
			Json data{
				{"id", id},
				{"alias", alias},
			};
			result["content"].emplace_back(data);
		};

		if (result.is_null()) throw self::HTTPException("", 404, 11);
		
		result["songId"] = id;

		return result;
	};
	
	Json documentSongidByAlias(const UserData& authentication, std::string alias) override {
		Json result;
		int32_t song_id;
		SQL_Util::PhiDB << "select song_id from alias where alias = ?"
			<< alias
			>> song_id;

		result["songId"] = song_id;

		if (result.is_null()) throw self::HTTPException("", 404, 11);

		return result;
	};

	Json matchAlias(const defined::PhiMatchAlias& matchAlias) override {
		Json body{
			{"q", matchAlias.query},
			{"limit", matchAlias.limit},
			{"offset", matchAlias.offset},
			{"hitsPerPage", matchAlias.hitsPerPage},
			{"page", matchAlias.page},
			{"showRankingScore", matchAlias.showRankingScore},
		}, resp, result;


		web::http::client::http_client client(U(Global::Meilisearch::Url));
		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::POST);
		request_add_index.set_request_uri("/indexes/"s + Global::Meilisearch::Phi::SearchNamespace + "/search"s);
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + Global::Meilisearch::Authorization);
		request_add_index.set_body(web::json::value::parse(body.dump()));

		auto response = client.request(request_add_index).get();

		if (response.status_code() < 400 and response.status_code() >= 200) {
			resp = Json::parse(response.extract_json().get().serialize());
		} else {
			auto error = response.extract_string().get();
			throw self::HTTPException(error, 500, 12);
		}

		// 遍历索引
		for (auto& index : resp["hits"]) {
			Json data;

			if (matchAlias.showRankingScore) data["rankingScore"] = index.at("_rankingScore").get<float>();

			data["id"] = index.at("id").get<int32_t>();

			if (index["sid"].is_null()) data["sid"] = nullptr;
			else data["sid"] = index.at("sid").get<std::string>();
			
			data["title"] = index.at("title").get<std::string>();
			data["aliases"] = index.at("aliases");
			result.emplace_back(data);
		}
		if (result.is_null()) result = Json::parse("[]");
		return result.dump();
	}

	Json getSongInfo(const UserData& authentication, const defined::PhiInfoParamStruct& infoParam) override {
		std::string front_sql{ " \
select sid,id, title, song_illustration_path, song_audio_path, \
rating_ez, rating_hd, rating_in, rating_at, rating_lg, rating_sp, \
note_ez, note_hd, note_in, note_at, note_lg, note_sp,\
design_ez, design_hd, design_in, design_at, design_lg, design_sp, \
artist, illustration, duration, bpm, chapter \
from phigros where " };

		switch (infoParam.mode) {
		case 0:
			if (!self::CheckParameterStr(infoParam.sid)) throw self::HTTPException("SQL injection may exist", 403, 8);
			front_sql += "sid = \""s + infoParam.sid + "\";"s;
			break;
		case 1:
			front_sql += "id = "s + std::to_string(infoParam.id) + ";"s;
			break;
		default:
			throw self::HTTPException("", 400, 10);
			break;
		}
		Json result;
		SQL_Util::PhiDB << front_sql
			>> [&](
				std::unique_ptr<std::string> sid_p, int32_t id, std::string title,
				std::string song_illustration_path, std::string song_audio_path,
				std::unique_ptr<float> rating_ez, std::unique_ptr<float> rating_hd, std::unique_ptr<float> rating_in, std::unique_ptr<float> rating_at, std::unique_ptr<float> rating_lg, std::unique_ptr<float> rating_sp,
				std::unique_ptr<uint16_t> note_ez, std::unique_ptr<uint16_t> note_hd, std::unique_ptr<uint16_t> note_in, std::unique_ptr<uint16_t> note_at, std::unique_ptr<uint16_t> note_lg, std::unique_ptr<uint16_t> note_sp,
				std::unique_ptr<std::string> design_ez, std::unique_ptr<std::string> design_hd, std::unique_ptr<std::string> design_in, std::unique_ptr<std::string> design_at, std::unique_ptr<std::string> design_lg, std::unique_ptr<std::string> design_sp,
				std::string artist, std::string illustration, std::string duration, std::string bpm, std::string chapter
				) {
					result["sid"] = sid_p ? *sid_p : nullptr;
					result["id"] = id;
					result["title"] = title;

					if (authentication.authority == 5)
					{
						result["illustrationPath"] = song_illustration_path;
						result["audioPath"] = song_audio_path;
					};

					std::array<std::string, 6> levels = { "ez", "hd", "in", "at", "lg", "sp" };

					for (const auto& level : levels) {
						result["content"][level]["rating"] = nullptr;
						result["content"][level]["note"] = nullptr;
						result["content"][level]["design"] = nullptr;
						result["flag"][level] = false;

						struct {
							std::unique_ptr<float> rating;
							std::unique_ptr<uint16_t> note;
							std::unique_ptr<std::string> design;
						} rating = { nullptr, nullptr, nullptr };

						if (level == "ez")		rating = { std::move(rating_ez), std::move(note_ez), std::move(design_ez) };
						else if (level == "hd") rating = { std::move(rating_hd), std::move(note_hd), std::move(design_hd) };
						else if (level == "in") rating = { std::move(rating_in), std::move(note_in), std::move(design_in) };
						else if (level == "at") rating = { std::move(rating_at), std::move(note_at), std::move(design_at) };
						else if (level == "lg") rating = { std::move(rating_lg), std::move(note_lg), std::move(design_lg) };
						else if (level == "sp") rating = { std::move(rating_sp), std::move(note_sp), std::move(design_sp) };

						if (rating.rating) result["content"][level]["rating"] = *rating.rating;
						if (rating.note) result["content"][level]["note"] = *rating.note;
						if (rating.design) result["content"][level]["design"] = *rating.design;
						if (rating.rating and rating.note and rating.design) result["flag"][level] = true;
					}

					result["artist"] = artist;
					result["illustration"] = illustration;
					result["duration"] = duration;
					result["bpm"] = bpm;
					result["chapter"] = chapter;
		};

		if (result.is_null()) throw self::HTTPException("", 404, 11);

		return result;
	}

	std::string addAlias(const UserData& authentication, const defined::PhiAliasAddParam& add) override {
		bool is_existe{ false };
		if (authentication.authority < 3) throw self::HTTPException("", 401, 6);
		SQL_Util::PhiDB << "select count(id) from phigros where id = ?;" << add.related_song_id >> is_existe;

		if(!is_existe) throw self::HTTPException("song id doesn't exist", 404, 3);
		SQL_Util::PhiDB << "insert into alias (alias,song_id) values (?,?);"
			<< add.alias
			<< add.related_song_id;

		if (Global::Meilisearch::IsOpen) {
			this->updateSearch(add.related_song_id);
		}

		return "ok";
	}

	std::string delAlias(const UserData& authentication, const defined::PhiAliasAddParam& add) override {
		bool is_existe{ false };
		uint32_t songId{};
		if (authentication.authority < 4) throw self::HTTPException("", 401, 6);
		SQL_Util::PhiDB << "select count(id),song_id from alias where id = ?;" << add.sid >> [&]
		(bool count_id, int32_t song_id) {
			is_existe = count_id;
			songId = song_id;
		};

		if(!is_existe) throw self::HTTPException("ID doesn't exist", 404, 3);
		SQL_Util::PhiDB << "delete from alias where id = ?;" << add.sid;

		if (Global::Meilisearch::IsOpen) {
			this->updateSearch(songId);
		}

		return "ok";
	}

	std::string asyncMatch(void) override {
		Json body;
		SQL_Util::PhiDB << "select sid,id,title from phigros" >> [&]
		(std::unique_ptr<std::string> sid_p, int32_t id, std::string title) {
			Json song_info;

			if (sid_p)song_info["sid"] = *sid_p;
			else song_info["sid"] = nullptr;

			song_info["id"] = id;
			song_info["title"] = title;
			SQL_Util::PhiDB << "select id,alias from alias where song_id = ?" << id >> [&]
			(int32_t id, std::string alias) {
				Json alias_data;
				song_info["aliases"].emplace_back(Json{
					{"aliasId", id},
					{"alias", alias},
					});
			};
			body.emplace_back(song_info);
		};

		web::http::client::http_client client(U(Global::Meilisearch::Url));
		// 创建第一个HTTP请求, 添加匹配索引
		web::http::http_request request_add_index(web::http::methods::POST);
		request_add_index.set_request_uri("/indexes/"s + Global::Meilisearch::Phi::SearchNamespace + "/documents?primaryKey=id"s);
		request_add_index.headers().add("Content-Type", "application/json");
		request_add_index.headers().add("Authorization", "Bearer "s + Global::Meilisearch::Authorization);
		request_add_index.set_body(web::json::value::parse(body.dump()));

		// 创建第二个HTTP请求设置只对特定数据索引
		web::http::http_request request_searchable_attributes(web::http::methods::PUT);
		request_searchable_attributes.set_request_uri("/indexes/"s + Global::Meilisearch::Phi::SearchNamespace + "/settings/searchable-attributes"s);
		request_searchable_attributes.headers().add("Authorization", "Bearer "s + Global::Meilisearch::Authorization);
		request_searchable_attributes.set_body(web::json::value::parse("[\"aliases.alias\",\"title\"]"));


		// 发送多个HTTP请求并获取响应(异步操作)
		std::vector<pplx::task<web::http::http_response>> tasks;
		tasks.push_back(client.request(request_add_index));
		tasks.push_back(client.request(request_searchable_attributes));

		pplx::when_all(tasks.begin(), tasks.end()).then([](std::vector<web::http::http_response> responses) {
			// 处理响应
			for (const auto& resp : responses) {
				if (resp.status_code() >= 300 or resp.status_code() < 200) {
					auto error = resp.extract_string().get();
					throw self::HTTPException(error, 500, 12);
				}
			}
		}).wait();

		return "ok";
	}
private:
};


#endif // !PHIGROS_SERVICE_HPP_IMPL
