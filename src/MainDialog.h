#pragma once

#include "mci.h"

class MainDialog : public CDialog {
	DECLARE_MESSAGE_MAP()

public:
	MainDialog(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum {
		IDD = IDD_HTWAVGP_DIALOG
	};
#endif

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void set_is_paused(bool paused);

	HICON m_hIcon;

	std::shared_ptr<mci::audiofile> current_audiofile;
	std::shared_ptr<mci::videofile> current_videofile;
	std::shared_ptr<mci::audiocd> current_audiocd;
	std::vector<mci::tmsf> current_audiocd_track_durations;
	bool is_paused = false;

public:
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR timer_id);
	afx_msg void OnBnClickedOpenAudio();
	afx_msg void OnBnClickedOpenVideo();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedOpenCd();
	afx_msg void OnLbnSelchangeTracklist();
};
