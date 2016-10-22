#pragma once
//=====================================================================//
/*!	@file
	@brief	RX24T グループ・ペリフェラル @n
			Copyright 2016 Kunihito Hiramatsu
	@author	平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include <cstdint>

namespace device {

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	/*!
		@brief  ペリフェラル種別
	*/
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
	enum class peripheral {
		CMT0,
		CMT1,
		CMT2,
		CMT3,

		RIIC0,

		SCI1,
		SCI5,
		SCI6,

		RSPI0,
	};
}
