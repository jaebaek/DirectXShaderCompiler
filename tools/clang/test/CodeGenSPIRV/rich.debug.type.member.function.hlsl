// Run: %dxc -T ps_6_0 -E main -fspv-debug=rich

struct foo {
  int a;

  void func0(float arg) {
    b.x = arg;
  }

  float4 b;

  int func1(int arg0, float arg1, bool arg2) {
    a = arg0;
    b.y = arg1;
    if (arg2) return arg0;
    return b.z;
  }

  bool c;
};

// CHECK: [[set:%\d+]] = OpExtInstImport "OpenCL.DebugInfo.100"

// CHECK: [[float:%\d+]] = OpExtInst %void [[set]] DebugTypeBasic {{%\d+}} %uint_32 Float
// CHECK: [[int:%\d+]] = OpExtInst %void [[set]] DebugTypeBasic {{%\d+}} %uint_32 Signed
// CHECK: [[bool:%\d+]] = OpExtInst %void [[set]] DebugTypeBasic {{%\d+}} %uint_32 Boolean

// CHECK: {{%\d+}} = OpExtInst %void [[set]] DebugTypeFunction FlagIsProtected|FlagIsPrivate %void [[float]]
// CHECK: {{%\d+}} = OpExtInst %void [[set]] DebugTypeFunction FlagIsProtected|FlagIsPrivate [[int]] [[int]] [[float]] [[bool]]

float4 main(float4 color : COLOR) : SV_TARGET {
  foo a;
  a.func0(1);
  a.func1(1, 1, 1);

  return color;
}
