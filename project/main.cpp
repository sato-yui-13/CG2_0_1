#include "Matrix4x4.h"
#include "Matrix3x3.h"
#include "Vector2.h"
#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>// ファイルを書いたり読み込んだりするライブラリ
#include <sstream>
#include <chrono> // 時間を扱うライブラリ 
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <numbers>
#include <dxgidebug.h>
#include <dxcapi.h>
// Debug用のあれやこれを使えるようにする
#include <dbghelp.h>
#include <strsafe.h>
#include <wrl.h>
#include <xaudio2.h>
#define DIRECTINPUT_VERSION   0x0800 //DirectInput
#include <dinput.h>
#include "Input.h"
#include "WinApp.h"
#include "DirectXCommon.h"
#include "StringUtility.h"
#include "D3DResourceLeakChecker.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "Vector4.h"
#include "TextureManager.h"

#include "extarnals/imgui//imgui.h"
#include "extarnals/imgui/imgui_impl_dx12.h"
#include "extarnals/imgui/imgui_impl_win32.h"
#include "extarnals/DirectXTex/d3dx12.h"

#include "extarnals/DirectXTex/DirectXTex.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

using namespace MatrixMath;

#pragma region 構造体



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
// Transform変数を作る
Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
Transform cameraTransfrom{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
Transform transformSprite{ {1.0f,1.0f,1.0f,},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
Transform uvTransformSprite{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f}
};

struct Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct MaterialData {
	std::string textureFilePath;
};


struct TransformationMatrix
{
	Matrix4x4 WVP;

	Matrix4x4 World;
};

struct DirectionalLight
{

	Vector4 color;//!<ライトの色
	Vector3 direction;//!<ライトの向き
	float intensity;//!<輝度

};

struct ModelData {
	std::vector<VertexData>vertices;
	MaterialData material;
};



struct ChunkHeader
{
	char id[4];//チャンク毎のID
	int32_t size;//チャンクサイズ
};

struct RifferHeader
{
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk
{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData
{
	//波形のフォーマット
	WAVEFORMATEX wfex;
	//バッファの先頭アドレス
	BYTE* pBuffer;
	//バッファのサイズ
	unsigned int bufferSize;
};

#pragma endregion

//#pragma region クラス
//class ResourceObject {
//public:
//	ResourceObject(ID3D12Resource* resource)
//		:resource_(resource)
//	{}
//	~ResourceObject() {
//		if (resource_) {
//			resource_->Release();
//		}
//	}
//	ID3D12Resource* Get() { return resource_; }
//private:
//	ID3D12Resource* resource_;
//};
#pragma endregion

#pragma region Creash関数
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// 時刻を取得して、時刻を名前に手に入れたファイルを作成。Dumpsディレクトリを以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// 他に関連づけられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}

#pragma endregion

Vector3 Normalize(const Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f)
		return { 0.0f, 0.0f, 0.0f };
	return { v.x / length, v.y / length, v.z / length };
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	Matrix4x4 result = {};

	result.m[0][0] = scale.x;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = scale.y;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = scale.z;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result = {};

	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = std::cos(radian);
	result.m[1][2] = std::sin(radian);
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = -std::sin(radian);
	result.m[2][2] = std::cos(radian);
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {};

	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;

	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;
	return result;
}


#pragma region log関数


// log系
std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}



std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}






#pragma endregion

#pragma region ウィンドウ関数

#pragma endregion



#pragma region objファイルを読み込む関数

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	//1.中で必要となる変数の宣言
	MaterialData materialData;//構築するMaterialData
	std::string line;//ファイルを
	//2.ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());
	//3.実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identiferに応じた処理
		if (identifier == "map==Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//凍結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	//4.MaterialDataを返す
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	//1.中で必要となる変数の宣言
	ModelData modelData;//構築するModelData
	std::vector<Vector4>positions;//座標
	std::vector<Vector3>normals;//法線
	std::vector<Vector2>texcoords;//テクスチャー座標
	std::string line;//ファイルから読んだ1行を格納するもの

	//2.ファイルを開く
	std::fstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかったら止める

	//3.実際にファイルを読み込み、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;//先頭の識別子を読む

		//identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのindexは「位置/UV/法線」で格納しているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');//  /区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				VertexData vertex = { position,texcoord,normal };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position,texcoord,normal };
			}
			//頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			//materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);

		}
	}

	//4.ModelDataを返す
	return modelData;
}

