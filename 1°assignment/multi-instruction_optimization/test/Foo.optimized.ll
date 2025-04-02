; ModuleID = '../test/Foo.optimized.bc'
source_filename = "../test/Foo.ll"

define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) {
  %3 = add nsw i32 %0, 10
  %4 = sdiv i32 %0, 4
  %5 = sdiv i32 %3, 1
  %6 = mul nsw i32 %3, %4
  ret i32 %6
}
