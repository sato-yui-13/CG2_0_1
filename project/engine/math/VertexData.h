#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"

/// <summary>
/// 頂点データの拡張
/// </summary>
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};