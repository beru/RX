#pragma once
//=====================================================================//
/*! @file
    @brief  ファイル書き込みクラス@n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include <cstdio>
#include <cstring>
#include "common/fifo.hpp"
// #include "main.hpp"

namespace seeda {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  write_file class
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	class write_file {

		uint32_t	limit_;
		uint32_t	count_;

		char		path_[128];

		bool		enable_;
		bool		state_;

		static const uint32_t BUF_SIZE = 64;

		typedef utils::fifo<uint16_t, BUF_SIZE, sample_t> FIFO;
		FIFO	fifo_;

		FILE	*fp_;

		time_t		time_ref_;

		uint32_t	unit_;

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief  コンストラクター
		*/
		//-----------------------------------------------------------------//
		write_file() : limit_(60), count_(0), path_{ "00000" },
			enable_(false), state_(false),
			fp_(nullptr), time_ref_(0), unit_(0) { }


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込み許可
		*/
		//-----------------------------------------------------------------//
		void enable(bool ena = true) {
			enable_ = ena;
			count_ = 0;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込み許可取得
			@return 書き込み許可
		*/
		//-----------------------------------------------------------------//
		bool get_enable() const { return enable_; }


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込みパス設定
			@param[in]	path	ファイルパス
		*/
		//-----------------------------------------------------------------//
		void set_path(const char* path)
		{
			utils::format("%s", path_, sizeof(path_)) % path;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込みパス取得
			@param[in]	path	ファイルパス
		*/
		//-----------------------------------------------------------------//
		const char* get_path() const { return path_; }


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込み回数の設定
			@param[in]	limit	書き込み回数
		*/
		//-----------------------------------------------------------------//
		void set_limit(uint32_t limit)
		{
			limit_ = limit;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  書き込み時間設定
			@return	書き込み時間（秒）
		*/
		//-----------------------------------------------------------------//
		uint32_t get_limit() const { return limit_; }


		//-----------------------------------------------------------------//
		/*!
			@brief  経過時間の取得
			@return 経過時間
		*/
		//-----------------------------------------------------------------//
		uint32_t get_resume() const { return count_; }


		//-----------------------------------------------------------------//
		/*!
			@brief  インジェクション
			@param[in]	data	データ
			@return 成功なら「true」
		*/
		//-----------------------------------------------------------------//
		bool injection(const sample_t& data)
		{
			if(fifo_.length() >= (BUF_SIZE -2)) { // スペースが無い場合
				return false;
			}
			fifo_.put(data);
			return true;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief  サービス
		*/
		//-----------------------------------------------------------------//
		void service()
		{
			bool back = state_;
			state_ = enable_;
			if(!enable_) {
				if(back && fp_ != nullptr) {
					utils::format("Write file aborted\n");
					fclose(fp_);
					fp_ = nullptr;
				}
				return;
			}

			time_t t = get_time();
			{
				if(time_ref_ != t) {
					time_ref_ = t;
					for(int i = 0; i < 8; ++i) {
						at_sample(i).time_ = t;
						at_sample(i).ch_ = i;
						fifo_.put(get_sample(i));
					}
//					utils::format("Write data injection...\n");

					if(fp_ == nullptr) {
						struct tm *m = localtime(&time_ref_);
						char file[256];
						utils::format("%s_%04d%02d%02d%02d%02d.csv", file, sizeof(file))
							% path_
							% static_cast<uint32_t>(m->tm_year + 1900)
							% static_cast<uint32_t>(m->tm_mon + 1)
							% static_cast<uint32_t>(m->tm_mday)
							% static_cast<uint32_t>(m->tm_hour)
							% static_cast<uint32_t>(m->tm_min);
						fp_ = fopen(file, "wb");
						unit_ = 0;
						if(fp_ == nullptr) {  // error then disable write.
							utils::format("File open error: '%s'\n") % file;
							enable_ = false;
						} else {
							utils::format("Start write file: '%s'\n") % file;
						}
						char data[1024];
						uint32_t l = 0;
						l += (utils::format("DATE,TIME", &data[l], sizeof(data) - l)).get_length();
						for(int i = 0; i < 8; ++i) {
							l += (utils::format(",CH,MAX,MIN,AVE,MEDIAN,COUNTUP", &data[l], sizeof(data) - l)).get_length();
						}
						data[l] = '\n';
						++l;
						fwrite(data, 1, l, fp_);
						return;
					}
				}
			}

			if(fifo_.length() == 0) return;

			sample_t smp = fifo_.get();

			char data[2048];
			uint32_t l = 0;
			struct tm *m = localtime(&smp.time_);
			l += (utils::format("%04d/%02d/%02d,%02d:%02d:%02d,", &data[l], sizeof(data) - l)
				% static_cast<uint32_t>(m->tm_year + 1900)
				% static_cast<uint32_t>(m->tm_mon + 1)
				% static_cast<uint32_t>(m->tm_mday)
				% static_cast<uint32_t>(m->tm_hour)
				% static_cast<uint32_t>(m->tm_min)
				% static_cast<uint32_t>(m->tm_sec)).get_length();
			l += smp.make_csv2(&data[l], sizeof(data) - l);
			if(smp.ch_ < 7) {
				data[l] = ',';
				++l;
			}
			l += smp.make_csv2(&data[l], sizeof(data) - l);
			if(smp.ch_ == 7) {
				data[l] = '\n';
				++l;
			}
			fwrite(data, 1, l, fp_);
//			utils::format("Write ch: %d\n") % t.ch_;

			if(smp.ch_ == 7) {  // last channnel
				++count_;
				++unit_;
				if(unit_ >= 60) {  // change file.
					fclose(fp_);
					fp_ = nullptr;
				}

				if(count_ >= limit_) {
					fclose(fp_);
					fp_ = nullptr;
					utils::format("Close write file: %d sec\n") % count_;
					enable_ = false;
				}
			}
		}
	};
}
