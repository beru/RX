//=====================================================================//
/*! @file
    @brief  GR-KAEDE(RX64M) サンプル @n
			Copyright 2017 Kunihito Hiramatsu
    @author 平松邦仁 (hira@rvf-rc45.net)
*/
//=====================================================================//
#include "common/renesas.hpp"

#include "common/cmt_io.hpp"
#include "common/fifo.hpp"
#include "common/sci_io.hpp"
#include "common/format.hpp"
#include "common/input.hpp"
#include "common/command.hpp"
#include "common/rspi_io.hpp"
#include "common/spi_io.hpp"
#include "common/sdc_io.hpp"

#include "chip/LTC2348_16.hpp"

/// #include "seeda.hpp"

#include <string>

#define SERVER_TASK

#ifdef SERVER_TASK
#include <cstdlib>
#include "GR/core/Ethernet.h"
#endif

namespace {

	const int seeda_version_ = 15;

	volatile unsigned long millis_ = 0;
	volatile unsigned long delay_ = 0;
	volatile uint32_t millis10x_ = 0;
	volatile uint32_t cmtdiv_ = 0;

	class cmt_task {
	public:
		void operator() () {
			++millis_;
			++cmtdiv_;
			if(cmtdiv_ >= 10) {
				cmtdiv_ = 0;
				++millis10x_;
			}
			if(delay_) {
				--delay_;
			}
		}
	};

	static void sync_100hz()
	{
		volatile uint32_t tmp = millis10x_;
		while(tmp == millis10x_) ;
	}

	device::cmt_io<device::CMT0, cmt_task>  cmt_;

//	device::cmt_io<device::CMT0, cmt_task>  cmt_;

	typedef utils::fifo<uint8_t, 1024> buffer;
	device::sci_io<device::SCI5, buffer, buffer> sci_;

	utils::rtc_io rtc_;

	typedef device::PORT<device::PORT6, device::bitpos::B7> SW1;
	typedef device::PORT<device::PORT6, device::bitpos::B6> SW2;

	typedef device::PORT<device::PORT7, device::bitpos::B0> LAN_RESN;
	typedef device::PORT<device::PORT7, device::bitpos::B3> LAN_PDN;

	// Soft SDC 用　SPI 定義（SPI）
	typedef device::PORT<device::PORTD, device::bitpos::B6> MISO;
	typedef device::PORT<device::PORTD, device::bitpos::B4> MOSI;
	typedef device::PORT<device::PORTD, device::bitpos::B5> SPCK;
	typedef device::spi_io<MISO, MOSI, SPCK> SPI;
	SPI spi_;

	typedef device::PORT<device::PORTD, device::bitpos::B3> sdc_select;	///< カード選択信号
	typedef device::NULL_PORT  sdc_power;	///< カード電源制御（常に電源ＯＮ）
	typedef device::PORT<device::PORTE, device::bitpos::B6> sdc_detect;	///< カード検出

	typedef utils::sdc_io<SPI, sdc_select, sdc_power, sdc_detect> SDC;
	SDC sdc_(spi_, 10000000);

	// LTC A/D 定義
	typedef device::PORT<device::PORT4, device::bitpos::B0> LTC_CSN;   // P40(141)
	typedef device::PORT<device::PORTC, device::bitpos::B6> LTC_CNV;   // PC6(61)
	typedef device::PORT<device::PORTD, device::bitpos::B0> LTC_BUSY;  // PD0/IRQ0(126)
	typedef device::PORT<device::PORT5, device::bitpos::B3> LTC_PD;    // P53(53)
	typedef device::PORT<device::PORT5, device::bitpos::B6> LTC_SDI;   // P56(50)
	typedef device::PORT<device::PORT8, device::bitpos::B6> LTC_SCKO;  // P86(41)
	typedef device::PORT<device::PORT8, device::bitpos::B7> LTC_SCKI;  // P87(39)
	typedef device::PORT<device::PORT2, device::bitpos::B0> LTC_SDO0;  // P20(37)
	typedef device::PORT<device::PORT2, device::bitpos::B1> LTC_SDO2;  // P21(36)
	typedef device::PORT<device::PORT2, device::bitpos::B2> LTC_SDO4;  // P22(35)
	typedef device::PORT<device::PORT2, device::bitpos::B3> LTC_SDO6;  // P23(34)
	typedef struct chip::LTC2348_SDO_t<LTC_SCKO, LTC_SDO0, LTC_SDO2, LTC_SDO4, LTC_SDO6> LTC_SDO;
	typedef chip::LTC2348_16<LTC_CSN, LTC_CNV, LTC_BUSY, LTC_PD, LTC_SDI, LTC_SCKI, LTC_SDO> EADC;
	EADC eadc_;

