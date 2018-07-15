// CoverDownloadDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "MusicPlayer2.h"
#include "CoverDownloadDlg.h"
#include "afxdialogex.h"


// CCoverDownloadDlg 对话框

IMPLEMENT_DYNAMIC(CCoverDownloadDlg, CDialog)

CCoverDownloadDlg::CCoverDownloadDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_COVER_DOWNLOAD_DIALOG, pParent)
{

}

CCoverDownloadDlg::~CCoverDownloadDlg()
{
}

UINT CCoverDownloadDlg::SongSearchThreadFunc(LPVOID lpParam)
{
	CCoverDownloadDlg* pThis = (CCoverDownloadDlg*)lpParam;
	wstring search_result;
	pThis->m_search_rtn = CInternetCommon::HttpPost(pThis->m_search_url, search_result);		//向网易云音乐的歌曲搜索API发送http的POST请求
	if (theApp.m_cover_download_dialog_exit)
		return 0;
	pThis->m_search_result = search_result;
	::PostMessage(pThis->m_hWnd, WM_SEARCH_COMPLATE, 0, 0);		//搜索完成后发送一个搜索完成的消息
	return 0;
}

UINT CCoverDownloadDlg::CoverDownloadThreadFunc(LPVOID lpParam)
{
	CCoverDownloadDlg* pThis = (CCoverDownloadDlg*)lpParam;
	CInternetCommon::ItemInfo match_item = pThis->m_down_list[pThis->m_item_selected];
	wstring song_id = match_item.id;
	if (song_id.empty())
		return 0;
	wstring cover_url = CCoverDownloadCommon::GetAlbumCoverURL(song_id);
	if (cover_url.empty())
	{
		AfxMessageBox(_T("专辑封面下载失败，请检查你的网络连接！"), MB_ICONWARNING | MB_OK);
	}

	//获取要保存的专辑封面的文件路径
	CFilePathHelper cover_file_path;
	if (match_item.album == theApp.m_player.GetCurrentSongInfo().album)		//如果在线搜索结果的唱片集名称和歌曲的相同，则以“唱片集”为文件名保存
	{
		wstring album_name{ match_item.album };
		CCommon::FileNameNormalize(album_name);
		cover_file_path.SetFilePath(theApp.m_player.GetCurrentDir() + album_name);
	}
	else				//否则以歌曲文件名为文件名保存
	{
		cover_file_path.SetFilePath(theApp.m_player.GetCurrentDir() + theApp.m_player.GetCurrentSongInfo().file_name);
	}
	CFilePathHelper url_path(cover_url);
	cover_file_path.ReplaceFileExtension(url_path.GetFileExtension().c_str());

	//下面专辑封面
	if (CCommon::FileExist(cover_file_path.GetFilePath()))
		::DeleteFile(cover_file_path.GetFilePath().c_str());
	URLDownloadToFile(0, cover_url.c_str(), cover_file_path.GetFilePath().c_str(), 0, NULL);

	//将下载的专辑封面改为隐藏属性
	SetFileAttributes(cover_file_path.GetFilePath().c_str(), FILE_ATTRIBUTE_HIDDEN);

	::PostMessage(pThis->m_hWnd, WM_DOWNLOAD_COMPLATE, 0, 0);		//下载完成后发送一个搜索完成的消息

	return 0;
}

void CCoverDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COVER_DOWN_LIST, m_down_list_ctrl);
}

void CCoverDownloadDlg::ShowDownloadList()
{
	m_down_list_ctrl.DeleteAllItems();
	for (int i{}; i < m_down_list.size(); i++)
	{
		CString tmp;
		tmp.Format(_T("%d"), i + 1);
		m_down_list_ctrl.InsertItem(i, tmp);
		m_down_list_ctrl.SetItemText(i, 1, m_down_list[i].title.c_str());
		m_down_list_ctrl.SetItemText(i, 2, m_down_list[i].artist.c_str());
		m_down_list_ctrl.SetItemText(i, 3, m_down_list[i].album.c_str());
	}
}


