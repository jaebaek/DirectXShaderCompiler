// Run: %dxc -T ps_6_0 -E main

struct S {
    float  a;
    float3 b;
    int2 c;
};

struct T {
    float  a;
    float3 b;
    S      c;
    int2   d;
};

StructuredBuffer<S> mySBuffer1 : register(t1);
StructuredBuffer<T> mySBuffer2 : register(t2);

RWStructuredBuffer<S> mySBuffer3 : register(u3);
RWStructuredBuffer<T> mySBuffer4 : register(u4);

void foo(RWStructuredBuffer<S> inputBuffer) {
  inputBuffer[0].a = 0;
}

float4 main() : SV_Target {
    foo(mySBuffer3);
    return 1.0;
}
