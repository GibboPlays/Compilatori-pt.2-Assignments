define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = add nsw i32 %0, 10
  %4 = sdiv i32 %0, 4
  %5 = sub nsw i32 %3, 10
  %6 = sdiv i32 %5, 1
  %7 = mul nsw i32 %3, %4
  ret i32 %7
}
