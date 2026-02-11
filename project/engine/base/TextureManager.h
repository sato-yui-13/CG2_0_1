#pragma once
#include <string>
#include <cassert>
#include "StringUtility.h"
#define DIRECTINPUT_VERSION   0x0800 //DirectInput
#include "extarnals/DirectXTex/d3dx12.h"
#include "extarnals/DirectXTex/DirectXTex.h"


class DirectXCommon;

class TextureManager
{
public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	//初期化
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath">テクスチャファイルのパス</param>
	void LoadTexture(const std::string& filePath);

	void ReleaseIntermediateResources();

	//SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	//テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

	const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);

private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	//テクスチャ一枚分のデータ
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metaData;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	};

	//テクスチャデータ
	std::vector<TextureData>textureDatas;

	DirectXCommon*dxCommon_=nullptr;

	//SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

};