	utils::command<256> cmd_;

#ifdef SERVER_TASK
	EthernetServer server_(80);
#endif

	// TelnetClient
//	EthernetServer telnet_(23);
}

extern "C" {

	void INT_Excep_ICU_GROUPAL1(void);
	void INT_Excep_CMT1_CMI1(void);

	void sci_putch(char ch)
	{
		sci_.putch(ch);
	}

	void sci_puts(const char* s)
	{
		sci_.puts(s);
	}

	char sci_getch(void)
	{
		return sci_.getch();
	}

	uint16_t sci_length(void)
	{
		return sci_.recv_length();
	}

	time_t get_time_()
	{
		time_t t = 0;
		if(!rtc_.get_time(t)) {
			utils::format("Stall RTC read\n");
		}
		return t;
	}

	DSTATUS disk_initialize(BYTE drv) {
		return sdc_.at_mmc().disk_initialize(drv);
	}

	DSTATUS disk_status(BYTE drv) {
		return sdc_.at_mmc().disk_status(drv);
	}

	DRESULT disk_read(BYTE drv, BYTE* buff, DWORD sector, UINT count) {
		return sdc_.at_mmc().disk_read(drv, buff, sector, count);
	}

	DRESULT disk_write(BYTE drv, const BYTE* buff, DWORD sector, UINT count) {
		return sdc_.at_mmc().disk_write(drv, buff, sector, count);
	}

	DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void* buff) {
		return sdc_.at_mmc().disk_ioctl(drv, ctrl, buff);
	}

	DWORD get_fattime(void) {
		time_t t = 0;
		rtc_.get_time(t);
		return utils::str::get_fattime(t);
	}

	void utf8_to_sjis(const char* src, char* dst) {
		utils::str::utf8_to_sjis(src, dst);
	}

	unsigned long millis(void)
	{
		return millis_;
	}

	void delay(unsigned long ms)
	{
		delay_ = ms;
		while(delay_ != 0) ;		
	}
}

namespace {

	uint8_t get_switch_()
	{
		return static_cast<uint8_t>(!SW1::P()) | (static_cast<uint8_t>(!SW2::P()) << 1);
	}

	void disp_time_(time_t t, char* dst = nullptr, uint32_t size = 0)
	{
		struct tm *m = localtime(&t);
		utils::format("%s %s %d %02d:%02d:%02d  %4d\n", dst, size)
			% get_wday(m->tm_wday)
			% get_mon(m->tm_mon)
			% static_cast<uint32_t>(m->tm_mday)
			% static_cast<uint32_t>(m->tm_hour)
			% static_cast<uint32_t>(m->tm_min)
			% static_cast<uint32_t>(m->tm_sec)
			% static_cast<uint32_t>(m->tm_year + 1900);
	}

	const char* get_dec_(const char* p, char tmch, int& value) {
		int v = 0;
		char ch;
		while((ch = *p) != 0) {
			++p;
			if(ch == tmch) {
				break;
			} else if(ch >= '0' && ch <= '9') {
				v *= 10;
				v += ch - '0';
			} else {
				return nullptr;
			}
		}
		value = v;
		return p;
	}