#pragma endregion




#pragma region DescriptorHeap作成関数

//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
//	Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
//	UINT numDescriptors, bool shaderVisible) {
//
//	// DXGIファクトリー
//	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
//	// HRESULTはWindows系のエラーコードであり、
//	// 関数が成功したかどうかをSUCCEDEDマクロで判定できる
//	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
//
//	// ディスクリプタヒープの生成
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
//	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
//	descriptorHeapDesc.Type = heapType; // 連打―ターゲットビュー用
//	descriptorHeapDesc.NumDescriptors = numDescriptors; // ダブルバッファ用に2つ。多くても別に構わない。
//	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
//	// ディスクリプタヒープが作れなかったので起動できない
//	assert(SUCCEEDED(hr));
//	return descriptorHeap;
//
//}


#pragma endregion

#pragma region TextrueResource関数






//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {
//
//	// 生成するResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = width; // Textureの幅
//	resourceDesc.Height = height; // Textureの高さ
//	resourceDesc.MipLevels = 1; // mipmap
//	resourceDesc.DepthOrArraySize = 1; // 奥行きor 配列Textureの配列数
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
//	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; //DepthStencilとして使う通知
//
//	// 利用するHrapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
//
//	// 深度値のクリア設定
//	D3D12_CLEAR_VALUE depthClearValue{};
//	depthClearValue.DepthStencil.Depth = 1.0f; // 1.0f(最大値)でクリア
//	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる
//
//	// Resorceの生成
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, // Heepの設定
//		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし
//		&resourceDesc, // Resourceの設定
//		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
//		&depthClearValue, //Clear最適値
//		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
//	assert(SUCCEEDED(hr));
//
//	return resource;
//}



#pragma endregion

#pragma region DescriptorHandleの関数化



#pragma endregion

#pragma region Comptr関数

//Microsoft::WRL::ComPtr<Microsoft::WRL::ComPtr<ID3D12Resource>>CreateTextureResource(const Microsoft::WRL::ComPtr<Microsoft::WRL::ComPtr<ID3D12Device>>& device, const DirectX::TexMetadata& metadata);

#pragma endregion


