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
// CHECK: [[foo:%\d+]] = OpString "foo"
// CHECK: [[a_name:%\d+]] = OpString "a"
// CHECK: [[b_name:%\d+]] = OpString "b"
// CHECK: [[c_name:%\d+]] = OpString "c"

// CHECK: OpExtInst %void [[set]] DebugTypeComposite [[foo]] Structure {{%\d+}} 3 1 {{%\d+}} {{%\d+}} %uint_192 FlagIsProtected|FlagIsPrivate [[a:%\d+]] [[b:%\d+]] [[c:%\d+]]
// CHECK: [[a]] = OpExtInst %void [[set]] DebugTypeMember [[a_name]]
// CHECK: [[b]] = OpExtInst %void [[set]] DebugTypeMember [[b_name]]
// CHECK: [[c]] = OpExtInst %void [[set]] DebugTypeMember [[c_name]]

float4 main(float4 color : COLOR) : SV_TARGET {
  foo a;
  a.func0(1);
  a.func1(1, 1, 1);

  return color;
}
