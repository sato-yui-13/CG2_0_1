#pragma once
#include <windows.h>
#include <wrl.h>
#include <cstdint>

//WindowsAPI
class WinApp
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public://メンバ関数
	//初期化
	void Initialize();

	//更新
	void Update();

	// クライアント領域サイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	//getter
	HWND GetHwnd() const { return hwnd; }

	//getter
	HINSTANCE GetHinstance() const { return wc.hInstance; }

	//終了
	void Finalize();

	bool ProcessMessage();
private:
	//ウィンドウハンドル
	HWND hwnd = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc{};

};