//音声データの読み込み
SoundData SoundLoadWave(const char* filename)
{
	//HRESULT result;


	//ファイルオープン
	std::ifstream file;

	file.open(filename, std::ios_base::binary);

	assert(file.is_open());

	//wavデータ読み込み
	RifferHeader riff;
	file.read((char*)&riff, sizeof(riff));

	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}
	FormatChunk format = {};
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	//チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);


	ChunkHeader data;
	file.read((char*)&data, sizeof(data));

	if (strncmp(data.id, "JUNK", 4) == 0) {
		file.seekg(data.size, std::ios_base::cur);

		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	//Waveファイルを閉じる
	file.close();

	//Dataチャンクのデータ部(波形データ)の読み来み
	//ファイルクローズ


	//読み込んだ音声データをreturn
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

void SoundUnload(SoundData* soundData)
{
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

////音声再生
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData) {
//	HRESULT result;
//
//	//波形フォーマットをもとにSourceVoiceの生成
//	IXAudio2SourceVoice* pSourceVoice = nullptr;
//	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
//	assert(SUCCEEDED(result));
//
//	//再生する波形のデータの設定
//	XAUDIO2_BUFFER buf{};
//	buf.pAudioData = soundData.pBuffer;
//	buf.AudioBytes = soundData.bufferSize;
//	buf.Flags = XAUDIO2_END_OF_STREAM;
//
//	//波形のデータの再生
//	result = pSourceVoice->SubmitSourceBuffer(&buf);
//	result = pSourceVoice->Start();
//}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	D3DResourceLeakChecker leakCheck;



	// 誰も補掟しなかった場合に(Unhandled),補掟する関数を登録
	// main関数は始まってすぐに登録するといい
	SetUnhandledExceptionFilter(ExportDump);

#pragma region 基盤システムの初期化
	WinApp* winApp = nullptr;

	winApp = new WinApp();

	winApp->Initialize();


	DirectXCommon* dxCommon = nullptr;

	//DirectXの初期化
	dxCommon = new DirectXCommon();

	dxCommon->Initialize(winApp);

	//テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxCommon);
	

	SpriteCommon* spriteCommon = nullptr;

	//スプライト共通部の初期化
	spriteCommon = new SpriteCommon;
	spriteCommon->Initialize(dxCommon);

#pragma endregion

#pragma region log



	// ログのディレクトリを用意
	std::filesystem::create_directory("logs");
	// 現在時刻を取得（UTC時刻)
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	// 日本時間（PCの設定時間）に変換
	std::chrono::zoned_time localTime(std::chrono::current_zone(), nowSeconds);
	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	//時刻を使ってファイル名を決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

#pragma endregion

#pragma region ウィンドウ



#pragma endregion



#pragma region DirectX12を初期化しよう

	//音声読み込み
	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");


	HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

	result = xAudio2->CreateMasteringVoice(&masterVoice);

	//音声再生
	//SoundPlayWave(xAudio2.Get(), soundData1);



#pragma endregion

#pragma region 最初のシーンの初期化
	std::vector<std::string> textures = {
	"resources/uvChecker.png",
	"resources/monsterball.png"
	};

	//Sprite* sprite = new Sprite();
	std::vector<Sprite*> sprites;
	for (uint32_t i = 0; i < 5; ++i) {
		Sprite* sprite = new Sprite();
		// 2つの画像を交互に割り当てるために、i % 2でインデックスを切り替え
		std::string& textureFile = textures[i % 2];
		sprite->Initialize(spriteCommon, textureFile);
		/*sprite->Initialize(spriteCommon, textureFile);
		sprite->SetIsFlipX(false);
		sprite->SetIsFlipY(false);
		sprite->SetTextureLeftTop({ 0.0f, 0.0f });
		sprite->SetTextureSize({ 64.0f, 64.0f });*/
		sprite->SetSize({ 64.0f, 64.0f });
		sprite->SetPosition({ 100.0f + i * 200.0f, 100.0f });
		sprites.push_back(sprite);
	}
#pragma endregion


#pragma region DescriptorSize
	////2枚目のTextureを読んで転送する
	//DirectX::ScratchImage mipImages2 = dxCommon->LoadTexture(d"resources/monsterBall.png");

	//const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = dxCommon->CreateTextureResource(metadata2).Get();
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = dxCommon->UploadTextureData(textureResource2, mipImages2);


	////2枚目のSRVを作る
	////metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};

	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャー
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);


	//index=2の位置にDescriptorHeapを生成する
	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon->GetSRVCPUDescriptorHandle(2);

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = dxCommon->GetSRVGPUDescriptorHandle(2);

	//SRVを生成
	//dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);
#pragma endregion





	//// Textrueを読んで転送する
	//DirectX::ScratchImage mipImages = dxCommon->LoadTexture("resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureeResource = dxCommon->CreateTextureResource(metadata).Get();
	////Update the code to correctly retrieve the raw pointer from the ComPtr.  
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = dxCommon->CreateTextureResource(metadata).Get();
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = dxCommon->UploadTextureData(textureResource, mipImages);

	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ	
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	//SRVを作成するDescriptorHeapの場所を決める
	/*D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(1);*/
	//先頭はImguiが使っているのでその次を使
	//SRVの生成
	//dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	//DSVの設定


#pragma endregion






#pragma region PSO

	//// RootSignature作成
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//descriptionRootSignature.Flags =
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//// DescriptorRange
	//D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	//descriptorRange[0].BaseShaderRegister = 0; //0から始める
	//descriptorRange[0].NumDescriptors = 1; // 数は1つ
	//descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	//descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算



	//// PootParameter作成。複数設定できるので配列。
	//D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; //PixelShaderで使う 
	//rootParameters[0].Descriptor.ShaderRegister = 0; //レジスタ番号0とバインド 

	//rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; //VertexShaderで使う 
	//rootParameters[1].Descriptor.ShaderRegister = 0; //レジスタ番号0とバインド 

	//rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; //DescriptorTableを使う
	//rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
	//rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
	//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う

	//descriptionRootSignature.pParameters = rootParameters; //ルートバラメータ配列へのポインタ
	//descriptionRootSignature.NumParameters = _countof(rootParameters); //配列の長さ

	//// Sampler
	//D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //バイナリアフィルタ
	//staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //0∼1の範囲側をリピート 
	//staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; //比較しない
	//staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMipmap
	//staticSamplers[0].ShaderRegister = 0; //レジスタ番号0を使う
	//staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//descriptionRootSignature.pStaticSamplers = staticSamplers;
	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);



	//HRESULT hr;
	//// シリアライズしてバイナリにする
	//Microsoft::WRL::ComPtr<ID3D10Blob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr<ID3D10Blob> errorBlob = nullptr;
	//hr = D3D12SerializeRootSignature(&descriptionRootSignature,
	//	D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	//if (FAILED(hr)) {
	//	//Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	//	assert(false);

	//}

	//
	////バイナリを元に生成
	//Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	//hr = dxCommon->GetDevice()->CreateRootSignature(0,
	//	signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
	//	IID_PPV_ARGS(&rootSignature));
	//assert(SUCCEEDED(hr));

	//// InputLayout
	//D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	//inputElementDescs[0].SemanticName = "POSITION";
	//inputElementDescs[0].SemanticIndex = 0;
	//inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//inputElementDescs[1].SemanticName = "TEXCOORD";
	//inputElementDescs[1].SemanticIndex = 0;
	//inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	//inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//inputElementDescs[2].SemanticName = "NORMAL";
	//inputElementDescs[2].SemanticIndex = 0;
	//inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//inputLayoutDesc.pInputElementDescs = inputElementDescs;
	//inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//// BlendStateの
	//D3D12_BLEND_DESC blendDesc{};
	//// 全ての色要素を書き込む
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//blendDesc.RenderTarget[0].BlendEnable = true;
	//blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	//blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	//// RasterizerStateの設定
	//D3D12_RASTERIZER_DESC rasterizerDesc{};
	//// 裏面(時計周り)を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//// 三角形の中を塗りつぶす
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//// Shaderをコンパイルする
	//Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon->CompileShader(L"Object3D.VS.hlsl", L"vs_6_0");
	//assert(vertexShaderBlob != nullptr);

	//Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon->CompileShader(L"Object3D.PS.hlsl", L"ps_6_0");
	//assert(pixelShaderBlob != nullptr);

	////DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	////機能を有効化
	//depthStencilDesc.DepthEnable = true;
	////書き込み
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//// PSOを生成する
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignatrue
	//graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;  // InputLayout
	//graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	//vertexShaderBlob->GetBufferSize() }; // VertexShader
	//graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	//pixelShaderBlob->GetBufferSize() }; // PixelShader 
	//graphicsPipelineStateDesc.BlendState = blendDesc;// BlensState
	//graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;// RasterizerState
	//// 書き込むRTVの情報
	//graphicsPipelineStateDesc.NumRenderTargets = 1;
	//graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//// 利用するトポロジ(形状)のタイプ。三角形
	//graphicsPipelineStateDesc.PrimitiveTopologyType =
	//	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//// どのように画面に色を打ち込むかの設定(気にしなくて良い)
	//graphicsPipelineStateDesc.SampleDesc.Count = 1;
	//graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//// DepthStencilの設定
	//graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


	//// 実際に生成
	//Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPinelineState = nullptr;
	//hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
	//	IID_PPV_ARGS(&graphicsPinelineState));
	//assert(SUCCEEDED(hr));


