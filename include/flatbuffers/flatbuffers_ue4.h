#include "flatbuffers.h"

namespace flatbuffers {
namespace ue4 {

//strings -> Offset<Vector<T = Offset<String>>>
Offset<Vector<Offset<String>>> CreateVector(flatbuffers::FlatBufferBuilder &_fbb, TArray<FString> arr) {
    std::vector<Offset<String>> v;
    v.reserve(arr.Num());
    for (const FString &s : arr) {
        v.push_back(_fbb.CreateString(TCHAR_TO_UTF8(*s)));
    }
    return _fbb.CreateVector(v);
}

// structs -> Offset<Vector<const Struct *>>
// U is a Uobject with ToFlatbufferStruct
template<typename T, typename U>
Offset<Vector<const Struct *>> CreateVector(flatbuffers::FlatBufferBuilder &_fbb, TArray<U *> foo) {
    std::vector<U> v;
    v.reserve(arr.Num());
    for (const U *elem : arr) {
        v.push_back(*U::ToFlatBufferStruct(elem));
    }
    return _fbb.CreateVectorOfStructs(v);
}

// tables -> Offset<Vector<T = Offset<Table>>>
// U is a UObject with ToFlatbuffer
template<typename T, typename U>
Offset<Vector<Offset<T>>> CreateVector(flatbuffers::FlatBufferBuilder &_fbb, TArray<U *> foo) {
    std::vector<Offset<U>> v;
    v.reserve(arr.Num());
    for (const U *elem : arr) {
        v.push_back(*U::ToFlatBuffer(elem));
    }
    return _fbb.CreateVector(v);
}

// scalars -> Offset<Vector<T = int>>
// T and U may not be exactly the same types
template<typename T, typename U>
Offset<Vector<T>> CreateVector(flatbuffers::FlatBufferBuilder &_fbb, TArray<U> foo) {
    std::vector<T> v;
    v.reserve(arr.Num());
    for (const U elem : arr) {
        v.push_back(elem);
    }
    return _fbb.CreateVector(v);
}
}
}
