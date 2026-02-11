#pragma once
#include "DirectXCommon.h"

//スプライト共通部
class SpriteCommon
{
public://メンバ関数
	//初期化
	void Initialize(DirectXCommon* dxCommon);

	DirectXCommon* GetDxCommon() const { return dxCommon_; }

	//共通描画設定
	void SetCommonDrawing();

private:
	//ルートシグネイチャー
	void CreateRootSignature();

	//グラフィックスパイプラインの生成
	void CrateGraphicsPipeline();

	DirectXCommon* dxCommon_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

};