BEGIN_MESSAGE_MAP(CCoverDownloadDlg, CDialog)
	ON_BN_CLICKED(IDC_SEARCH_BUTTON, &CCoverDownloadDlg::OnBnClickedSearchButton)
	ON_MESSAGE(WM_SEARCH_COMPLATE, &CCoverDownloadDlg::OnSearchComplate)
	ON_BN_CLICKED(IDC_DOWNLOAD_SELECTED, &CCoverDownloadDlg::OnBnClickedDownloadSelected)
	ON_NOTIFY(NM_CLICK, IDC_COVER_DOWN_LIST, &CCoverDownloadDlg::OnNMClickCoverDownList)
	ON_NOTIFY(NM_DBLCLK, IDC_COVER_DOWN_LIST, &CCoverDownloadDlg::OnNMDblclkCoverDownList)
	ON_NOTIFY(NM_RCLICK, IDC_COVER_DOWN_LIST, &CCoverDownloadDlg::OnNMRClickCoverDownList)
	ON_MESSAGE(WM_DOWNLOAD_COMPLATE, &CCoverDownloadDlg::OnDownloadComplate)
	ON_EN_CHANGE(IDC_TITLE_EDIT, &CCoverDownloadDlg::OnEnChangeTitleEdit)
	ON_EN_CHANGE(IDC_ARTIST_EDIT, &CCoverDownloadDlg::OnEnChangeArtistEdit)
END_MESSAGE_MAP()


// CCoverDownloadDlg 消息处理程序


BOOL CCoverDownloadDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_title = theApp.m_player.GetPlayList()[theApp.m_player.GetIndex()].title;
	m_artist = theApp.m_player.GetPlayList()[theApp.m_player.GetIndex()].artist;
	m_album = theApp.m_player.GetPlayList()[theApp.m_player.GetIndex()].album;

	if (m_title == DEFAULT_TITLE)		//如果没有标题信息，就把文件名设为标题
	{
		m_title = theApp.m_player.GetFileName();
		size_t index = m_title.rfind(L'.');
		m_title = m_title.substr(0, index);
	}
	if (m_artist == DEFAULT_ARTIST)	//没有艺术家信息，清空艺术家的文本
	{
		m_artist.clear();
	}
	if (m_album == DEFAULT_ALBUM)	//没有唱片集信息，清空唱片集的文本
	{
		m_album.clear();
	}
	m_file_name = theApp.m_player.GetFileName();

	SetDlgItemText(IDC_TITLE_EDIT, m_title.c_str());
	SetDlgItemText(IDC_ARTIST_EDIT, m_artist.c_str());

	//设置列表控件主题颜色
	m_down_list_ctrl.SetColor(theApp.m_app_setting_data.theme_color);

	//初始化搜索结果列表控件
	CRect rect;
	m_down_list_ctrl.GetClientRect(rect);
	int width0, width1, width2, width3;
	width0 = rect.Width() / 10;
	width1 = rect.Width() * 3 / 10;
	width2 = rect.Width() * 2 / 10;
	width3 = rect.Width() - DPI(21) - width0 - width1 - width2;

	m_down_list_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
	m_down_list_ctrl.InsertColumn(0, _T("序号"), LVCFMT_LEFT, width0);		//插入第1列
	m_down_list_ctrl.InsertColumn(1, _T("标题"), LVCFMT_LEFT, width1);		//插入第2列
	m_down_list_ctrl.InsertColumn(2, _T("艺术家"), LVCFMT_LEFT, width2);		//插入第3列
	m_down_list_ctrl.InsertColumn(3, _T("唱片集"), LVCFMT_LEFT, width3);		//插入第3列

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CCoverDownloadDlg::OnBnClickedSearchButton()
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_STATIC_INFO, _T("正在搜索……"));
	GetDlgItem(IDC_SEARCH_BUTTON)->EnableWindow(FALSE);		//点击“搜索”后禁用该按钮
	wstring keyword = CInternetCommon::URLEncode(m_artist + L' ' + m_title);	//搜索关键字为“艺术家 标题”，并将其转换成URL编码
	wchar_t buff[1024];
	swprintf_s(buff, L"http://music.163.com/api/search/get/?s=%s&limit=%d&type=1&offset=0", keyword.c_str(), 30);
	//int rtn = CLyricDownloadCommon::HttpPost(buff, m_search_result);		//向网易云音乐的歌曲搜索API发送http的POST请求
	m_search_url = buff;
	theApp.m_cover_download_dialog_exit = false;
	m_pSearchThread = AfxBeginThread(SongSearchThreadFunc, this);
}


