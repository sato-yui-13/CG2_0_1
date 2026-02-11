#define _USE_MATH_DEFINES 
#include "Matrix4x4.h"
#include <math.h>
#include <cassert>
#include <cmath>

using namespace MatrixMath;

// 行列の加法
Matrix4x4 MatrixMath::Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			result.m[row][col] = m1.m[row][col] + m2.m[row][col];
		}
	}

	return result;
}
// 行列の減法
Matrix4x4 MatrixMath::Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			result.m[row][col] = m1.m[row][col] - m2.m[row][col];
		}
	}
	return result;
}
// 4x4行列の積
Matrix4x4 MatrixMath::Multipty(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;

	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			result.m[row][col] = 0;
			for (int k = 0; k < 4; ++k) {
				result.m[row][col] += m1.m[row][k] * m2.m[k][col];
			}
		}
	}

	return result;
}
// 4x4行列の逆行列
Matrix4x4 MatrixMath::Inverse(const Matrix4x4& m) {
	float aug[4][8] = {};
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			aug[row][col] = m.m[row][col];
		}
	}
	// 単位行列の追加
	aug[0][4] = 1.0f;
	aug[1][5] = 1.0f;
	aug[2][6] = 1.0f;
	aug[3][7] = 1.0f;

	for (int i = 0; i < 4; i++) {
		// ピボットが0の場合下の行と入れ替える
		if (aug[i][i] == 0.0f) {
			for (int j = i + 1; j < 4; j++) {
				if (aug[j][i] != 0.0f) {
					// 行を交換する
					for (int k = 0; k < 8; k++) { // 列
						float copyNum = aug[i][k]; //もともとの上の行を代入
						aug[i][k] = aug[j][k]; //上の行
						aug[j][k] = copyNum; //下の行
					}
					break;
				}
			}
		}

		// ピボットを1にする
		float pivot = aug[i][i];
		for (int k = 0; k < 8; k++) {
			aug[i][k] /= pivot;
		}

		//i列目のピボット以外を0にする
		for (int j = 0; j < 4; j++) {
			if (j != i) {
				float factor = aug[j][i];
				for (int k = 0; k < 8; k++) {
					aug[j][k] -= factor * aug[i][k];
				}
			}
		}

	}

	Matrix4x4 result = {};
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			result.m[row][col] = aug[row][col + 4];
		}
	}

	return result;
}
// 転置行列
Matrix4x4 MatrixMath::Transpoce(const Matrix4x4& m) {
	Matrix4x4 result = {};

	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			result.m[row][col] = m.m[col][row];
		}
	}

	return result;
}
// 単位行列の作成
Matrix4x4 MatrixMath::MakeIdentity4x4() {
	Matrix4x4 result = {};

	for (int i = 0; i < 4; ++i) {
		result.m[i][i] = 1.0f;
	}
	return result;
}

// 平行移動行列
Matrix4x4 MatrixMath::MakeTranslate(const Vector3& translate) {

	Matrix4x4 result = {};
	// 単位行列の作成
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	// 平行移動の成分
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;// 完成した平行移動を返す
}
// 拡大縮小行列
Matrix4x4 MatrixMath::MakeScale(const Vector3& scale) {

	Matrix4x4 result = {};
	// 拡大率の設定
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;

	return result;// 拡大(スケ―リンク)行列を返す
}

//X軸の回転行列
Matrix4x4 MatrixMath::MakeRotateX(float radian) {

	Matrix4x4 result = {};
	// 3次元のX軸周りの回転行列
	result.m[0][0] = 1.0f;// X軸方向のベクトル変化しない
	result.m[1][1] = std::cos(radian); // Y成分の回転
	result.m[1][2] = std::sin(radian); // Z成分への影響
	result.m[2][1] = -std::sin(radian);// Y成分への影響  
	result.m[2][2] = std::cos(radian); // Z成分の回転
	result.m[3][3] = 1.0f;// 同時系列のw成分(固定値1)

	return result;// X軸の回転行列を返す
}
// Y軸の回転行列
Matrix4x4 MatrixMath::MakeRotateY(float radian) {

	Matrix4x4 result = {};
	// 3次元のY軸周りの回転行列
	result.m[0][0] = std::cos(radian); // X成分の回転
	result.m[0][2] = -std::sin(radian);// Z成分への影響
	result.m[1][1] = 1.0f;// Y軸は固定
	result.m[2][0] = std::sin(radian); // X成分への影響
	result.m[2][2] = std::cos(radian); // Z成分の回転
	result.m[3][3] = 1.0f;// 同次座標系のw成分(固定値1)

	return result;// Y軸の回転行列を返す
}
// Z軸の回転行列
Matrix4x4 MatrixMath::MakeRotateZ(float radian) {

	Matrix4x4 result = {};
	// 3次元のZ軸周りの回転行列
	result.m[0][0] = std::cos(radian); // X成分の回転
	result.m[0][1] = std::sin(radian);  // Y成分への影響  
	result.m[1][0] = -std::sin(radian); // X成分への影響
	result.m[1][1] = std::cos(radian);  // Y成分の回転
	result.m[2][2] = 1.0f;// Z軸は固定
	result.m[3][3] = 1.0f;// 同時座標系のw成分(固定値1)

	return result;// Z軸の回転行列を返す
}
// 3次元アフィン変換行列
Matrix4x4 MatrixMath::MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

	Matrix4x4 result = {};
	// 拡大縮小を生成
	Matrix4x4 scaleMatrix = MakeScale(scale);

	// X軸の回転行列を生成
	Matrix4x4 rotateXMatrix = MakeRotateX(rotate.x);
	// Y軸の回転行列を生成
	Matrix4x4 rotateYMatrix = MakeRotateY(rotate.y);
	// Z軸の回転行列を生成
	Matrix4x4 rotateZMatrix = MakeRotateZ(rotate.z);
	// X軸、Y軸、Z軸の順に回転を合成
	Matrix4x4 rotateXYZMatrix = Multipty(Multipty(rotateXMatrix, rotateYMatrix), rotateZMatrix);

	// 平行移動を生成
	Matrix4x4 translateMatrix = MakeTranslate(translate);

	// 最終的なアフィン変換行列： T * R * S
	result = Multipty(Multipty(scaleMatrix, rotateXYZMatrix), translateMatrix);



	return result;
}
// 正射影行列
Matrix4x4 MatrixMath::Orthographic(float left, float top, float right, float bottom, float nearClip, float farClip) {

	Matrix4x4 result = {};

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);
	result.m[3][3] = 1.0f;


	return result;
}
// 透視投影行列
Matrix4x4 MatrixMath::PerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip) {

	Matrix4x4 result = {};

	// コタンジェント(fovY/2)
	float f = 1.0f / std::tan(fovY / 2.0f);

	// 透視投影行列の各要素設定
	result.m[0][0] = f / aspectRatio;
	result.m[1][1] = f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

	return result;
}
// ビューポート変換行列
Matrix4x4 MatrixMath::Viewport(float left, float top, float width, float height, float minDepth, float maxDepth) {

	Matrix4x4 result = {};

	result.m[0][0] = width / 2.0f;
	result.m[1][1] = -height / 2.0f;
	result.m[2][2] = maxDepth - minDepth;
	result.m[3][0] = left + (width / 2.0f);
	result.m[3][1] = top + (height / 2.0f);
	result.m[3][2] = minDepth;
	result.m[3][3] = 1.0f;

	return result;
}
// クロス積
Vector3 MatrixMath::Cross(const Vector3& v1, const Vector3& v2) {
	return Vector3(
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x
	);
}