#pragma endregion

#pragma region 頂点データの作成とビュー

	// 実際に頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//// リソースの先頭のアドレスから使う
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点3つ分
	//vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	//// 1頂点あたりのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	// 書き込むためのアドレス取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// //左下
	//vertexData[0] = { -0.5f,-0.5f,0.0f,1.0f };
	//vertexData[0].texcoord = { 0.0f,1.0f };
	//// 上
	//vertexData[1] = { 0.0f,0.5f,0.0f,1.0f };
	//vertexData[1].texcoord = { 0.5f,0.0f };
	//// 右下
	//vertexData[2] = { 0.5f,-0.5f,0.0f,1.0f };
	//vertexData[2].texcoord = { 1.0f,1.0f };


	//// 左下2
	//vertexData[3].position = { -0.5f,-0.5f,0.5f,1.0f };
	//vertexData[3].texcoord = { 0.0f,1.0f };
	//// 上2
	//vertexData[4].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexData[4].texcoord = { 0.5f,0.0f };
	//// 右下2
	//vertexData[5].position = { 0.5f,-0.5f,-0.5f,1.0f };
	//vertexData[5].texcoord = { 1.0f,1.0f };


#pragma endregion

#pragma region Index用

//	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);
//
//	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
//
//	//リソースの先頭のアドレスから使う
//	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
//
//	//使用するするリソースのサイズはインデックス6つ分のサイズ
//	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
//
//	//インデックスはuint32_tとする
//	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
//
//	//インデックスにリソースデータを書き込む
//	uint32_t* indexDataSprite = nullptr;
//	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
//	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
//	indexDataSprite[3] = 1;	indexDataSprite[4] = 3; indexDataSprite[5] = 2;
//
//#pragma endregion
//
//
//	// Sprite用のTransformationMatirx用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
//	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
//	// データを書き込む
//	Matrix4x4* transformationMatirxDataSprite = nullptr;
//
//	// 書き込むためのアドレスを取得
//	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatirxDataSprite));
//
//	*transformationMatirxDataSprite = MakeIdentity4x4();