afx_msg LRESULT CCoverDownloadDlg::OnSearchComplate(WPARAM wParam, LPARAM lParam)
{
	//响应WM_SEARCH_CONPLATE消息
	GetDlgItem(IDC_SEARCH_BUTTON)->EnableWindow(TRUE);	//搜索完成之后启用该按钮
	switch (m_search_rtn)
	{
	case 1: MessageBox(_T("搜索失败，请检查你的网络连接！"), NULL, MB_ICONWARNING); return 0;
	case 2: MessageBox(_T("搜索超时！"), NULL, MB_ICONWARNING); return 0;
	default: break;
	}

	CInternetCommon::DisposeSearchResult(m_down_list, m_search_result);		//处理返回的结果
	ShowDownloadList();			//将搜索的结果显示在列表控件中

	//计算搜索结果中最佳匹配项目
	int best_matched;
	bool id_releated{ false };
	if (!theApp.m_player.GetCurrentSongInfo().song_id.empty())		//如果当前歌曲已经有关联的ID，则根据该ID在搜索结果列表中查找对应的项目
	{
		for (size_t i{}; i<m_down_list.size(); i++)
		{
			if (theApp.m_player.GetCurrentSongInfo().song_id == m_down_list[i].id)
			{
				id_releated = true;
				best_matched = i;
				break;
			}
		}
	}
	if (!id_releated)
		best_matched = CInternetCommon::SelectMatchedItem(m_down_list, m_title, m_artist, m_album, m_file_name, true);
	CString info;
	if (m_down_list.empty())
		info = _T("搜索结果：（没有找到歌曲）");
	else if (best_matched == -1)
		info = _T("搜索结果：（似乎没有最佳匹配的项）");
	else if (id_releated)
		info.Format(_T("搜索结果：（已关联项：%d）"), best_matched + 1);
	else
		info.Format(_T("搜索结果：（最佳匹配项：%d）"), best_matched + 1);
	SetDlgItemText(IDC_STATIC_INFO, info);
	//自动选中列表中最佳匹配的项目
	m_down_list_ctrl.SetFocus();
	m_down_list_ctrl.SetItemState(best_matched, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);	//选中行
	m_down_list_ctrl.EnsureVisible(best_matched, FALSE);		//使选中行保持可见
	m_item_selected = best_matched;
	return 0;
}


void CCoverDownloadDlg::OnBnClickedDownloadSelected()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_item_selected < 0 || m_item_selected >= m_down_list.size()) return;
	GetDlgItem(IDC_DOWNLOAD_SELECTED)->EnableWindow(FALSE);		//点击“下载选中项”后禁用该按钮
	theApp.m_player.SetRelatedSongID(m_down_list[m_item_selected].id);		//将选中项目的歌曲ID关联到歌曲
	m_pDownThread = AfxBeginThread(CoverDownloadThreadFunc, this);
}


void CCoverDownloadDlg::OnNMClickCoverDownList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	m_item_selected = pNMItemActivate->iItem;
	*pResult = 0;
}


void CCoverDownloadDlg::OnNMDblclkCoverDownList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	m_item_selected = pNMItemActivate->iItem;
	if (m_item_selected >= 0 && m_item_selected < m_down_list.size())
	{
		OnBnClickedDownloadSelected();
	}
	*pResult = 0;
}


void CCoverDownloadDlg::OnNMRClickCoverDownList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	m_item_selected = pNMItemActivate->iItem;
	*pResult = 0;
}


void CCoverDownloadDlg::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类
	theApp.m_cover_download_dialog_exit = true;
	if (m_pSearchThread != nullptr)
		WaitForSingleObject(m_pSearchThread->m_hThread, 1000);	//等待线程退出
	if (m_pDownThread != nullptr)
		WaitForSingleObject(m_pDownThread->m_hThread, 1000);	//等待线程退出

	CDialog::OnOK();
}


void CCoverDownloadDlg::OnCancel()
{
	// TODO: 在此添加专用代码和/或调用基类
	theApp.m_cover_download_dialog_exit = true;
	if (m_pSearchThread != nullptr)
		WaitForSingleObject(m_pSearchThread->m_hThread, 1000);	//等待线程退出
	if (m_pDownThread != nullptr)
		WaitForSingleObject(m_pDownThread->m_hThread, 1000);	//等待线程退出

	CDialog::OnCancel();
}


afx_msg LRESULT CCoverDownloadDlg::OnDownloadComplate(WPARAM wParam, LPARAM lParam)
{
	//重新从本地获取专辑封面
	theApp.m_player.SearchOutAlbumCover();
	GetDlgItem(IDC_DOWNLOAD_SELECTED)->EnableWindow(TRUE);		//下载完成后启用该按钮
	MessageBox(_T("下载完成"), NULL, MB_ICONINFORMATION | MB_OK);
	return 0;
}


void CCoverDownloadDlg::OnEnChangeTitleEdit()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString tmp;
	GetDlgItemText(IDC_TITLE_EDIT, tmp);
	m_title = tmp;
}


void CCoverDownloadDlg::OnEnChangeArtistEdit()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString tmp;
	GetDlgItemText(IDC_ARTIST_EDIT, tmp);
	m_artist = tmp;
}