	void set_time_date_()
	{
		time_t t = get_time_();
		if(t == 0) return;

		struct tm *m = localtime(&t);
		bool err = false;
		if(cmd_.get_words() == 3) {
			char buff[12];
			if(cmd_.get_word(1, sizeof(buff), buff)) {
				const char* p = buff;
				int vs[3];
				uint8_t i;
				for(i = 0; i < 3; ++i) {
					p = get_dec_(p, '/', vs[i]);
					if(p == nullptr) {
						break;
					}
				}
				if(p != nullptr && p[0] == 0 && i == 3) {
					if(vs[0] >= 1900 && vs[0] < 2100) m->tm_year = vs[0] - 1900;
					if(vs[1] >= 1 && vs[1] <= 12) m->tm_mon = vs[1] - 1;
					if(vs[2] >= 1 && vs[2] <= 31) m->tm_mday = vs[2];		
				} else {
					err = true;
				}
			}

			if(cmd_.get_word(2, sizeof(buff), buff)) {
				const char* p = buff;
				int vs[3];
				uint8_t i;
				for(i = 0; i < 3; ++i) {
					p = get_dec_(p, ':', vs[i]);
					if(p == nullptr) {
						break;
					}
				}
				if(p != nullptr && p[0] == 0 && (i == 2 || i == 3)) {
					if(vs[0] >= 0 && vs[0] < 24) m->tm_hour = vs[0];
					if(vs[1] >= 0 && vs[1] < 60) m->tm_min = vs[1];
					if(i == 3 && vs[2] >= 0 && vs[2] < 60) m->tm_sec = vs[2];
					else m->tm_sec = 0;
				} else {
					err = true;
				}
			}
		}

		if(err) {
			sci_puts("Can't analize Time/Date input.\n");
			return;
		}

		time_t tt = mktime(m);
		if(!rtc_.set_time(tt)) {
			sci_puts("Stall RTC write...\n");
		}
	}


	bool check_mount_() {
		auto f = sdc_.get_mount();
		if(!f) {
			utils::format("SD card not mount.\n");
		}
		return f;
	}


	bool reset_signal_(uint8_t cmdn)
	{
		bool f = false;
		if(cmdn == 1) {
			bool v = LAN_RESN::P();
			utils::format("LAN-RESN: %d\n") % static_cast<int>(v);
			return true;
		} else if(cmdn > 1) {
			char tmp[16];
			if(cmd_.get_word(1, sizeof(tmp), tmp)) {
				// Reset signal
				if(strcmp(tmp, "0") == 0) {
					device::PORT7::PODR.B0 = 0;
					f = true;
				} else if(strcmp(tmp, "1") == 0) {
					device::PORT7::PODR.B0 = 1;
					f = true;
				} else {
					utils::format("reset param error: '%s'\n") % tmp;
				}
			}
		}
		return f;
	}


	bool eadc_conv_(uint8_t cmdn)
	{
		bool f = false;
		if(cmdn == 1) {
			if(eadc_.convert()) {
				for(int i = 0; i < 8; ++i) {
					uint32_t v = eadc_.get_data(i);
					float gain = 5.12f;
					float fv = static_cast<float>(v >> 8) / 65535.0f * gain;
					utils::format("LTC2348-16(%d): CHID: %d, SPAN: %03b, %6.3f [V] (%d)\n")
						% i % ((v >> 3) & 7) % (v & 7) % fv % (v >> 8);
				}
			} else {
				utils::format("LTC2348-16 convert error\n");
			}
			return true;
		} else if(cmdn > 1) {
			char tmp[16];
			if(cmd_.get_word(1, sizeof(tmp), tmp)) {
				if(tmp[0] >= '0' && tmp[0] <= '7' && tmp[1] == 0) {
					if(eadc_.convert()) {
						int ch = tmp[0] - '0';
						uint32_t v = eadc_.get_value(ch);
						float gain = 5.12f;
						float fv = static_cast<float>(v >> 8) / 65535.0f * gain;
						utils::format("LTC2348-16(%d): %6.3f [V] (%d)\n") % ch % fv % v;
					} else {
						utils::format("LTC2348-16 convert error\n");
					}
				} else {
					utils::format("convert param error: '%s'\n") % tmp;
				}
				f = true;
			}
		}
		return f;
	}


