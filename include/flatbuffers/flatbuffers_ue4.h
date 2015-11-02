#include "flatbuffers.h"

#pragma once

namespace flatbuffers {
namespace ue4 {

//strings -> Offset<Vector<T = Offset<String>>>
Offset<Vector<Offset<String>>> CreateVector(FlatBufferBuilder &_fbb, TArray<FString> arr) {
    std::vector<Offset<String>> v;
    v.reserve(arr.Num());
    for (const FString &s : arr) {
        v.push_back(_fbb.CreateString(TCHAR_TO_UTF8(*s)));
    }
    return _fbb.CreateVector(v);
}

// tables -> Offset<Vector<T = Offset<Table>>>
// U is a UObject with ToFlatbuffer
template<typename T>
auto CreateVector(FlatBufferBuilder &_fbb, const TArray<T*> &arr)
        -> typename std::enable_if<
            std::is_base_of<flatbuffers::Table, typename T::flatbuffer_t>::value,
                Offset<Vector<Offset<typename T::flatbuffer_t>>>>::type {
    std::vector<Offset<typename T::flatbuffer_t>> v;
    v.reserve(arr.Num());
    for (const T *elem : arr) {
        v.push_back(elem->ToFlatBuffer(_fbb));
    }
    return _fbb.CreateVector(v);
}

// structs -> Offset<Vector<const Struct *>>
// U is a Uobject with ToFlatbufferStruct
template<typename T>
auto CreateVector(FlatBufferBuilder &_fbb, const TArray<T*> &arr)
        -> typename std::enable_if<
            !std::is_base_of<flatbuffers::Table, typename T::flatbuffer_t>::value,
            Offset<Vector<const typename T::flatbuffer_t *>>>::type {
    std::vector<typename T::flatbuffer_t> v;
    v.reserve(arr.Num());
    for (const T *elem : arr) {
        v.push_back(*elem->ToFlatBufferStruct());
    }
    return _fbb.CreateVectorOfStructs(v);
}

// enums -> Offset<Vector<T = enum>>
// U needs to be cast to T enum
template<typename T, typename U>
auto CreateVector(FlatBufferBuilder &_fbb, const TArray<U> &arr)
        -> typename std::enable_if<std::is_enum<U>::value, Offset<Vector<T>>>::type {
    std::vector<T> v;
    v.reserve(arr.Num());
    for (const U &elem : arr) {
        v.push_back(static_cast<T>(elem));
    }
    return _fbb.CreateVector(v);
}

// scalars -> Offset<Vector<T = int>>
// T and U must be exactly the same types
template<typename T, typename U>
auto CreateVector(FlatBufferBuilder &_fbb, const TArray<U> &arr)
        -> typename std::enable_if<std::is_same<T, U>::value, Offset<Vector<T>>>::type {
    return _fbb.CreateVector(arr.GetData(), arr.Num());
}
}
}