#pragma region Spriteの実装

	//// Sprite用の頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//// リソースの先頭のアドレスから作成する
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点6つ分のサイズ
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//// 1頂点あたりのサイズ
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//VertexData* vertexDataSprite = nullptr;
	//// 書き込むためのアドレス取得
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	//// 1枚目の三角形
	//// 左下
	//vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	//vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };

	////左上
	//vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };

	////右下
	//vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };

	////右上
	//vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[3].texcoord = { 1.0f,0.0f };
	//vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };




#pragma endregion

#pragma region ModelDataを使う
	//ModelDataを使う
	//モデルの読み込み
	ModelData modelData = LoadObjFile("resources", "axis2.obj");

	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();//リソースの先頭のアドレスから使う
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());//使用するリソースのサイズは頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);//1頂点のサイズ

	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));//書き込むためのアドレスを取得
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());//頂点データをリソースにコピー

	//Obj用のtransformationMatrix用のリソースを作る。matrix4x4　1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceObj = dxCommon->CreateBufferResource(sizeof(Matrix4x4));
	//データを書き込む
	Matrix4x4* transformationMatrixDataObj = nullptr;
	//書き込むためのアドレスを取得
	transformationMatrixResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataObj));
	//単位行列を書き込んでおく
	*transformationMatrixDataObj = MakeIdentity4x4();


	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズ用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceObj = dxCommon->CreateBufferResource(sizeof(Material));
	//マテリアルにデータを書き込む
	Material* materialDataObj = nullptr;
	//書き込むためのアドレスを取得
	materialResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&materialDataObj));
	//今回は赤を書き込んでみる
	materialDataObj->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataObj->enableLighting = true;
	materialDataObj->uvTransform = MakeIdentity4x4();


	//WVP用のリソースを作る。matrix4x4　1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceObj = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	//データを読み込む
	TransformationMatrix* wvpDataObj = nullptr;
	//書き込むためのアドレスを取得
	wvpResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataObj));
	//単位行列を書き込んでおく
	wvpDataObj->WVP = MakeIdentity4x4();


