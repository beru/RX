#pragma once
//=====================================================================//
/*!	@file
	@brief	Audio タグ・クラス
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/RX/blob/master/LICENSE
*/
//=====================================================================//
#include "common/fixed_string.hpp"

namespace sound {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief	タグ情報
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	struct tag_t {
		typedef utils::fixed_string<128> STR128;
		typedef utils::fixed_string<16> STR16;
		typedef utils::fixed_string<8> STR8;

	private:
		STR128	album_;		///< アルバム名
		STR128	title_;		///< タイトル（曲名）
		STR128	artist_;	///< アーティスト
		STR16	year_;		///< リリース年
		STR8	disc_;		///< ディスク
		STR8	track_;		///< トラック

	public:
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		/*!
			@brief	画像情報
		*/
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
		struct apic_t {
			uint8_t		typ_;
			char		ext_[4];
			uint32_t	ofs_;
			uint32_t	len_;
			apic_t() : typ_(0), ext_{ 0 }, ofs_(0), len_(0) { }
		};
	private:
		apic_t	apic_;

	public:
		//-------------------------------------------------------------//
		/*!
			@brief	コンストラクター
		*/
		//-------------------------------------------------------------//
		tag_t() noexcept : apic_() { clear(); }


		//-------------------------------------------------------------//
		/*!
			@brief	クリア
		*/
		//-------------------------------------------------------------//
		void clear() noexcept
		{
			album_.clear();
			title_.clear();
			artist_.clear();
			year_.clear();
			disc_.clear();
			track_.clear();
			apic_ = apic_t();
		}


		//-------------------------------------------------------------//
		/*!
			@brief	アルバムへの参照
			@return アルバム
		*/
		//-------------------------------------------------------------//
		STR128& at_album() noexcept { return album_; }


		//-------------------------------------------------------------//
		/*!
			@brief	アルバムへの参照 (RO)
			@return アルバム
		*/
		//-------------------------------------------------------------//
		const STR128& get_album() const noexcept { return album_; }


		//-------------------------------------------------------------//
		/*!
			@brief	タイトルへの参照
			@return タイトル
		*/
		//-------------------------------------------------------------//
		STR128& at_title() noexcept { return title_; }


		//-------------------------------------------------------------//
		/*!
			@brief	タイトルへの参照 (RO)
			@return タイトル
		*/
		//-------------------------------------------------------------//
		const STR128& get_title() const noexcept { return title_; }


		//-------------------------------------------------------------//
		/*!
			@brief	アーティストへの参照
			@return アーティスト
		*/
		//-------------------------------------------------------------//
		STR128& at_artist() noexcept { return artist_; }


		//-------------------------------------------------------------//
		/*!
			@brief	アーティストへの参照 (RO)
			@return アーティスト
		*/
		//-------------------------------------------------------------//
		const STR128& get_artist() const noexcept { return artist_; }


		//-------------------------------------------------------------//
		/*!
			@brief	年への参照
			@return 年
		*/
		//-------------------------------------------------------------//
		STR16& at_year() noexcept { return year_; }


		//-------------------------------------------------------------//
		/*!
			@brief	年への参照 (RO)
			@return 年
		*/
		//-------------------------------------------------------------//
		const STR16& get_year() const noexcept { return year_; }


		//-------------------------------------------------------------//
		/*!
			@brief	ディスクへの参照
			@return ディスク
		*/
		//-------------------------------------------------------------//
		STR8& at_disc() noexcept { return disc_; }


		//-------------------------------------------------------------//
		/*!
			@brief	ディスクへの参照 (RO)
			@return ディスク
		*/
		//-------------------------------------------------------------//
		const STR8& get_disc() const noexcept { return disc_; }


		//-------------------------------------------------------------//
		/*!
			@brief	トラックへの参照
			@return トラック
		*/
		//-------------------------------------------------------------//
		STR8& at_track() noexcept { return track_; }


		//-------------------------------------------------------------//
		/*!
			@brief	トラックへの参照 (RO)
			@return トラック
		*/
		//-------------------------------------------------------------//
		const STR8& get_track() const noexcept { return track_; }


		//-------------------------------------------------------------//
		/*!
			@brief	アルバム画像情報への参照
			@return アルバム画像情報
		*/
		//-------------------------------------------------------------//
		apic_t& at_apic() noexcept { return apic_; }


		//-------------------------------------------------------------//
		/*!
			@brief	アルバム画像情報への参照 (RO)
			@return アルバム画像情報
		*/
		//-------------------------------------------------------------//
		const apic_t& get_apic() const noexcept { return apic_; }
	};
}
