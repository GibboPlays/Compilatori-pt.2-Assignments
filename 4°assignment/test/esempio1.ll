define void @two_loops_cfe_compliant(i32* %A, i32 %N) {
entry:
  br label %common.entry_point

common.entry_point:
  %i_common = phi i32 [ 0, %entry ], [ %i_common_next, %common.exit_point ]
  br label %loop1.header

loop1.header:
  %i1 = phi i32 [ %i_common, %common.entry_point ], [ %i1.next, %loop1.latch ]
  %val1 = load i32, i32* %A
  %i1.next = add i32 %i1, 1
  %cond1 = icmp slt i32 %i1, %N
  br i1 %cond1, label %loop1.latch, label %loop2.header

loop1.latch:
  store i32 %i1, i32* %A
  br label %loop1.header

loop2.header:
  %i2 = phi i32 [ %i_common, %loop1.header ], [ %i2.next, %loop2.latch ]
  %val2 = load i32, i32* %A
  %i2.next = add i32 %i2, 1
  %cond2 = icmp slt i32 %i2, %N
  br i1 %cond2, label %loop2.latch, label %common.exit_point

loop2.latch:
  store i32 %i2, i32* %A
  br label %loop2.header

common.exit_point:
  %i_common_next = add i32 %i_common, 1
  %cond_common = icmp slt i32 %i_common_next, %N
  br i1 %cond_common, label %common.entry_point, label %final.exit

final.exit:
  ret void
}