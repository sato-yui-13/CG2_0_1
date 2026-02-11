#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include "WinApp.h"
#include <dxcapi.h>
#include <string>
#include "StringUtility.h"
#include "extarnals/DirectXTex/DirectXTex.h"
#include <chrono>


//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


class DirectXCommon
{
public://メンバ関数

	void Initialize(WinApp* winApp);

	void Deviceinitialize();

	void CommandListInitialize();

	void SwapChainInitialize();

	/*Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DepthInitialize(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		UINT numDescriptors, bool shaderVisible);*/

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);


	/// <summary>
	/// 深度バッファの生成
	/// </summary>
	void CreateDescriptorHeaps();

	void CreateRTV();

	/// <summary>
	/// SRVの指定番号のCPUデスクリプタハンドルを取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

	void CreateDSV();

	void CreateFence();

	void SetViewportRect();

	void SetScissorRect();

	void CreateDXCCompiler();

	void InitializeImGui();

	void PreDraw();

	void PostDraw();

	//// --- Getter ---
	ID3D12Device* GetDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }

	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		// CompilerするShaderファイルへのパス
		const std::wstring& filePath,
		// Compilerに仕様するProfile
		const wchar_t* profile);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource>CreateTextureResource(const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource>UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages);

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	//最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;


private:
	WinApp* winApp = nullptr;

	//Timer timer;

	//DirectX12のデバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;

	//DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

	//コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	//コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	//コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

	//スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	//SRV用のDescriptorSize
	uint32_t descriptorSizeSRV;

	//RTV用のDescriptorSize
	uint32_t descriptorSizeRTV;

	//DSV用のDescriptorSize
	uint32_t descriptorSizeDSV;

	//RTVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;

	//SRVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;

	//DSVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	//スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	//RTVを2つ作るので、ディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	//RTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	//RTVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	//深度バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

	//フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	//フェンス用イベントハンドル
	HANDLE fenceEvent;

	//ビューポート
	D3D12_VIEWPORT viewport{ };

	//シザー矩形
	D3D12_RECT scissorRect{ };

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;

	//dxcCompiler
	IDxcUtils* dxcUtils;
	IDxcCompiler3* dxcCompiler;

	//includeHandler
	IDxcIncludeHandler* includeHandler;

	//TransitionBarrier
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	D3D12_RESOURCE_BARRIER barrier{};

	//フェンス値
	UINT64 fenceValue;

	//FPS固定初期化
	void InitializeFixFPS();

	//FPS固定更新
	void UpdateFixFPS();

	//メンバ関数
	//記録時間
	std::chrono::steady_clock::time_point reference_;
};

