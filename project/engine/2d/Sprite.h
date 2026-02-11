#pragma once
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

class SpriteCommon;


class TextureManager;



//スプライト
class Sprite
{

	struct TransformationMatrix
	{
		Matrix4x4 WVP;

		Matrix4x4 World;
	};

	struct Material
	{
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

public://メンバ関数
	//初期化
	void Initialize(SpriteCommon* spriteCommon, std::string textureFilePath);

	void Update();

	void Draw();


	//getter
	const Vector2& GetPosition()const { return position; }
	//setter
	void SetPosition(const Vector2& position) { this->position = position; }


	float GetRotation() const { return rotation; }
	void SetRotation(float rotation) { this->rotation = rotation; }


	const Vector4& GetColor()const { return materialData->color; }
	void SetColor(const Vector4& color) { materialData->color = color; }


	const Vector2& GetSize() const { return size; }
	void SetSize(const Vector2& size) { this->size = size; }


	//getter
	const Vector2& GetAnchorPoint()const { return anchorPoint; }
	//setter
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }


	const BOOL& GetIsFlipX()const { return isFlipX_; }
	void SetIsFlipX(const BOOL& isFlipX_) { this->isFlipX_ = isFlipX_; }


	const BOOL& GetIsFlipY()const { return isFlipY_; }
	void SetIsFlipY(const BOOL& isFlipY_) { this->isFlipY_ = isFlipY_; }


	const Vector2& GetTextureLeftTop()const { return textureLeftTop; }
	void SetTextureLeftTop(const Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }


	const Vector2& GetTextureSize() { return textureSize; }
	void SetTextureSize(const Vector2& textureSize) { this->textureSize = textureSize; }

private:
	SpriteCommon* spriteCommon = nullptr;

	DirectXCommon* dxCommon;

	VertexData* vertexData_;

	uint32_t* indexData;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;


	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;


	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;

	Material* materialData = nullptr;


	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Transform transformSprite{ {1.0f,1.0f,1.0f,},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};



	void CreateVertexData();

	void CreateMaterialData();

	void CreateTransformationMatrixData();

	Vector2 position = { 0.0f,0.0f };

	float rotation = 0.0f;

	Vector2 size = { 640.0f,360.0f };

	//テクスチャ番号
	uint32_t textureIndex = 0;

	Vector2 anchorPoint = { 0.0f,0.0f };

	//左右フリップ
	bool isFlipX_ = false;
	//上下スリップ
	bool isFlipY_ = false;

	//テクスチャ左上座標
	Vector2 textureLeftTop = { 0.0f,0.0f };
	//テクスチャ切り出しサイズ
	Vector2 textureSize = { 100.0f,100.0f };

	//テクスチャサイズをイメージに合わせる
	void AdjustTextureSize();
};

