#include "stdafx.h"

std::wstring_view cstring_view(const CSimpleString& str) {
	return std::wstring_view(str, str.GetLength());
}