#pragma endregion

#pragma region Sphereの実装

	const uint32_t kSubdivision = 16;
	const uint32_t vertexCount = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t indexCount = kSubdivision * kSubdivision * 6;


	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere = dxCommon->CreateBufferResource(sizeof(VertexData) * vertexCount);


	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	// リソースの先頭のアドレスから作成する
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * vertexCount;
	// 1頂点あたりのサイズ
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataSphere = nullptr;

	// 書き込むためのアドレス取得
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	// 経度分割1つ分の角度 
	const float kLonEvery = std::numbers::pi_v<float>*2.0f / float(kSubdivision);
	// 緯度分割1つ分の角度
	const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);




	// 緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {

		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;
		// 経度の方向に分割しながら線を描く
		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {

			float lon = kLonEvery * lonIndex;

			uint32_t index = latIndex * (kSubdivision + 1) + lonIndex;

			vertexDataSphere[index].position = {
				std::cosf(lat) * std::cosf(lon),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon),
				1.0f
			};
			vertexDataSphere[index].texcoord = {
				1.0f - float(lonIndex) / float(kSubdivision),
				1.0f - float(latIndex) / float(kSubdivision)
			};
			vertexDataSphere[index].normal = {
				std::cosf(lat) * std::cosf(lon),
				std::sinf(lat),
				std::cosf(lat) * std::sinf(lon)
			};

		}

	}


#pragma endregion

#pragma region indexSphere
	// インデックスリソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSphere = dxCommon->CreateBufferResource(sizeof(uint32_t) * indexCount);

	// インデックスバッファビュー作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * indexCount;
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータ書き込み
	uint32_t* indexDataSphere = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

	uint32_t currentIndex = 0;
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t a = latIndex * (kSubdivision + 1) + lonIndex;
			uint32_t b = (latIndex + 1) * (kSubdivision + 1) + lonIndex;
			uint32_t c = latIndex * (kSubdivision + 1) + (lonIndex + 1);
			uint32_t d = (latIndex + 1) * (kSubdivision + 1) + (lonIndex + 1);

			// 1枚目の三角形
			indexDataSphere[currentIndex++] = a;
			indexDataSphere[currentIndex++] = b;
			indexDataSphere[currentIndex++] = c;

			// 2枚目の三角形
			indexDataSphere[currentIndex++] = c;
			indexDataSphere[currentIndex++] = b;
			indexDataSphere[currentIndex++] = d;
		}
	}

#pragma endregion


#pragma region ViewportとScissor

	// ビューボート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WinApp::kClientWidth;
	viewport.Height = WinApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューボートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WinApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::kClientHeight;



#pragma endregion

#pragma region Material用

	// マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));
	// マテリアルにデータを書き込む
	Material* materialData = nullptr;
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 今回は白を書き込んでいる
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	materialData->enableLighting = true;

	materialData->uvTransform = MakeIdentity4x4();

#pragma endregion

