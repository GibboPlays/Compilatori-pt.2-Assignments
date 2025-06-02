define void @fusione_ok(ptr %A, i32 %N) {
entry:
  br label %loop1.preheader

loop1.preheader:
  br label %loop1.header

loop1.header:
  %i1 = phi i32 [ 0, %loop1.preheader ], [ %i1.next, %loop1.latch ]
  %v1 = load i32, ptr %A
  %i1.next = add i32 %i1, 1
  %cond1 = icmp slt i32 %i1, %N
  br i1 %cond1, label %loop1.latch, label %loop2.preheader

loop1.latch:
  store i32 %i1, ptr %A
  br label %loop1.header

loop2.preheader:
  br label %loop2.header

loop2.header:
  %i2 = phi i32 [ 0, %loop2.preheader ], [ %i2.next, %loop2.latch ]
  %v2 = load i32, ptr %A
  %i2.next = add i32 %i2, 1
  %cond2 = icmp slt i32 %i2, %N
  br i1 %cond2, label %loop2.latch, label %exit

loop2.latch:
  store i32 %i2, ptr %A
  br label %loop2.header

exit:
  ret void
}
