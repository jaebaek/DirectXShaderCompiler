// Run: %dxc -T ps_6_0 -E main -fspv-reflect -fspv-target-env=vulkan1.2

// CHECK: OpName %type_StructuredBuffer_S "type.StructuredBuffer.S"
// CHECK: OpName %type_StructuredBuffer_T "type.StructuredBuffer.T"

// CHECK: OpName %type_RWStructuredBuffer_S "type.RWStructuredBuffer.S"
// CHECK: OpName %type_RWStructuredBuffer_T "type.RWStructuredBuffer.T"

// CHECK: OpDecorate %type_StructuredBuffer_S Block
// CHECK: OpDecorate %type_StructuredBuffer_T Block

// CHECK: OpDecorate %type_RWStructuredBuffer_S Block
// CHECK: OpDecorate %type_RWStructuredBuffer_T Block

struct S {
    float    a;
    float3   b;
    float2x3 c;
};

// CHECK: %_ptr_Uniform_type_StructuredBuffer_S = OpTypePointer StorageBuffer %type_StructuredBuffer_S
// CHECK: %_ptr_Uniform_type_StructuredBuffer_T = OpTypePointer StorageBuffer %type_StructuredBuffer_T

// CHECK: %_ptr_Uniform_type_RWStructuredBuffer_S = OpTypePointer StorageBuffer %type_RWStructuredBuffer_S
// CHECK: %_ptr_Uniform_type_RWStructuredBuffer_T = OpTypePointer StorageBuffer %type_RWStructuredBuffer_T
struct T {
    float    a[3];
    float3   b[4];
    S        c[3];
    float3x2 d[4];
};

// CHECK: %mySBuffer1 = OpVariable %_ptr_Uniform_type_StructuredBuffer_S StorageBuffer
StructuredBuffer<S> mySBuffer1 : register(t1);
// CHECK: %mySBuffer2 = OpVariable %_ptr_Uniform_type_StructuredBuffer_T StorageBuffer
StructuredBuffer<T> mySBuffer2 : register(t2);

// CHECK: %mySBuffer3 = OpVariable %_ptr_Uniform_type_RWStructuredBuffer_S StorageBuffer
RWStructuredBuffer<S> mySBuffer3 : register(u3);
// CHECK: %mySBuffer4 = OpVariable %_ptr_Uniform_type_RWStructuredBuffer_T StorageBuffer
RWStructuredBuffer<T> mySBuffer4 : register(u4);

float4 main() : SV_Target {
    return 1.0;
}
