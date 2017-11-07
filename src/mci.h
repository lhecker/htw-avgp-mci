#pragma once

namespace mci {

class error : std::runtime_error {
public:
	explicit error(const char* str);
	explicit error(const std::wstring_view str);
	explicit error(MCIERROR code);

	std::wstring wide_what() const;

private:
	static std::string from_error(MCIERROR code);
	static std::string wide_to_utf8(const std::wstring_view wstr);
	static std::wstring utf8_to_wide(const std::string_view str);
};

class tmsf {
public:
	explicit constexpr tmsf(uint8_t track = 0, uint8_t minute = 0, uint8_t second = 0, uint8_t frame = 0) : track(track), minute(minute), second(second), frame(frame) {
	}

	static constexpr tmsf from_tmsf_dword(DWORD value) {
		return tmsf(MCI_TMSF_TRACK(value), MCI_TMSF_MINUTE(value), MCI_TMSF_SECOND(value), MCI_TMSF_FRAME(value));
	}

	static constexpr tmsf from_msf_dword(DWORD value, uint8_t track = 0) {
		return tmsf(track, MCI_MSF_MINUTE(value), MCI_MSF_SECOND(value), MCI_MSF_FRAME(value));
	}

	constexpr DWORD as_tmsf_dword() {
		return MCI_MAKE_TMSF(track, minute, second, frame);
	}

	const uint8_t track;
	const uint8_t minute;
	const uint8_t second;
	const uint8_t frame;
};

class base {
public:
	base(const base&) = delete;
	base& operator=(const base&) = delete;

	base(base&&) = default;
	base& operator=(base&&) = default;

	virtual ~base();

	void play(tmsf from = tmsf(), tmsf to = tmsf());
	void set_paused(bool paused);

protected:
	explicit base(MCIDEVICEID device_id);

	static inline void send_open_command(DWORD_PTR param1, DWORD_PTR param2) {
		send_command(0, MCI_OPEN, param1, param2);
	}

	static inline void send_open_command(DWORD_PTR param1, void* param2) {
		send_command(0, MCI_OPEN, param1, reinterpret_cast<DWORD_PTR>(param2));
	}

	static inline void send_open_command(void* param1, DWORD_PTR param2) {
		send_command(0, MCI_OPEN, reinterpret_cast<DWORD_PTR>(param1), param2);
	}

	static inline void send_open_command(void* param1, void* param2) {
		send_command(0, MCI_OPEN, reinterpret_cast<DWORD_PTR>(param1), reinterpret_cast<DWORD_PTR>(param2));
	}

	inline void send_command(UINT message, DWORD_PTR param1, DWORD_PTR param2) {
		send_command(device_id, message, param1, param2);
	}

	inline void send_command(UINT message, DWORD_PTR param1, void* param2) {
		send_command(device_id, message, param1, reinterpret_cast<DWORD_PTR>(param2));
	}

	inline void send_command(UINT message, void* param1, DWORD_PTR param2) {
		send_command(device_id, message, reinterpret_cast<DWORD_PTR>(param1), param2);
	}

	inline void send_command(UINT message, void* param1, void* param2) {
		send_command(device_id, message, reinterpret_cast<DWORD_PTR>(param1), reinterpret_cast<DWORD_PTR>(param2));
	}

	enum class state {
		stopped = 0,
		playing,
		paused,
	};

	const MCIDEVICEID device_id;
	state device_state = state::stopped;

private:
	static void send_command(MCIDEVICEID device_id, UINT message, DWORD_PTR param1, DWORD_PTR param2);
};

class audiofile : public base {
public:
	explicit audiofile(std::wstring_view path);

private:
	static MCIDEVICEID open_audiofile(std::wstring_view path);
};

class videofile : public base {
public:
	explicit videofile(std::wstring_view path, HWND hwnd, CRect rect);

private:
	static MCIDEVICEID open_videofile(std::wstring_view path);
};

class audiocd : public base {
public:
	explicit audiocd(std::wstring_view drive = std::wstring_view());

	std::vector<tmsf> track_durations();
	tmsf position();
	void seek(tmsf to);

private:
	static MCIDEVICEID open_audiocd(std::wstring_view drive);
};

} // namespace mci
