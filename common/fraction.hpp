#pragma once
//=====================================================================//
/*!	@file
	@brief	分数 クラス
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2018 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/glfw_app/blob/master/LICENSE
*/
//=====================================================================//
#include <cstdint>

namespace imath {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief	分数 クラス
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	template <typename T>
	class fraction {

		T		up_;	// 分子 
		T		bo_;	// 分母
		T		val_;

	public:
		//-----------------------------------------------------------------//
		/*!
			@brief	コンストラクタ
		 */
		//-----------------------------------------------------------------//
		fraction() noexcept : up_(0), bo_(1), val_(0) { }


		//-----------------------------------------------------------------//
		/*!
			@brief	分数を設定
			@param[in]	up	分子
			@param[in]	bo	分母
			@param[in]	st	初期値が「０」以外の場合設定
		 */
		//-----------------------------------------------------------------//
		void set(T up, T bo, T st = 0) noexcept
		{
			up_ = up;
			bo_ = bo;
			val_ = st;
		}


		//-----------------------------------------------------------------//
		/*!
			@brief	加算
			@return 桁溢れなら「true」
		 */
		//-----------------------------------------------------------------//
		bool add() noexcept
		{
			val_ += up_;
			if(bo_ <= val_) {
				val_ -= bo_;
				return true;
			}
			return false;
		}
	};
}