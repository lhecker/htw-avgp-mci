#include "stdafx.h"

#include "MainDialog.h"

#include "MainApp.h"

static const UINT_PTR kAudioCdStatusTimerId = (UINT_PTR)&kAudioCdStatusTimerId;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(MainDialog, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_OPEN_AUDIO, &MainDialog::OnBnClickedOpenAudio)
	ON_BN_CLICKED(IDC_OPEN_VIDEO, &MainDialog::OnBnClickedOpenVideo)
	ON_BN_CLICKED(IDC_PAUSE, &MainDialog::OnBnClickedPause)
	ON_BN_CLICKED(IDC_STOP, &MainDialog::OnBnClickedStop)
	ON_BN_CLICKED(IDC_OPEN_CD, &MainDialog::OnBnClickedOpenCd)
	ON_LBN_SELCHANGE(IDC_TRACKLIST, &MainDialog::OnLbnSelchangeTracklist)
END_MESSAGE_MAP()

MainDialog::MainDialog(CWnd* pParent) : CDialog(IDD_HTWAVGP_DIALOG, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void MainDialog::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
}

BOOL MainDialog::OnInitDialog() {
	CDialog::OnInitDialog();

	// Set the icon for this dialog. The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE); // Set small icon

	::SetTimer(m_hWnd, kAudioCdStatusTimerId, 100, nullptr);

	return TRUE; // return TRUE unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
// to draw the icon. For MFC applications using the document/view model,
// this is automatically done for you by the framework.
void MainDialog::OnPaint() {
	if (!IsIconic()) {
		CDialog::OnPaint();
		return;
	}

	CPaintDC dc(this); // device context for painting

	SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

	// Center icon in client rectangle
	int cxIcon = GetSystemMetrics(SM_CXICON);
	int cyIcon = GetSystemMetrics(SM_CYICON);
	CRect rect;
	GetClientRect(&rect);
	int x = (rect.Width() - cxIcon + 1) / 2;
	int y = (rect.Height() - cyIcon + 1) / 2;

	// Draw the icon
	dc.DrawIcon(x, y, m_hIcon);
}

// The system calls this function to obtain the cursor to
// display while the user drags the minimized window.
HCURSOR MainDialog::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void MainDialog::OnTimer(UINT_PTR timer_id) {
	if (timer_id != kAudioCdStatusTimerId) {
		return;
	}

	if (!current_audiocd) {
		return;
	}

	auto label = GetDlgItem(IDC_CD_POSITION);
	const auto duration = current_audiocd->position();
	const auto track = current_audiocd_track_durations[duration.track - 1];
	const auto progress =
		(size_t(duration.minute) * 600000 + size_t(duration.second) * 10000 + size_t(duration.frame)) /
		(size_t(track.minute) * 6000 + size_t(track.second) * 100 + size_t(track.frame));

	CString str;
	str.Format(L"[%02d] %02d:%02d (%d%%)", duration.track, duration.minute, duration.second, progress);
	label->SetWindowTextW(str);
}

void MainDialog::OnBnClickedOpenAudio() {
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, L"Audio-Files (*.wav, *.mp3, *.mid)|*.wav;*.mp3;*.mid|All Files (*.*)|*.*||");
	if (dlg.DoModal() != IDOK) {
		return;
	}

	current_audiofile.reset();

	std::shared_ptr<mci::audiofile> f;
	try {
		f = std::make_shared<mci::audiofile>(cstring_view(dlg.GetPathName()));
		f->play();
	} catch (const mci::error &e) {
		AfxMessageBox(e.wide_what().c_str());
		return;
	}

	current_audiofile = f;
}

void MainDialog::OnBnClickedOpenVideo() {
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, L"Video-Files (*.mpg)|*.mpg|All Files (*.*)|*.*||");
	if (dlg.DoModal() != IDOK) {
		return;
	}

	current_videofile.reset();

	const CWnd* canvas = GetDlgItem(IDC_VIDEO_CANVAS);
	CRect canvas_rect;
	canvas->GetClientRect(&canvas_rect);

	std::shared_ptr<mci::videofile> f;
	try {
		f = std::make_shared<mci::videofile>(cstring_view(dlg.GetPathName()), canvas->m_hWnd, canvas_rect);
		f->play();
	} catch (const mci::error &e) {
		AfxMessageBox(e.wide_what().c_str());
		return;
	}

	current_videofile = f;
}

void MainDialog::OnBnClickedPause() {
	if (!current_audiofile && !current_videofile && !current_audiocd) {
		return;
	}

	set_is_paused(!is_paused);
}

void MainDialog::OnBnClickedStop() {
	current_audiofile.reset();
	current_videofile.reset();
	current_audiocd.reset();
	set_is_paused(false);
}

void MainDialog::OnBnClickedOpenCd() {
	current_audiocd.reset();

	std::shared_ptr<mci::audiocd> f;
	try {
		f = std::make_shared<mci::audiocd>();
		current_audiocd_track_durations = f->track_durations();
		f->play();
	} catch (const mci::error &e) {
		AfxMessageBox(e.wide_what().c_str());
		return;
	}

	current_audiocd = f;

	{
		auto list_box = static_cast<CListBox*>(GetDlgItem(IDC_TRACKLIST));

		list_box->ResetContent();

		for (const auto duration : current_audiocd_track_durations) {
			CString str;
			str.Format(L"[%02d] %02d:%02d", duration.track, duration.minute, duration.second);
			list_box->AddString(str);
		}
	}
}

void MainDialog::set_is_paused(bool paused) {
	is_paused = paused;

	try {
		if (current_audiofile) {
			current_audiofile->set_paused(is_paused);
		}
		if (current_videofile) {
			current_videofile->set_paused(is_paused);
		}
		if (current_audiocd) {
			current_audiocd->set_paused(is_paused);
		}
	} catch (const mci::error &e) {
		AfxMessageBox(e.wide_what().c_str());
		return;
	}

	auto button = GetDlgItem(IDC_PAUSE);
	button->SetWindowTextW(is_paused ? L"Continue" : L"Pause");
}

void MainDialog::OnLbnSelchangeTracklist() {
	if (!current_audiocd) {
		return;
	}

	const auto list_box = static_cast<CListBox*>(GetDlgItem(IDC_TRACKLIST));
	const auto selection = list_box->GetCurSel();

	if (selection == LB_ERR) {
		return;
	}

	try {
		current_audiocd->seek(mci::tmsf(uint8_t(selection + 1)));
	} catch (const mci::error &e) {
		AfxMessageBox(e.wide_what().c_str());
	}
}
