define void @non_adiacenti(ptr %A, i32 %N) {
entry:
  br label %loop1

loop1:
  %i1 = phi i32 [ 0, %entry ], [ %i1.next, %loop1 ]
  %i1.next = add i32 %i1, 1
  store i32 %i1, ptr %A
  %cond1 = icmp slt i32 %i1, %N
  br i1 %cond1, label %loop1, label %middle

middle:
  br label %loop2

loop2:
  %i2 = phi i32 [ 0, %middle ], [ %i2.next, %loop2 ]
  %i2.next = add i32 %i2, 1
  store i32 %i2, ptr %A
  %cond2 = icmp slt i32 %i2, %N
  br i1 %cond2, label %loop2, label %exit

exit:
  ret void
}
