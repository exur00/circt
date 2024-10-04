// RUN: circt-opt --hw-my-pass %s | FileCheck %s

hw.module @myfoo(in %a: i32, in %b: i32, out out: i32) {
  // CHECK:   %c1_i32 = hw.constant 1 : i32
  %c1 = hw.constant 1 : i32
  // CHECK:   %myfoo_0 = hw.wire %c1_i32  : i32
  %wire_1 = hw.wire %c1 : i32
  // CHECK:   %myfoo_1 = hw.wire %a  : i32
  %wire_a = hw.wire %a : i32
  // CHECK:   %myfoo_2 = hw.wire %b  : i32
  %wire_b = hw.wire %b : i32
  // CHECK:   %0 = comb.add bin %myfoo_1, %myfoo_0 : i32
  %ap1 = comb.add bin %wire_a, %wire_1 : i32
  // CHECK:   %myfoo_3 = hw.wire %0  : i32
  %wire_ap1 = hw.wire %ap1 : i32
  // CHECK:   %1 = comb.add bin %myfoo_3, %myfoo_2 : i32
  %ap1pb = comb.add bin %wire_ap1, %wire_b : i32
  // CHECK:   %myfoo_4 = hw.wire %1  : i32
  %wire_ap1pb = hw.wire %ap1pb : i32
  // CHECK:   hw.output %myfoo_4 : i32
  hw.output %wire_ap1pb : i32
}