	bool eadc_sample_(uint8_t cmdn)
	{
		if(cmdn < 8) {
			utils::format("sample param error...\n"); 
		} else {
			int mode = 0;
			int ch = -1;
			int rate = -1;
			int num = -1;
			char fname[32];
			fname[0] = 0;
			for(uint8_t i = 1; i < cmdn; ++i) {
				char tmp[32];
				if(cmd_.get_word(i, sizeof(tmp), tmp)) {
					if(tmp[0] == '-') {
						if(strcmp(tmp, "-ch") == 0) {
							mode = 1;
						} else if(strcmp(tmp, "-rate") == 0) {
							mode = 2;
						} else if(strcmp(tmp, "-num") == 0) {
							mode = 3;
						} else {
							utils::format("sample option error: '%s'\n") % tmp;
							return true;
						}
					} else {
						bool f = false;
						switch(mode) {
						case 0:
							strcpy(fname, tmp);
							f = true;
							break;
						case 1:
							f = (utils::input("%d", tmp) % ch).status();
							break;
						case 2:
							f = (utils::input("%d", tmp) % rate).status();
							break;
						case 3:
							f = (utils::input("%d", tmp) % num).status();
							break;
						}
						mode = 0;
						if(!f) {
							utils::format("sample option value error: '%s'\n") % tmp;
							return true;
						}
					}
				} else {
					return false;
				}
			}
			if(ch > 7 || ch < 0) {
				utils::format("sample chanel out range: %d\n") % ch;
			}

			utils::format("ch:   %d\n") % ch;
			utils::format("rate: %d\n") % rate;
			utils::format("num:  %d\n") % num;
			utils::format("name: '%s'\n") % fname;
		}
		return true;
	}


#ifdef SERVER_TASK
	void service_server()
	{
		EthernetClient& client = server_.available();
		if (client) {
			utils::format("new client\n");

			while (client.connected()) {
				if (client.available()) {
					char tmp[256];
					int len = client.read(tmp, 255);
					if(len > 0 && len < 256) {
						tmp[len] = 0;
	        			sci_puts(tmp);
					}

					// if you've gotten to the end of the line (received a newline
					// character) and the line is blank, the http request has ended,
					// so you can send a reply
					if (len > 0 && tmp[len - 1] == '\n') {
						// send a standard http response header
						client.println("HTTP/1.1 200 OK");
          				client.println("Content-Type: text/html");
						// the connection will be closed after completion of the response
          				client.println("Connection: close");
						client.println("Refresh: 5");  // refresh the page automatically every 5 sec
						client.println();
						client.println("<!DOCTYPE HTML>");
						client.println("<html>");
						client.println("<font size=\"5\">");
						{  // 時間表示
							char tmp[128];
							time_t t = get_time_();
							disp_time_(t, tmp, sizeof(tmp));
							client.print(tmp);
							client.println("<br/ >");
						}
						// アナログ入力の表示
						for (int ach = 0; ach < 4; ++ach) {
							char tmp[128];
							int v = rand() & (4096 - 1);
							utils::format("analog input(%d): %d", tmp, sizeof(tmp)) % ach % v;
							client.print(tmp);
							client.println("<br/ >");
						}
						client.println("</font>");
						client.println("</html>");
						break;
					}
				}
			}
			// give the web browser time to receive the data
///			delay(1);
			// close the connection:
			client.stop();
			utils::format("client disconnected\n");
		}
	}
#endif

#if 0
	void telnet_service()
	{
		EthernetClient& client = telnet_.available();
		if(client) {
			char tmp[256];
			int len = client.read(tmp, 256);
			if(len > 0) {
				client.write(tmp, len);
			}
		}
	}
#endif
}


int main(int argc, char** argv);