#pragma region WVP

	//	// WVB用のリソースを作る。Matrix4x4 一つ分のサイズを用意する
	//	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
	//
	//	TransformationMatrix* wvpData = nullptr;
	//
	//	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//
	//
	//	// 単位行列を書き込んでおく
	//	wvpData->WVP = MakeIdentity4x4();
	//
	//
	//
	//
	//#pragma endregion
	//
	//#pragma region Sprite用のマテリアル
	//
	//	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = dxCommon->CreateBufferResource(sizeof(Material));
	//
	//	Material* materialSpriteData = nullptr;
	//
	//	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialSpriteData));
	//
	//	materialSpriteData->uvTransform = MakeIdentity4x4();
	//
	//
	//	*materialSpriteData = {};
	//
	//	materialSpriteData->color = Vector4{ 1.0f,1.0f,1.0f,1.0f };
	//
	//
	//	//SpriteはLightingしないのでfalseを設定する
	//
	//	materialSpriteData->enableLighting = false;



#pragma region 平行光原

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = dxCommon->CreateBufferResource(sizeof(DirectionalLight));

	DirectionalLight* directionalLightData = nullptr;

	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };

	directionalLightData->direction = { 0.0f,-1.0f,0.0f };

	directionalLightData->intensity = 1.0f;

#pragma endregion

#pragma region Inputの初期化
	//ポインタ
	Input* input = nullptr;
	//入力の初期化
	input = new Input();
	input->Initialize(winApp);

