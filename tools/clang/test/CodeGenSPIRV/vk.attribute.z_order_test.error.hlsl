// Run: %dxc -T ps_6_0 -E main

// CHECK: 4:3: error: vk::z_order_test attribute can be used only for entry points
[[vk::z_order_test("RE_Z")]]
void foo() {}

[[vk::z_order_test("RE_Z")]]
float4 main(float4 offset: A) : SV_Target {
    foo();
    return offset;
}
