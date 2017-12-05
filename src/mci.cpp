#include "stdafx.h"

#include "mci.h"

#include "MainApp.h"

namespace mci {

error::error(const char * str) : std::runtime_error(str) {
}

error::error(const std::wstring_view str) : std::runtime_error(wide_to_utf8(str)) {
}

error::error(MCIERROR code) : std::runtime_error(from_error(code)) {
}

std::wstring error::wide_what() const {
	return utf8_to_wide(what());
}

std::string error::from_error(MCIERROR code) {
	std::array<wchar_t, 128> buf;

	if (mciGetErrorStringW(code, buf.data(), buf.size())) {
		return wide_to_utf8(buf.data());
	}

	return "unknown";
}

std::string error::wide_to_utf8(const std::wstring_view wstr) {
	if (wstr.empty()) {
		return std::string();
	}
	if (wstr.size() > size_t(std::numeric_limits<int>::max())) {
		throw std::length_error("input string too long");
	}

	const int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string str(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
	return str;
}

std::wstring error::utf8_to_wide(const std::string_view str) {
	if (str.empty()) {
		return std::wstring();
	}
	if (str.size() > size_t(std::numeric_limits<int>::max())) {
		throw std::length_error("input string too long");
	}

	const int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

base::base(MCIDEVICEID device_id) : device_id(device_id) {
}

base::~base() {
	mciSendCommandW(device_id, MCI_CLOSE, MCI_WAIT, 0);
}

void base::play(tmsf from, tmsf to) {
	DWORD flags = 0;
	MCI_PLAY_PARMS parms = {};

	if (from.track) {
		flags |= MCI_FROM;
		parms.dwFrom = from.as_tmsf_dword();
	}
	if (to.track) {
		flags |= MCI_TO;
		parms.dwFrom = to.as_tmsf_dword();
	}

	send_command(MCI_PLAY, flags, &parms);

	device_state = state::playing;
}

void base::set_paused(bool paused) {
	const auto must_be_state = paused ? state::playing : state::paused;
	const auto will_be_state = paused ? state::paused : state::playing;

	if (device_state != must_be_state) {
		return;
	}

	if (paused) {
		send_command(MCI_PAUSE, MCI_WAIT, nullptr);
	} else {
		send_command(MCI_RESUME, MCI_WAIT, nullptr);
	}

	device_state = will_be_state;
}

void base::send_command(MCIDEVICEID device_id, UINT message, DWORD_PTR param1, DWORD_PTR param2) {
	const auto result = mciSendCommandW(device_id, message, param1, param2);

	if (result != 0) {
		throw error(result);
	}
}

audiofile::audiofile(std::wstring_view path) : base(open_audiofile(path)) {
}

MCIDEVICEID audiofile::open_audiofile(std::wstring_view path) {
	MCI_OPEN_PARMS parms = {};
	parms.lpstrElementName = path.data();

	send_open_command(MCI_OPEN_ELEMENT, &parms);

	return parms.wDeviceID;
}

videofile::videofile(std::wstring_view path, HWND hwnd, CRect rect) : base(open_videofile(path)) {
	{
		MCI_ANIM_WINDOW_PARMS parms;
		parms.hWnd = hwnd;
		send_command(MCI_WINDOW, MCI_ANIM_WINDOW_HWND, &parms);
	}

	{
		MCI_ANIM_RECT_PARMS parms;
		parms.rc = rect;
		send_command(MCI_PUT, MCI_ANIM_RECT | MCI_ANIM_PUT_DESTINATION, &parms);
	}
}

MCIDEVICEID videofile::open_videofile(std::wstring_view path) {
	MCI_OPEN_PARMS parms = {};
	parms.lpstrElementName = path.data();

	send_open_command(MCI_OPEN_ELEMENT, &parms);

	return parms.wDeviceID;
}

MCIDEVICEID audiocd::open_audiocd(std::wstring_view drive) {
	MCI_OPEN_PARMS parms = {};
	parms.lpstrDeviceType = (LPCWSTR)MCI_DEVTYPE_CD_AUDIO;

	DWORD flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID;

	if (!drive.empty()) {
		parms.lpstrElementName = drive.data();
		flags |= MCI_OPEN_ELEMENT;
	}

	send_open_command(flags, &parms);

	return parms.wDeviceID;
}

audiocd::audiocd(std::wstring_view drive) : base(open_audiocd(drive)) {
	MCI_SET_PARMS parms = {};
	parms.dwTimeFormat = MCI_FORMAT_TMSF;
	send_command(MCI_SET, MCI_SET_TIME_FORMAT, &parms);
}

std::vector<tmsf> audiocd::track_durations() {
	DWORD number_of_tracks;
	{
		MCI_STATUS_PARMS parms = {};
		parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
		send_command(MCI_STATUS, MCI_STATUS_ITEM, &parms);

		if (parms.dwReturn > 255) {
			parms.dwReturn = 255;
		}

		number_of_tracks = parms.dwReturn;
	}

	std::vector<tmsf> track_durations;
	track_durations.reserve(number_of_tracks);

	for (DWORD track = 1; track <= number_of_tracks; ++track) {
		MCI_STATUS_PARMS parms = {};
		parms.dwItem = MCI_STATUS_LENGTH;
		parms.dwTrack = track;
		send_command(MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK, &parms);

		track_durations.emplace_back(tmsf::from_msf_dword(parms.dwReturn, uint8_t(track)));
	}

	return track_durations;
}

tmsf audiocd::position() {
	MCI_STATUS_PARMS parms = {};
	parms.dwItem = MCI_STATUS_POSITION;

	send_command(MCI_STATUS, MCI_STATUS_ITEM, &parms);

	return tmsf::from_tmsf_dword(parms.dwReturn);
}

void audiocd::seek(tmsf to) {
	if (device_state == state::playing) {
		play(to);
	} else {
		MCI_SEEK_PARMS parms = {};
		parms.dwTo = to.as_tmsf_dword();
		send_command(MCI_SEEK, MCI_TO, &parms);
	}
}

} // namespace mci
