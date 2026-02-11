#pragma once
#include <windows.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION  0x0800 //DirectInputのバージン指定
#include <dinput.h>
#include "WinApp.h"



class Input
{
public:

	void Initialize(WinApp* winApp);

	void Update();

	bool PushKey(BYTE keyNumber);


	bool TriggerKey(BYTE keyNumber);
	//namespace省略
	template <class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
private://メンバ変数
	//キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard;

	//DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput;

	BYTE key[256] = {};

	//前回の全キーの状態
	BYTE keyPre[256] = {};
	//WindowsAPI
	WinApp* winApp_ = nullptr;
};

