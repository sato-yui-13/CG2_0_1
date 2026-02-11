#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

using namespace MatrixMath;

void Sprite::Initialize(SpriteCommon* spriteCommon, std::string textureFilePath)
{

	//引数を受け取ってメンバ変数に記録する
	this->spriteCommon = spriteCommon;

	dxCommon = spriteCommon->GetDxCommon();

	CreateVertexData();

	CreateMaterialData();

	CreateTransformationMatrixData();
	
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	//単位行列を書き込んでおく
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	AdjustTextureSize();
}


void Sprite::Update() {

	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	//左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}

	//上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureIndex);
	float tex_Left = textureLeftTop.x / metadata.width;
	float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
	float tex_top = textureLeftTop.y / metadata.height;
	float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

	// 1枚目の三角形
	// 左下
	vertexData_[0].position = { left,bottom,0.0f,1.0f };
	vertexData_[0].texcoord = { tex_Left,1.0f };
	vertexData_[0].normal = { 0.0f,0.0f,-1.0f };

	////左上
	vertexData_[1].position = { left,top,0.0f,1.0f };
	vertexData_[1].texcoord = { tex_Left,0.0f };
	vertexData_[1].normal = { 0.0f,0.0f,-1.0f };

	////右下
	vertexData_[2].position = { right,bottom,0.0f,1.0f };
	vertexData_[2].texcoord = { tex_right,1.0f };
	vertexData_[2].normal = { 0.0f,0.0f,-1.0f };

	////右上
	vertexData_[3].position = { right,top,0.0f,1.0f };
	vertexData_[3].texcoord = { tex_right,0.0f };
	vertexData_[3].normal = { 0.0f,0.0f,-1.0f };

	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 1;	indexData[4] = 3; indexData[5] = 2;

	//transform.rotate.y += 0.01f;
	// Sprite
	Matrix4x4 worldMatrix = MakeAffine(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = Orthographic(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	transform.translate = { position.x,position.y,0.0f };
	transform.rotate = { 0.0f,0.0f,rotation };
	transformationMatrixData->WVP = Multipty(worldMatrix, Multipty(viewMatrix, projectionMatrix));
	transformationMatrixData->World = worldMatrix;
	transform.rotate = { 0.0f,0.0f,rotation };
	transform.scale = { size.x,size.y,1.0f };
}

void Sprite::Draw()
{
	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
	dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);


	// マテリアルCBuffer
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

	//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, dxCommon->GetSRVGPUDescriptorHandle(1));
	dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
	//描画!(DrawCall/ドローコー)6個のインデックスを使用し1つのインスタンスを描画。その他は当面0で良い
	dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void Sprite::CreateVertexData()
{

	// Sprite用の頂点リソースを作る
	vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * 4);
	indexResource = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);

	// リソースの先頭のアドレスから作成する
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	//リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	//使用するするリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;

	//インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	//インデックスにリソースデータを書き込む


	// 書き込むためのアドレス取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	

}

void Sprite::CreateMaterialData()
{

	materialResource = dxCommon->CreateBufferResource(sizeof(Material));

	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	materialData->color = Vector4{ 1.0f,1.0f,1.0f,1.0f };

	materialData->enableLighting = false;

	materialData->uvTransform = MakeIdentity4x4();

}

void Sprite::CreateTransformationMatrixData()
{

	// Sprite用のTransformationMatirx用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	transformationMatrixResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込む

	// 書き込むためのアドレスを取得
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::AdjustTextureSize()
{
	//テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureIndex);
	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	//画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}