#pragma endregion

	// Transform変数を作る
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform cameraTransfrom{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
	Transform transformSprite{ {1.0f,1.0f,1.0f,},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};

	Transform transformObj{ {1.5f, 1.5f, 1.5f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	Transform cameraTransformObj{ {1.0f, 1.0f, 1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-10.0f} };

#pragma endregion

	bool useMonsterBall = false;


	while (true) {

		//Windowsのメッセージ処理
		if (winApp->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		}

		// ImGuiの開始処理
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
		ImGui::ShowDemoWindow();



		//input->Update();

		// ゲームの処理







		// 球描画
		//transform.rotate.y += 0.03f;
		Matrix4x4 worldMatrix = MakeAffine(transform.scale, transform.rotate, transform.translate);
		//sprite->wvpData->World = worldMatrix;
		Matrix4x4 cameraMatrix = MakeAffine(cameraTransfrom.scale, cameraTransfrom.rotate, cameraTransfrom.translate);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = PerspectiveFov(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
		Matrix4x4 worldViewProjectionMatrix = Multipty(worldMatrix, Multipty(viewMatrix, projectionMatrix));
		//wvpData->WVP = worldViewProjectionMatrix;

		//// Sprite
		//Matrix4x4 worldMatrixSprite = MakeAffine(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		//Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
		//Matrix4x4 projectionMatrixSprite = Orthographic(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
		//Matrix4x4 worldViewProjectionMatrixSprite = Multipty(worldMatrixSprite, Multipty(viewMatrixSprite, projectionMatrixSprite));
		//*transformationMatirxDataSprite = worldViewProjectionMatrixSprite;


		bool temp_enableLightFlag = (materialData->enableLighting == 1);

		directionalLightData->direction = Normalize(directionalLightData->direction);

		ImGui::Begin("Settings");

		ImGui::ColorEdit4("Color", &materialData->color.x);
		ImGui::SliderAngle("RotateX", &transformSprite.rotate.x, -500, 500);
		ImGui::SliderAngle("RotateY", &transformSprite.rotate.y, -500, 500);
		ImGui::SliderAngle("RotateZ", &transformSprite.rotate.z, -500, 500);
		ImGui::DragFloat3("transform", &transformSprite.translate.x, -180, 180);
		ImGui::DragFloat3("transformsphere", &transform.translate.x);
		ImGui::SliderAngle("SphereRotateX", &transform.rotate.x);
		ImGui::SliderAngle("SphereRotateY", &transform.rotate.y);
		ImGui::SliderAngle("SphereRotateZ", &transform.rotate.z);
		ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		if (ImGui::Checkbox("enableLightFlag", &temp_enableLightFlag)) {
			materialData->enableLighting = temp_enableLightFlag ? 1 : 0;
		}
		ImGui::SliderFloat3("Light", &directionalLightData->direction.x, -1.0f, 0.8f);

		ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);


		//数字の0キーが押されていたら
		//if (key[DIK_0])
		//{
		//	OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
		//	transformSprite.translate.x += 5.0f;
		//}

		if (input->PushKey(DIK_W)) {
			OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
			transformSprite.translate.y -= 5.0f;
		}

		if (input->PushKey(DIK_A)) {
			OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
			transformSprite.translate.x -= 5.0f;
		}


		if (input->PushKey(DIK_S)) {
			OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
			transformSprite.translate.y += 5.0f;
		}

		if (input->PushKey(DIK_D)) {
			OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
			transformSprite.translate.x += 5.0f;
		}

		if (input->TriggerKey(DIK_0)) {
			OutputDebugStringA("Hit 0\n");//出力ウィンドウに「Hit 0」と表示
			transformSprite.translate.x += 5.0f;
		}

#pragma region UVTransform
		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		uvTransformMatrix = Multipty(uvTransformMatrix, MakeRotateXMatrix(uvTransformSprite.rotate.z));
		uvTransformMatrix = Multipty(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
		//materialSpriteData->uvTransform = uvTransformMatrix;

#pragma endregion

		ImGui::End();

		// ImGuiの内部コマンドを生成する
		ImGui::Render();

		for (Sprite* sprite : sprites) {
			sprite->Update();
			Vector2 position = sprite->GetPosition();

			position += Vector2{ 0.1f,0.1f };

			sprite->SetPosition(position);
			//角度を変化させるテスト
			float rotation = sprite->GetRotation();

			rotation +=0.01f;
			sprite->SetRotation(rotation);

			//色を変化させるテスト
			Vector4 color = sprite->GetColor();
			color.x += 0.01f;
			if (color.x > 1.0f) {
				color.x -= 1.0f;
			}
			sprite->SetColor(color);

			Vector2 size = sprite->GetSize();
			size.x += 0.1f;
			size.y += 0.1f;
			sprite->SetSize(size);

		}




		dxCommon->PreDraw();

		spriteCommon->SetCommonDrawing();


		for (Sprite* sprite : sprites) {
			sprite->Draw();
		}

		
		//// rootSignatrueを設定。PSOに設定してるけど別途設定が必要
		//dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
		//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get()); //PSOを設定


		// 形状を設定。PSOに設定しているものとは別。同じものを設定すると考えておけば良い
		//dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#pragma region sphereの描画
		// wvp用のCBufferの場所を設定
//		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
//		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
//		// マテリアルCBuffer
//		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
//		//平行光源用CBufferの場所を設定
//		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
//
//		dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
//
//		// 描画！(DraoCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
//		//xCommon->GetCommandList()->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
//		dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
//#pragma endregion
//
//
//		
//
//		// 描画！(DraoCall/ドローコール)
//		dxCommon->GetCommandList()->DrawInstanced(6, 1, 0, 0);
//
//
//		dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);//IBVを設定
//
//		//描画!(DrawCall/ドローコー)6個のインデックスを使用し1つのインスタンスを描画。その他は当面0で良い
//		dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);


		// 実際のcommandListのImGuiの描画コマンドを積む
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());






		dxCommon->PostDraw();

		TextureManager::GetInstance()->ReleaseIntermediateResources();
	}

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#pragma region オブジェクトを解放

	//CloseHandle(fenceEvent);


#ifdef _DEBUG


#endif // _DEBUG

#pragma endregion

	//xAudio2解放
	xAudio2.Reset();

	//音声データ解放
	SoundUnload(&soundData1);

	//入力解放
	//delete input;

	//DirectX解放
	delete dxCommon;

	//出力ウィンドウへの文字出力
	//Log(logStream, "HelloWored\n");
	//Log(logStream, ConvertString(std::format(L"WSTRING{}\n", WinApp::kClientWidth)));

	//テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();

	for (Sprite* sprite : sprites) {
		delete sprite;
	}

	delete spriteCommon;



	winApp->Finalize();

	delete winApp;

	//WindowsAPI解放
	winApp = nullptr;

	return 0;

}