int main(int argc, char** argv)
{
	device::PORT3::PCR.B5 = 1; // P35 pull-up

	device::SYSTEM::PRCR = 0xA50B;	// クロック、低消費電力、関係書き込み許可

	device::SYSTEM::MOSCWTCR = 9;	// 1ms wait
	// メインクロック強制発振とドライブ能力設定
	device::SYSTEM::MOFCR = device::SYSTEM::MOFCR.MODRV2.b(0b10)
						  | device::SYSTEM::MOFCR.MOFXIN.b(1);
	device::SYSTEM::MOSCCR.MOSTP = 0;		// メインクロック発振器動作
	while(device::SYSTEM::OSCOVFSR.MOOVF() == 0) asm("nop");

	// Base Clock 12.5MHz
	// PLLDIV: 1/1, STC: 16 倍(200MHz)
	device::SYSTEM::PLLCR = device::SYSTEM::PLLCR.PLIDIV.b(0) |
							device::SYSTEM::PLLCR.STC.b(0b011111);
	device::SYSTEM::PLLCR2.PLLEN = 0;			// PLL 動作
	while(device::SYSTEM::OSCOVFSR.PLOVF() == 0) asm("nop");

	device::SYSTEM::SCKCR = device::SYSTEM::SCKCR.FCK.b(2)		// 1/2 (200/4=50)
						  | device::SYSTEM::SCKCR.ICK.b(1)		// 1/2 (200/2=100)
						  | device::SYSTEM::SCKCR.BCK.b(2)		// 1/2 (200/4=50)
						  | device::SYSTEM::SCKCR.PCKA.b(1)		// 1/2 (200/2=100)
						  | device::SYSTEM::SCKCR.PCKB.b(2)		// 1/4 (200/4=50)
						  | device::SYSTEM::SCKCR.PCKC.b(2)		// 1/4 (200/4=50)
						  | device::SYSTEM::SCKCR.PCKD.b(2);	// 1/4 (200/4=50)
	device::SYSTEM::SCKCR2 = device::SYSTEM::SCKCR2.UCK.b(0b0011) | 1;  // USB Clock: 1/4 (200/4=50)
	device::SYSTEM::SCKCR3.CKSEL = 0b100;	///< PLL 選択

	{  // DIP-SW プルアップ
		SW1::DIR = 0;  // input
		SW2::DIR = 0;  // input
		SW1::PU = 1;
		SW2::PU = 1;
	}

	{  // タイマー設定、１０００Ｈｚ（１ｍｓ）
		uint8_t int_level = 1;
		cmt_.start(1000, int_level);
	}

	{  // SCI 設定
		uint8_t int_level = 2;
		sci_.start(115200, int_level);
	}

	{  // RTC 設定
		rtc_.start();
	}

	{  // SD カード・クラスの初期化
		sdc_.start();
	}

	{  // LTC2348ILX-16 初期化
		// 内臓リファレンスと内臓バッファ
		// VREFIN: 2.024V、VREFBUF: 4.096V、Analog range: 0V to 5.12V
//		if(!eadc_.start(200000, 0b001)) {
		if(!eadc_.start(1000000, 0b001)) {
			utils::format("LTC2348_16 not found...\n");
		}
	}

	{  // LAN initialize (PHY reset, PHY POWER-DOWN
		LAN_PDN::DIR = 1;  // output;
		LAN_PDN::P = 1;    // Not Power Down Mode..
		LAN_RESN::DIR = 1; // output;

		LAN_RESN::P = 0;
		utils::delay::milli_second(200); /// reset time
		LAN_RESN::P = 1;
	}

	// タイトル・コール
	utils::format("\nStart Seeda03 Version %d.%02d\n") % (seeda_version_ / 100) % (seeda_version_ % 100);
	utils::format("Endian: %3b") % static_cast<uint32_t>(device::SYSTEM::MDE.MDE());
	utils::format(", PCKA: %u [Hz]") % static_cast<uint32_t>(F_PCKA);
	utils::format(", PCKB: %u [Hz]\n") % static_cast<uint32_t>(F_PCKB);
	utils::format("DIP-Switch: %d\n") % static_cast<int>(get_switch_());
	if(eadc_.probe()) {
		utils::format("Device LTC2348-16: Ready\n");
		utils::format("    CH Span: %024b\n") % eadc_.get_span();
	} else {
		utils::format("Device LTC2348-16: Not Ready\n");
	}
	{
		time_t t = get_time_();
		if(t != 0) {
			disp_time_(t);
		}
	}


//	while(1) {
//		eadc_.convert();
//	}



#ifdef SERVER_TASK
	if(get_switch_() == 3) {  // Ethernet 起動
		device::power_cfg::turn(device::peripheral::ETHERCA);
		device::port_map::turn(device::peripheral::ETHERCA);
		set_interrupt_task(INT_Excep_ICU_GROUPAL1, static_cast<uint32_t>(device::icu_t::VECTOR::GROUPAL1));
		set_interrupt_task(INT_Excep_CMT1_CMI1, static_cast<uint32_t>(device::icu_t::VECTOR::CMI1));

		Ethernet.maininit();

		static const uint8_t mac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
		if(Ethernet.begin(mac) == 0) {
			utils::format("Ethernet Fail: begin (DHCP)...\n");

			utils::format("SetIP: ");
			IPAddress ipa(192, 168, 3, 20);
			Ethernet.begin(mac, ipa);
		} else {
			utils::format("DHCP: ");
		}
		Ethernet.localIP().print();

		server_.begin();
		utils::format("Start server: ");
		Ethernet.localIP().print();
	}
#endif

	cmd_.set_prompt("# ");

	device::PORT4::PDR.B7 = 1; // output

	uint32_t cnt = 0;

	while(1) {
		sync_100hz();

#ifdef SERVER_TASK
		Ethernet.mainloop();

		service_server();
#endif

		sdc_.service();

		// コマンド入力と、コマンド解析
		if(cmd_.service()) {
			uint8_t cmdn = cmd_.get_words();
			if(cmdn >= 1) {
				bool f = false;
				if(cmd_.cmp_word(0, "dir")) {  // dir [xxx]
					if(check_mount_()) {
						if(cmdn >= 2) {
							char tmp[128];
							cmd_.get_word(1, sizeof(tmp), tmp);
							sdc_.dir(tmp);
						} else {
							sdc_.dir("");
						}
					}
					f = true;
				} else if(cmd_.cmp_word(0, "cd")) {  // cd [xxx]
					if(check_mount_()) {
						if(cmdn >= 2) {
							char tmp[128];
							cmd_.get_word(1, sizeof(tmp), tmp);
							sdc_.cd(tmp);						
						} else {
							sdc_.cd("/");
						}
					}
					f = true;
				} else if(cmd_.cmp_word(0, "pwd")) { // pwd
					utils::format("%s\n") % sdc_.get_current();
					f = true;
				} else if(cmd_.cmp_word(0, "date")) {
					if(cmdn == 1) {
						time_t t = get_time_();
						if(t != 0) {
							disp_time_(t);
						}
					} else {
						set_time_date_();
					}
					f = true;
				} else if(cmd_.cmp_word(0, "reset")) {
					f = reset_signal_(cmdn);
				} else if(cmd_.cmp_word(0, "eadc")) {
					f = eadc_conv_(cmdn);
				} else if(cmd_.cmp_word(0, "sample")) {
					f = eadc_sample_(cmdn);
				} else if(cmd_.cmp_word(0, "help") || cmd_.cmp_word(0, "?")) {
					utils::format("date\n");
					utils::format("date yyyy/mm/dd hh:mm[:ss]\n");
					utils::format("dir [name]\n");
					utils::format("cd [directory-name]\n");
					utils::format("pwd\n");
					utils::format("reset [01]  (PHY reset signal)\n");
					utils::format("eadc [0-7]  (LTC2348 A/D conversion)\n");
					utils::format("sample -ch 0-7 -rate FRQ -num SAMPLE-NUM file-name (LTC2348 A/D sample)\n");
					f = true;
				}
				if(!f) {
					char tmp[128];
					if(cmd_.get_word(0, sizeof(tmp), tmp)) {
						utils::format("Command error: '%s'\n") % tmp;
					}
				}
			}
		}

		++cnt;
		if(cnt >= 30) {
			cnt = 0;
		}
		device::PORT4::PODR.B7 = (cnt < 10) ? 0 : 1;
	}
}
