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
			data["content"]["other"]["avatar"] = playerData.avatar;
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

			res.emplace_back(std::move(j));
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
		data["content"]["best_list"]["phi"] = singlePhi.size() > 0;
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
			data["content"]["other"]["avatar"] = playerData.avatar;
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
private:
};


#endif // !PHIGROS_SERVICE_HPP_IMPL
