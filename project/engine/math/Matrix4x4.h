#pragma once
#include "Vector3.h"

struct Matrix4x4 {

	float m[4][4];
};

namespace MatrixMath {
	// 行列の加法
	Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
	// 行列の減法
	Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
	// 行列の積
	Matrix4x4 Multipty(const Matrix4x4& m1, const Matrix4x4& m2);
	// 逆行列
	Matrix4x4 Inverse(const Matrix4x4& m);
	// 転置行列
	Matrix4x4 Transpoce(const Matrix4x4& m);
	// 単位行列の作成
	Matrix4x4 MakeIdentity4x4();
	// 平行移動行列
	Matrix4x4 MakeTranslate(const Vector3& translate);
	// 拡大縮小行列
	Matrix4x4 MakeScale(const Vector3& scale);

	// X軸の回転行列
	Matrix4x4 MakeRotateX(float radian);
	// Y軸の回転行列
	Matrix4x4 MakeRotateY(float radian);
	// Z軸の回転行列
	Matrix4x4 MakeRotateZ(float radian);

	// 3次元アフィン変換行列
	Matrix4x4 MakeAffine(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// 正射影行列
	Matrix4x4 Orthographic(float left, float top, float right, float bottom, float nearClip, float farClip);
	// 透視投影行列
	Matrix4x4 PerspectiveFov(float fovY, float aspectRatio, float nearClip, float farClip);
	// ビューポート変換行列
	Matrix4x4 Viewport(float left, float top, float width, float height, float minDepth, float maxDepth);
	// クロス積
	Vector3 Cross(const Vector3& v1, const Vector3& v2);

}
